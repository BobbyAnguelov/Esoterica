#include "Animation_ToolsGraphNode_Warping.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_OrientationWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void OrientationWarpToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, false );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Direction (Character)", GraphValueType::Vector );
        CreateInputPin( "Angle Offset (Deg)", GraphValueType::Float );
    }

    int16_t OrientationWarpToolsNode::Compile( GraphCompilationContext& context ) const
    {
        OrientationWarpNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<OrientationWarpNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_clipReferenceNodeIdx = compiledNodeIdx;
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
                    pSettings->m_targetValueNodeIdx = compiledNodeIdx;
                    pSettings->m_isOffsetNode = false;
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
                    pSettings->m_targetValueNodeIdx = compiledNodeIdx;
                    pSettings->m_isOffsetNode = true;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_isOffsetRelativeToCharacter = ( m_offsetType == OffsetType::RelativeToCharacter );
        }
        return pSettings->m_nodeIdx;
    }

    bool OrientationWarpToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx == 0 )
        {
            return IsOfType<AnimationClipToolsNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    //-------------------------------------------------------------------------

    void TargetWarpToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, false );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "World Target", GraphValueType::Target );
    }

    int16_t TargetWarpToolsNode::Compile( GraphCompilationContext& context ) const
    {
        TargetWarpNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<TargetWarpNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_clipReferenceNodeIdx = compiledNodeIdx;
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
                    pSettings->m_targetValueNodeIdx = compiledNodeIdx;
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

        pSettings->m_allowTargetUpdate = m_allowTargetUpdate;
        pSettings->m_samplingMode = m_samplingMode;
        pSettings->m_samplingPositionErrorThresholdSq = Math::Sqr( m_samplingPositionErrorThreshold );
        pSettings->m_maxTangentLength = m_maxTangentLength;
        pSettings->m_lerpFallbackDistanceThreshold = m_lerpFallbackDistanceThreshold;
        pSettings->m_targetUpdateDistanceThreshold = m_targetUpdateDistanceThreshold;
        pSettings->m_targetUpdateAngleThresholdRadians = m_targetUpdateAngleThreshold.ToRadians().ToFloat();

        return pSettings->m_nodeIdx;
    }

    bool TargetWarpToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx == 0 )
        {
            return IsOfType<AnimationClipToolsNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }
}