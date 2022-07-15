#include "Animation_EditorGraphNode_Pose.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Pose.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ZeroPoseEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ZeroPoseEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ZeroPoseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ZeroPoseNode>( this, pSettings );
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void ReferencePoseEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ReferencePoseEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencePoseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ReferencePoseNode>( this, pSettings );
        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void AnimationPoseEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotEditorNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Time", GraphValueType::Float );
    }

    int16_t AnimationPoseEditorNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationPoseNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AnimationPoseNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pTimeParameterNode = GetConnectedInputNode<EditorGraphNode>( 0 );
            if ( pTimeParameterNode != nullptr )
            {
                auto compiledNodeIdx = pTimeParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_poseTimeValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected time parameter pin on animation pose node!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pSettings->m_dataSlotIndex = context.RegisterSlotNode( GetID() );
            pSettings->m_inputTimeRemapRange = m_inputTimeRemapRange;
        }
        return pSettings->m_nodeIdx;
    }
}