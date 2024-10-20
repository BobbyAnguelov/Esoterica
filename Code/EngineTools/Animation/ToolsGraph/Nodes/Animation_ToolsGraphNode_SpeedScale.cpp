#include "Animation_ToolsGraphNode_SpeedScale.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_SpeedScale.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    SpeedScaleToolsNode::SpeedScaleToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Scale", GraphValueType::Float );
    }

    int16_t SpeedScaleToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SpeedScaleNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<SpeedScaleNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_childNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                if ( m_multiplier <= 0.0f )
                {
                    context.LogError( this, "Invalid desired multiplier for speed scale node! Multiplier must be > 0.0f." );
                    return InvalidIndex;
                }
            }
        }

        pDefinition->m_defaultInputValue = m_multiplier;
        return pDefinition->m_nodeIdx;
    }

    void SpeedScaleToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        auto pSpeedInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
        if ( pSpeedInputNode == nullptr )
        {
            BeginDrawInternalRegion( ctx );
            ImGui::Text( "Speed Scale = %.2f", m_multiplier );
            EndDrawInternalRegion( ctx );
        }
    }

    //-------------------------------------------------------------------------

    DurationScaleToolsNode::DurationScaleToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "New Duration", GraphValueType::Float );
    }

    int16_t DurationScaleToolsNode::Compile( GraphCompilationContext& context ) const
    {
        DurationScaleNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<DurationScaleNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_childNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                if ( m_desiredDuration <= 0.0f )
                {
                    context.LogError( this, "Invalid desired duration for duration scale node! Duration must be > 0.0f." );
                    return InvalidIndex;
                }
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_defaultInputValue = m_desiredDuration;
        return pDefinition->m_nodeIdx;
    }

    void DurationScaleToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        auto pDurationInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
        if ( pDurationInputNode == nullptr )
        {
            BeginDrawInternalRegion( ctx );
            ImGui::Text( "Duration = %.2f", m_desiredDuration );
            EndDrawInternalRegion( ctx );
        }
    }

    //-------------------------------------------------------------------------

    VelocityBasedSpeedScaleToolsNode::VelocityBasedSpeedScaleToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Desired Velocity", GraphValueType::Float );
    }

    bool VelocityBasedSpeedScaleToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx == 0 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    int16_t VelocityBasedSpeedScaleToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VelocityBasedSpeedScaleNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<VelocityBasedSpeedScaleNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_childNodeIdx = compiledNodeIdx;
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
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                if ( m_desiredVelocity <= 0.0f )
                {
                    context.LogError( this, "Invalid desired velocity for velocity based speed scale node! Velocity must be > 0.0f." );
                    return InvalidIndex;
                }
            }
        }

        pDefinition->m_defaultInputValue = m_desiredVelocity;
        return pDefinition->m_nodeIdx;
    }

    void VelocityBasedSpeedScaleToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        auto pSpeedInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
        if ( pSpeedInputNode == nullptr )
        {
            BeginDrawInternalRegion( ctx );
            ImGui::Text( "Velocity = %.2f", m_desiredVelocity );
            EndDrawInternalRegion( ctx );
        }
    }
}