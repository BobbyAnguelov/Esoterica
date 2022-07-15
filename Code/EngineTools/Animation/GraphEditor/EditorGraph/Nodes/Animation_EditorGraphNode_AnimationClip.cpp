#include "Animation_EditorGraphNode_AnimationClip.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_AnimationClip.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void AnimationClipEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotEditorNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Play In Reverse", GraphValueType::Bool );
    }

    int16_t AnimationClipEditorNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationClipNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AnimationClipNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pShouldPlayInReverseNodeNode = GetConnectedInputNode<EditorGraphNode>( 0 );
            if ( pShouldPlayInReverseNodeNode != nullptr )
            {
                auto compiledNodeIdx = pShouldPlayInReverseNodeNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_playInReverseValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_dataSlotIdx = context.RegisterSlotNode( GetID() );
            pSettings->m_sampleRootMotion = m_sampleRootMotion;
            pSettings->m_allowLooping = m_allowLooping;
        }
        return pSettings->m_nodeIdx;
    }

}