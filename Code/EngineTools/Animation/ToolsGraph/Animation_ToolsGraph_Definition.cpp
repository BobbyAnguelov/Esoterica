#include "Animation_ToolsGraph_Definition.h"
#include "Nodes/Animation_ToolsGraphNode_Result.h"
#include "Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "Nodes/Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ToolsGraphDefinition::CreateDefaultRootGraph()
    {
        m_variationHierarchy.Reset();
        m_rootGraph.CreateInstance<FlowGraph>( GraphType::BlendTree );
        m_rootGraph.Get()->m_ID = UUID::GenerateID();
        m_rootGraph.Get()->CreateNode<PoseResultToolsNode>();
    }

    //-------------------------------------------------------------------------

    void ToolsGraphDefinition::UpdateInternalReferencesAndClones( TypeSystem::TypeRegistry const& typeRegistry )
    {
        StateToolsNode::UpdateAllClonedStates( typeRegistry, m_rootGraph.Get() );
        ParameterBaseToolsNode::RefreshParameterReferences( m_rootGraph.Get() );
    }

    void ToolsGraphDefinition::ReflectParameters( ToolsGraphDefinition const& otherGraphDefinition, bool reflectVirtualParameters, TVector<String>* pOptionalOutputLog )
    {
        EE_ASSERT( otherGraphDefinition.IsValid() );
        FlowGraph const* pOtherRootGraph = otherGraphDefinition.m_rootGraph.Get();
        EE_ASSERT( pOtherRootGraph != nullptr );
        auto otherControlParameters = pOtherRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        auto otherVirtualParameters = pOtherRootGraph->FindAllNodesOfType<VirtualParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );

        FlowGraph* pRootGraph = GetRootGraph();
        auto controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        auto virtualParameters = pRootGraph->FindAllNodesOfType<VirtualParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );

        //-------------------------------------------------------------------------

        enum SearchResult { CanAdd, MismatchedCase, MismatchedType, AlreadyExists };

        auto CanAddParameter = [&] ( String parameterName, GraphValueType type )
        {
            for ( auto pControlParameter : controlParameters )
            {
                if ( pControlParameter->GetParameterName().comparei( parameterName ) == 0 )
                {
                    // Names dont match in case
                    if ( pControlParameter->GetParameterName().compare( parameterName ) != 0 )
                    {
                        return SearchResult::MismatchedCase;
                    }

                    // Types dont match
                    if ( pControlParameter->GetOutputValueType() != type )
                    {
                        return SearchResult::MismatchedType;
                    }

                    return SearchResult::AlreadyExists;
                }
            }

            //-------------------------------------------------------------------------

            for ( auto pVirtualParameter : virtualParameters )
            {
                if ( pVirtualParameter->GetParameterName().comparei( parameterName ) == 0 )
                {
                    // Names dont match in case
                    if ( pVirtualParameter->GetParameterName().compare( parameterName ) != 0 )
                    {
                        return SearchResult::MismatchedType;
                    }

                    // Types dont match
                    if ( pVirtualParameter->GetOutputValueType() != type )
                    {
                        return SearchResult::MismatchedType;
                    }

                    return SearchResult::AlreadyExists;
                }
            }

            //-------------------------------------------------------------------------

            return SearchResult::CanAdd;
        };

        auto ProcessResult = [&] ( SearchResult result, GraphValueType type, String const& parameterName, String const& parameterCategory, ControlParameterToolsNode const* pOtherParameterNode = nullptr )
        {
            switch ( result )
            {
                case SearchResult::CanAdd:
                {
                    auto pCreatedParameterNode = ControlParameterToolsNode::Create( pRootGraph, type, parameterName, parameterCategory );
                    pCreatedParameterNode->ReflectPreviewValues( pOtherParameterNode );

                    if ( pOptionalOutputLog != nullptr )
                    {
                        pOptionalOutputLog->emplace_back( String::CtorSprintf(), "Parameter Added: %s", parameterName.c_str() );
                    }
                }
                break;

                case SearchResult::MismatchedCase:
                {
                    if ( pOptionalOutputLog != nullptr )
                    {
                        pOptionalOutputLog->emplace_back( String::CtorSprintf(), "Parameter Exists with Incorrect Case: %s\n", parameterName.c_str() );
                    }
                }
                break;

                case SearchResult::MismatchedType:
                {
                    if ( pOptionalOutputLog != nullptr )
                    {
                        pOptionalOutputLog->emplace_back( String::CtorSprintf(), "Parameter Exists with Incorrect Type: %s\n", parameterName.c_str() );
                    }
                }
                break;

                case SearchResult::AlreadyExists:
                {
                    pOptionalOutputLog->emplace_back( String::CtorSprintf(), "Parameter Exists, Reflecting preview values: %s\n", parameterName.c_str() );

                    for ( auto pExistingControlParameter : controlParameters )
                    {
                        if ( pExistingControlParameter->GetParameterName().comparei( parameterName ) == 0 )
                        {
                            pExistingControlParameter->ReflectPreviewValues( pOtherParameterNode );
                            break;
                        }
                    }
                }
                break;

                default:
                break;
            }
        };

        //-------------------------------------------------------------------------

        for ( auto pParameterNode : otherControlParameters )
        {
            SearchResult const result = CanAddParameter( pParameterNode->GetParameterName(), pParameterNode->GetOutputValueType() );
            ProcessResult( result, pParameterNode->GetOutputValueType(), pParameterNode->GetParameterName(), pParameterNode->GetParameterGroup(), pParameterNode );
        }

        //-------------------------------------------------------------------------

        if ( reflectVirtualParameters )
        {
            for ( auto pParameterNode : otherVirtualParameters )
            {
                SearchResult const result = CanAddParameter( pParameterNode->GetParameterName(), pParameterNode->GetOutputValueType() );
                ProcessResult( result, pParameterNode->GetOutputValueType(), pParameterNode->GetParameterName(), pParameterNode->GetParameterGroup() );
            }
        }
    }

    void ToolsGraphDefinition::PostDeserialize( TypeSystem::TypeRegistry const& typeRegistry )
    {
        IReflectedType::PostDeserialize( typeRegistry );

        if ( m_rootGraph.IsSet() )
        {
            UpdateInternalReferencesAndClones( typeRegistry );
        }
    }

    TInlineVector<EE::StringID, 10> ToolsGraphDefinition::GetVariationIDs() const
    {
        TInlineVector<EE::StringID, 10> IDs;
        for ( Variation const& variation : m_variationHierarchy.GetAllVariations() )
        {
            IDs.emplace_back( variation.m_ID );
        }

        return IDs;
    }
}