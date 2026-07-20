#include "Animation_ToolsGraphNode_TargetWarp.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TargetWarpToolsNode::TargetWarpToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "World Target", GraphValueType::Target );
    }

    int16_t TargetWarpToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TargetWarpNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<TargetWarpNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_clipReferenceNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_targetValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_targetUpdateRule = m_targetUpdateRule;
        pDefinition->m_alignWithTargetAtLastWarpEvent = m_alignWithTargetAtLastWarpEvent;
        pDefinition->m_samplingMode = m_samplingMode;
        pDefinition->m_samplingPositionErrorThresholdSq = Math::Sqr( m_samplingPositionErrorThreshold );
        pDefinition->m_maxTangentLength = m_maxTangentLength;
        pDefinition->m_lerpFallbackDistanceThreshold = m_lerpFallbackDistanceThreshold;
        pDefinition->m_targetUpdateDistanceThreshold = m_targetUpdateDistanceThreshold;
        pDefinition->m_targetUpdateAngleThresholdRadians = m_targetUpdateAngleThreshold.ToRadians().ToFloat();
        pDefinition->m_alignmentBoneID = m_alignmentBoneID;

        return pDefinition->m_nodeIdx;
    }

    bool TargetWarpToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx == 0 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }
}