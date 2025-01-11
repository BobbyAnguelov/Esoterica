#include "Animation_ToolsGraphNode_Warping.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_OrientationWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    OrientationWarpToolsNode::OrientationWarpToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Direction (Character)", GraphValueType::Vector );
        CreateInputPin( "Angle Offset (Deg)", GraphValueType::Float );
    }

    int16_t OrientationWarpToolsNode::Compile( GraphCompilationContext& context ) const
    {
        OrientationWarpNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<OrientationWarpNode>( this, pDefinition );
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

            auto pDirectionNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            auto pOffsetNode = GetConnectedInputNode<FlowToolsNode>( 2 );

            if ( pDirectionNode != nullptr && pOffsetNode != nullptr )
            {
                context.LogError( this, "Cannot have both the direction and the offset inputs set, these are mutually exclusive!" );
                return InvalidIndex;
            }

            if ( pDirectionNode == nullptr && pOffsetNode == nullptr )
            {
                context.LogError( this, "You need to have either the direction or the offset node connected!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            // Direction
            if ( pDirectionNode != nullptr )
            {
                int16_t const compiledNodeIdx = pDirectionNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_targetValueNodeIdx = compiledNodeIdx;
                    pDefinition->m_isOffsetNode = false;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else // Offset node
            {
                int16_t const compiledNodeIdx = pOffsetNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_targetValueNodeIdx = compiledNodeIdx;
                    pDefinition->m_isOffsetNode = true;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pDefinition->m_isOffsetRelativeToCharacter = ( m_offsetType == OffsetType::RelativeToCharacter );
            pDefinition->m_samplingMode = m_samplingMode;
        }
        return pDefinition->m_nodeIdx;
    }

    bool OrientationWarpToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx == 0 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    //-------------------------------------------------------------------------

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

        pDefinition->m_allowTargetUpdate = m_allowTargetUpdate;
        pDefinition->m_samplingMode = m_samplingMode;
        pDefinition->m_samplingPositionErrorThresholdSq = Math::Sqr( m_samplingPositionErrorThreshold );
        pDefinition->m_maxTangentLength = m_maxTangentLength;
        pDefinition->m_lerpFallbackDistanceThreshold = m_lerpFallbackDistanceThreshold;
        pDefinition->m_targetUpdateDistanceThreshold = m_targetUpdateDistanceThreshold;
        pDefinition->m_targetUpdateAngleThresholdRadians = m_targetUpdateAngleThreshold.ToRadians().ToFloat();

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