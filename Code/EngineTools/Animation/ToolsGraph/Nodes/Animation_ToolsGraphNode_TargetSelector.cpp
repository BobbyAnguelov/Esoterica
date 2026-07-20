#include "Animation_ToolsGraphNode_TargetSelector.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TargetSelector.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TargetSelectorToolsNode::TargetSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Target", GraphValueType::Target );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
    }

    void TargetSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        auto pCreatedPin = GetInputPin( pinID );
        m_optionLabels.emplace_back( pCreatedPin->m_name );
        RefreshDynamicPins();
    }

    void TargetSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        m_optionLabels.erase( m_optionLabels.begin() + pinToBeRemovedIdx );
    }

    void TargetSelectorToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_optionLabels" ) )
        {
            RefreshDynamicPins();
        }
    }

    void TargetSelectorToolsNode::RefreshDynamicPins()
    {
        int32_t firstDynamicPinIdx = InvalidIndex;
        int32_t const numInputPins = GetNumInputPins();
        for ( int32_t i = 0; i < numInputPins; i++ )
        {
            if ( GetInputPins()[i].IsDynamicPin() )
            {
                firstDynamicPinIdx = i;
                break;
            }
        }

        if ( firstDynamicPinIdx != InvalidIndex )
        {
            for ( int32_t i = firstDynamicPinIdx; i < numInputPins; i++ )
            {
                NodeGraph::Pin* pInputPin = GetInputPin( i );
                pInputPin->m_name.sprintf( "%s", m_optionLabels[i - firstDynamicPinIdx].empty() ? "Option" : m_optionLabels[i - firstDynamicPinIdx].c_str() );
            }
        }
    }

    bool TargetSelectorToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 1 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    int16_t TargetSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TargetSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TargetSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( Math::IsNearZero( m_orientationScoreWeight ) && Math::IsNearZero( m_positionScoreWeight ) )
            {
                context.LogError( this, "Both rotation and translation are not allowed to be zero!" );
                return InvalidIndex;
            }

            int32_t const numInputPins = GetNumInputPins();
            if ( numInputPins == 1 )
            {
                context.LogError( this, "No options on selector node!" );
                return InvalidIndex;
            }

            pDefinition->m_ignoreInvalidOptions = m_ignoreInvalidOptions;

            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_parameterNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on target selector node!" );
                return InvalidIndex;
            }

            // Compile Options
            //-------------------------------------------------------------------------

            for ( auto i = 1; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }

        pDefinition->m_orientationScoreWeight = m_orientationScoreWeight;
        pDefinition->m_positionScoreWeight = m_positionScoreWeight;
        pDefinition->m_isWorldSpaceTarget = m_isWorldSpaceTarget;

        return pDefinition->m_nodeIdx;
    }
}