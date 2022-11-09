#include "Animation_ToolsGraph_Definition.h"
#include "Nodes/Animation_ToolsGraphNode_Result.h"
#include "Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using namespace EE::Animation::GraphNodes;

    //-------------------------------------------------------------------------

    ToolsGraphDefinition::ToolsGraphDefinition()
    {
        ResetToDefaultState();
    }

    ToolsGraphDefinition::~ToolsGraphDefinition()
    {
        ResetInternalState();
    }

    void ToolsGraphDefinition::ResetInternalState()
    {
        m_variationHierarchy.Reset();

        if ( m_pRootGraph != nullptr )
        {
            m_pRootGraph->Shutdown();
            EE::Delete( m_pRootGraph );
        }
    }

    void ToolsGraphDefinition::ResetToDefaultState()
    {
        ResetInternalState();
        m_pRootGraph = EE::New<FlowGraph>( GraphType::BlendTree );
        m_pRootGraph->CreateNode<ResultToolsNode>( GraphValueType::Pose );
    }

    void ToolsGraphDefinition::RefreshParameterReferences()
    {
        EE_ASSERT( m_pRootGraph != nullptr );
        auto controlParameters = m_pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        auto const virtualParameters = m_pRootGraph->FindAllNodesOfType<VirtualParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );

        //-------------------------------------------------------------------------

        TInlineVector<ParameterReferenceToolsNode*, 10> invalidReferenceNodes; // These nodes are invalid and need to be removed

        auto parameterReferenceNodes = m_pRootGraph->FindAllNodesOfType<ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( auto pReferenceNode : parameterReferenceNodes )
        {
            FlowToolsNode* pFoundParameterNode = nullptr;
            FlowToolsNode* pFoundMatchingNameParameterNode = nullptr;

            // Check all control parameters for matching ID or matching name
            for ( auto pParameter : controlParameters )
            {
                if ( pParameter->GetID() == pReferenceNode->GetReferencedParameterID() )
                {
                    EE_ASSERT( pFoundParameterNode == nullptr );
                    pFoundParameterNode = pParameter;
                    break;
                }

                if ( pParameter->GetParameterName().comparei( pReferenceNode->GetReferencedParameterName() ) == 0 )
                {
                    EE_ASSERT( pFoundMatchingNameParameterNode == nullptr );
                    pFoundMatchingNameParameterNode = pParameter;
                }
            }

            // Check all virtual parameters for matching ID
            if ( pFoundParameterNode == nullptr )
            {
                for ( auto pParameter : virtualParameters )
                {
                    if ( pParameter->GetID() == pReferenceNode->GetReferencedParameterID() )
                    {
                        EE_ASSERT( pFoundParameterNode == nullptr );
                        pFoundParameterNode = pParameter;
                        break;
                    }

                    if ( pParameter->GetParameterName().comparei( pReferenceNode->GetReferencedParameterName() ) == 0 )
                    {
                        EE_ASSERT( pFoundMatchingNameParameterNode == nullptr );
                        pFoundMatchingNameParameterNode = pParameter;
                    }
                }
            }

            // If we have a matching parameter node, set the reference to point to it
            if ( pFoundParameterNode != nullptr && ( pReferenceNode->GetReferencedParameterValueType() == pFoundParameterNode->GetValueType() ) )
            {
                pReferenceNode->SetReferencedParameter( pFoundParameterNode );
            }
            // If we have a parameter that matches both name and type then link to it (this handles cross graph pasting of parameters)
            else if ( pFoundMatchingNameParameterNode != nullptr )
            {
                if ( pReferenceNode->GetReferencedParameterValueType() == pFoundMatchingNameParameterNode->GetValueType() )
                {
                    pReferenceNode->SetReferencedParameter( pFoundMatchingNameParameterNode );
                }
                else // Flag this reference as invalid since we cannot create a parameter with this name as one already exists
                {
                    invalidReferenceNodes.emplace_back( pReferenceNode );
                }
            }
            else // Create missing parameter
            {
                GraphNodes::ControlParameterToolsNode* pParameter = nullptr;

                switch ( pReferenceNode->GetReferencedParameterValueType() )
                {
                    case GraphValueType::Bool:
                    pParameter = m_pRootGraph->CreateNode<GraphNodes::BoolControlParameterToolsNode>( pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterCategory() );
                    break;

                    case GraphValueType::ID:
                    pParameter = m_pRootGraph->CreateNode<GraphNodes::IDControlParameterToolsNode>( pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterCategory() );
                    break;

                    case GraphValueType::Int:
                    pParameter = m_pRootGraph->CreateNode<GraphNodes::IntControlParameterToolsNode>( pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterCategory() );
                    break;

                    case GraphValueType::Float:
                    pParameter = m_pRootGraph->CreateNode<GraphNodes::FloatControlParameterToolsNode>( pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterCategory() );
                    break;

                    case GraphValueType::Vector:
                    pParameter = m_pRootGraph->CreateNode<GraphNodes::VectorControlParameterToolsNode>( pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterCategory() );
                    break;

                    case GraphValueType::Target:
                    pParameter = m_pRootGraph->CreateNode<GraphNodes::TargetControlParameterToolsNode>( pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterCategory() );
                    break;

                    default:
                    {
                        EE_UNREACHABLE_CODE();
                    }
                    break;
                }

                // Set the reference to the newly created parameter
                pReferenceNode->SetReferencedParameter( pParameter );

                // Add newly created parameter to the list of parameters to be used for references
                controlParameters.emplace_back( pParameter );
            }
        }

        // Destroy any invalid reference nodes
        //-------------------------------------------------------------------------

        for ( auto pInvalidNode : invalidReferenceNodes )
        {
            pInvalidNode->Destroy();
        }
    }

    //-------------------------------------------------------------------------

    bool ToolsGraphDefinition::LoadFromJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphDescriptorObjectValue )
    {
        EE_ASSERT( graphDescriptorObjectValue.IsObject() );

        ResetInternalState();

        // Find relevant json values
        //-------------------------------------------------------------------------

        Serialization::JsonValue const* pGraphObjectValue = nullptr;
        Serialization::JsonValue const* pVariationsObjectValue = nullptr;
        Serialization::JsonValue const* pRootGraphObjectValue = nullptr;

        auto graphDefinitionValueIter = graphDescriptorObjectValue.FindMember( "GraphDefinition" );
        if ( graphDefinitionValueIter != graphDescriptorObjectValue.MemberEnd() && graphDefinitionValueIter->value.IsObject() )
        {
            pGraphObjectValue = &graphDefinitionValueIter->value;
        }

        if ( pGraphObjectValue != nullptr )
        {
            auto rootGraphValueIter = pGraphObjectValue->FindMember( "RootGraph" );
            if ( rootGraphValueIter != pGraphObjectValue->MemberEnd() )
            {
                pRootGraphObjectValue = &rootGraphValueIter->value;
            }
        }

        if ( pGraphObjectValue != nullptr )
        {
            auto variationsValueIter = pGraphObjectValue->FindMember( "Variations" );
            if ( variationsValueIter != pGraphObjectValue->MemberEnd() && variationsValueIter->value.IsArray() )
            {
                pVariationsObjectValue = &variationsValueIter->value;
            }
        }

        // Deserialize graph
        //-------------------------------------------------------------------------

        bool serializationSuccessful = false;

        if ( pRootGraphObjectValue != nullptr && pVariationsObjectValue != nullptr )
        {
            m_pRootGraph = Cast<FlowGraph>( VisualGraph::BaseGraph::CreateGraphFromSerializedData( typeRegistry, *pRootGraphObjectValue, nullptr ) );
            serializationSuccessful = m_variationHierarchy.Serialize( typeRegistry, *pVariationsObjectValue );
        }

        // If serialization failed, reset the graph state to a valid one
        if ( !serializationSuccessful )
        {
            ResetToDefaultState();
        }

        //-------------------------------------------------------------------------

        if ( serializationSuccessful )
        {
            RefreshParameterReferences();
        }

        return serializationSuccessful;
    }

    void ToolsGraphDefinition::SaveToJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.StartObject();
        writer.Key( Serialization::s_typeIDKey );
        writer.String( GraphResourceDescriptor::GetStaticTypeID().c_str() );

        writer.Key( "GraphDefinition" );
        writer.StartObject();
        {
            writer.Key( "RootGraph" );
            m_pRootGraph->Serialize( typeRegistry, writer );

            writer.Key( "Variations" );
            m_variationHierarchy.Serialize( typeRegistry, writer );

            writer.EndObject();
        }
        writer.EndObject();
    }
}