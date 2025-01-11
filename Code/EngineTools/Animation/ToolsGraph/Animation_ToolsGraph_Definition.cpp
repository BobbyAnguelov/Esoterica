#include "Animation_ToolsGraph_Definition.h"
#include "Nodes/Animation_ToolsGraphNode_Result.h"
#include "Nodes/Animation_ToolsGraphNode_Parameters.h"
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

    void ToolsGraphDefinition::RefreshParameterReferences()
    {
        EE_ASSERT( m_rootGraph.IsSet() );
        auto parameterNodes = m_rootGraph->FindAllNodesOfType<ParameterBaseToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );

        //-------------------------------------------------------------------------

        auto SetReferencedParameter = [] ( ParameterReferenceToolsNode* pReferenceNode, ParameterBaseToolsNode* pParameter )
        {
            EE_ASSERT( pParameter != nullptr );
            EE_ASSERT( IsOfType<ControlParameterToolsNode>( pParameter ) || IsOfType<VirtualParameterToolsNode>( pParameter ) );
            EE_ASSERT( pParameter->GetOutputValueType() == pReferenceNode->GetOutputValueType() );
            pReferenceNode->m_pParameter = pParameter;
            pReferenceNode->UpdateCachedParameterData();
        };

        TInlineVector<ParameterReferenceToolsNode*, 10> invalidReferenceNodes; // These nodes are invalid and need to be removed

        auto parameterReferenceNodes = m_rootGraph->FindAllNodesOfType<ParameterReferenceToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pReferenceNode : parameterReferenceNodes )
        {
            ParameterBaseToolsNode* pFoundParameterNode = nullptr;
            ParameterBaseToolsNode* pFoundMatchingNameParameterNode = nullptr;

            // Check all parameters for matching ID or matching name
            for ( auto pParameterNode : parameterNodes )
            {
                if ( pParameterNode->GetID() == pReferenceNode->GetReferencedParameterID() )
                {
                    EE_ASSERT( pFoundParameterNode == nullptr );
                    pFoundParameterNode = pParameterNode;
                    break;
                }

                if ( pParameterNode->GetParameterName().comparei( pReferenceNode->GetReferencedParameterName() ) == 0 )
                {
                    EE_ASSERT( pFoundMatchingNameParameterNode == nullptr );
                    pFoundMatchingNameParameterNode = pParameterNode;
                }
            }

            GraphValueType const referencedParameterValueType = pReferenceNode->GetReferencedParameterValueType();

            // If we have a matching parameter node, set the reference to point to it
            if ( pFoundParameterNode != nullptr && ( referencedParameterValueType == pFoundParameterNode->GetOutputValueType() ) )
            {
                SetReferencedParameter( pReferenceNode, pFoundParameterNode );
            }
            // If we have a parameter that matches both name and type then link to it (this handles cross graph pasting of parameters)
            else if ( pFoundMatchingNameParameterNode != nullptr )
            {
                if ( referencedParameterValueType == pFoundMatchingNameParameterNode->GetOutputValueType() )
                {
                    SetReferencedParameter( pReferenceNode, pFoundMatchingNameParameterNode );
                }
                else // Flag this reference as invalid since we cannot create a parameter with this name as one already exists
                {
                    invalidReferenceNodes.emplace_back( pReferenceNode );
                }
            }
            else // Create missing parameter
            {
                auto pParameter = ControlParameterToolsNode::Create( m_rootGraph.Get(), referencedParameterValueType, pReferenceNode->GetReferencedParameterName(), pReferenceNode->GetReferencedParameterGroup() );

                // Set the reference to the newly created parameter
                SetReferencedParameter( pReferenceNode, pParameter );

                // Add newly created parameter to the list of parameters to be used for references
                parameterNodes.emplace_back( pParameter );
            }
        }

        // Destroy any invalid reference nodes
        //-------------------------------------------------------------------------

        for ( auto pInvalidNode : invalidReferenceNodes )
        {
            pInvalidNode->Destroy();
        }
    }

    void ToolsGraphDefinition::ReflectParameters( ToolsGraphDefinition const& otherGraphDefinition, bool reflectVirtualParameters, TVector<String>* pOptionalOutputLog )
    {
        FlowGraph const* pOtherRootGraph = otherGraphDefinition.m_rootGraph.Get();
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

    void ToolsGraphDefinition::PostDeserialize()
    {
        IReflectedType::PostDeserialize();

        if ( m_rootGraph.IsSet() )
        {
            RefreshParameterReferences();
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