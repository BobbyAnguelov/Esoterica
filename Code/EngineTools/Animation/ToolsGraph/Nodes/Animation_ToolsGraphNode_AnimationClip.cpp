#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void AnimationClipToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Play In Reverse", GraphValueType::Bool );
    }

    int16_t AnimationClipToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationClipNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AnimationClipNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pShouldPlayInReverseNodeNode = GetConnectedInputNode<FlowToolsNode>( 0 );
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

            pSettings->m_dataSlotIdx = context.RegisterDataSlotNode( GetID() );
            pSettings->m_sampleRootMotion = m_sampleRootMotion;
            pSettings->m_allowLooping = m_allowLooping;
        }
        return pSettings->m_nodeIdx;
    }

}