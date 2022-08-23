#include "Animation_ToolsGraphNode_Ragdoll.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Ragdoll.h"
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

    EE::ResourceTypeID PoweredRagdollToolsNode::GetSlotResourceTypeID() const
    {
        return Physics::RagdollDefinition::GetStaticResourceTypeID();
    }
}