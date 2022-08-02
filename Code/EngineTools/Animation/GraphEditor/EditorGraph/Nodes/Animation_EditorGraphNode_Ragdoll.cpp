#include "Animation_EditorGraphNode_Ragdoll.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Ragdoll.h"
#include "Engine/Physics/PhysicsRagdoll.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void PoweredRagdollEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotEditorNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Pose, true );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Physics Blend Weight", GraphValueType::Float );
    }

    int16_t PoweredRagdollEditorNode::Compile( GraphCompilationContext& context ) const
    {
        PoweredRagdollNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<PoweredRagdollNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<EditorGraphNode>( 0 );
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

            pInputNode = GetConnectedInputNode<EditorGraphNode>( 1 );
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

    EE::ResourceTypeID PoweredRagdollEditorNode::GetSlotResourceTypeID() const
    {
        return Physics::RagdollDefinition::GetStaticResourceTypeID();
    }
}