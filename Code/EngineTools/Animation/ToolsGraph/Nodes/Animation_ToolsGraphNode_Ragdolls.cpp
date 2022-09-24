#include "Animation_ToolsGraphNode_Ragdolls.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_PoweredRagdoll.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_SimulatedRagdoll.h"
#include "Engine/Physics/PhysicsRagdoll.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void PoweredRagdollToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Physics Blend Weight", GraphValueType::Float );
    }

    int16_t PoweredRagdollToolsNode::Compile( GraphCompilationContext& context ) const
    {
        PoweredRagdollNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<PoweredRagdollNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_childNodeIdx = compiledNodeIdx;
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
                    pSettings->m_physicsBlendWeightNodeIdx = compiledNodeIdx;
                }
                else
                {
                    context.LogError( this, "Failed to compile physics blend weight node!" );
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_dataSlotIdx = context.RegisterDataSlotNode( GetID() );
            pSettings->m_profileID = m_profileID;
            pSettings->m_physicsBlendWeight = m_physicsBlendWeight;
            pSettings->m_isGravityEnabled = m_isGravityEnabled;
        }
        return pSettings->m_nodeIdx;
    }

    ResourceTypeID PoweredRagdollToolsNode::GetSlotResourceTypeID() const
    {
        return Physics::RagdollDefinition::GetStaticResourceTypeID();
    }

    //-------------------------------------------------------------------------

    void SimulatedRagdollToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );

        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Exit Option", GraphValueType::Pose );
    }

    bool SimulatedRagdollToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        return IsOfType<AnimationClipToolsNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceToolsNode>( pOutputPinNode );
    }

    int16_t SimulatedRagdollToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SimulatedRagdollNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<SimulatedRagdollNode>( this, pSettings );
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
                    pSettings->m_entryNodeIdx = compiledNodeIdx;
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

            // Exits
            //-------------------------------------------------------------------------

            for ( auto i = 1; i < GetNumInputPins(); i++ )
            {
                auto pExitOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pExitOptionNode != nullptr )
                {
                    int16_t const compiledNodeIdx = pExitOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_exitOptionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_dataSlotIdx = context.RegisterDataSlotNode( GetID() );
            pSettings->m_profileID = m_profileID;
        }
        return pSettings->m_nodeIdx;
    }

    ResourceTypeID SimulatedRagdollToolsNode::GetSlotResourceTypeID() const
    {
        return Physics::RagdollDefinition::GetStaticResourceTypeID();
    }
}