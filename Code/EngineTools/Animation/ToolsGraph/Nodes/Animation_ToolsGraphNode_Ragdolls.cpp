#include "Animation_ToolsGraphNode_Ragdolls.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_PoweredRagdoll.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_SimulatedRagdoll.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    PoweredRagdollToolsNode::PoweredRagdollToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Physics Blend Weight", GraphValueType::Float );
        CreateInputPin( "Impulse Origin", GraphValueType::Vector );
        CreateInputPin( "Impulse Force", GraphValueType::Vector );
    }

    int16_t PoweredRagdollToolsNode::Compile( GraphCompilationContext& context ) const
    {
        PoweredRagdollNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<PoweredRagdollNode>( this, pDefinition );
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
                    pDefinition->m_physicsBlendWeightNodeIdx = compiledNodeIdx;
                }
                else
                {
                    context.LogError( this, "Failed to compile physics blend weight input node!" );
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            auto pImpulseOriginNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pImpulseOriginNode != nullptr )
            {
                int16_t const compiledNodeIdx = pImpulseOriginNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inpulseOriginVectorNodeIdx = compiledNodeIdx;
                }
                else
                {
                    context.LogError( this, "Failed to compile impulse vector source input node!" );
                    return InvalidIndex;
                }
            }

            auto pImpulseForceNode = GetConnectedInputNode<FlowToolsNode>( 3 );
            if ( pImpulseForceNode != nullptr )
            {
                int16_t const compiledNodeIdx = pImpulseForceNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inpulseForceVectorNodeIdx = compiledNodeIdx;
                }
                else
                {
                    context.LogError( this, "Failed to compile impulse force input node!" );
                    return InvalidIndex;
                }
            }

            if ( pImpulseOriginNode != nullptr || pImpulseForceNode != nullptr )
            {
                if ( pImpulseOriginNode == nullptr || pImpulseForceNode == nullptr )
                {
                    context.LogError( this, "For impulse support, you need both origin and force nodes set." );
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            pDefinition->m_dataSlotIdx = context.RegisterResource( pData->m_ragdollDefinition.GetResourceID() );
            pDefinition->m_physicsBlendWeight = m_physicsBlendWeight;
            pDefinition->m_isGravityEnabled = m_isGravityEnabled;
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    SimulatedRagdollToolsNode::SimulatedRagdollToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Result", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Trigger Exit", GraphValueType::Bool );
        CreateInputPin( "Exit Option", GraphValueType::Pose );
    }

    bool SimulatedRagdollToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx != 1 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    int16_t SimulatedRagdollToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SimulatedRagdollNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<SimulatedRagdollNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Entry
            //-------------------------------------------------------------------------

            auto pEntryNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pEntryNode != nullptr )
            {
                int16_t const compiledNodeIdx = pEntryNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_entryNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected entry clip pin!" );
                return InvalidIndex;
            }

            // Exit condition
            //-------------------------------------------------------------------------

            auto pExitConditionNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pExitConditionNode != nullptr )
            {
                int16_t const compiledNodeIdx = pExitConditionNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_exitConditionNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            // Exits
            //-------------------------------------------------------------------------

            for ( auto i = 2; i < GetNumInputPins(); i++ )
            {
                auto pExitOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pExitOptionNode != nullptr )
                {
                    int16_t const compiledNodeIdx = pExitOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_exitOptionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            pDefinition->m_dataSlotIdx = context.RegisterResource( pData->m_ragdollDefinition.GetResourceID() );
            pDefinition->m_blendTime = m_blendTime;
        }
        return pDefinition->m_nodeIdx;
    }
}