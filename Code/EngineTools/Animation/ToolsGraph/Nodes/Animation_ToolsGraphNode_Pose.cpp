#include "Animation_ToolsGraphNode_Pose.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Pose.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    ZeroPoseToolsNode::ZeroPoseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ZeroPoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ZeroPoseNode::Definition* pDefinition = nullptr;
        context.GetDefinition<ZeroPoseNode>( this, pDefinition );
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    ReferencePoseToolsNode::ReferencePoseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
    }

    int16_t ReferencePoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ReferencePoseNode::Definition* pDefinition = nullptr;
        context.GetDefinition<ReferencePoseNode>( this, pDefinition );
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    AnimationPoseToolsNode::AnimationPoseToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Time", GraphValueType::Float );
    }

    int16_t AnimationPoseToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationPoseNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<AnimationPoseNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pTimeParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pTimeParameterNode != nullptr )
            {
                auto compiledNodeIdx = pTimeParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_poseTimeValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            pDefinition->m_dataSlotIdx = context.RegisterResource( pData->m_animClip.GetResourceID() );
            pDefinition->m_inputTimeRemapRange = m_inputTimeRemapRange;
            pDefinition->m_userSpecifiedTime = ( pData->m_variationTimeValue >= 0 ) ? pData->m_variationTimeValue : m_fixedTimeValue;
            pDefinition->m_useFramesAsInput = m_useFramesAsInput;
        }
        return pDefinition->m_nodeIdx;
    }

    void AnimationPoseToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        if ( GetConnectedInputNode( 0 ) == nullptr )
        {
            auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
            auto pVariationData = GetResolvedVariationDataAs<Data>( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
            float const userSpecifiedTime = ( pVariationData->m_variationTimeValue >= 0 ) ? pVariationData->m_variationTimeValue : m_fixedTimeValue;

            BeginDrawInternalRegion( ctx );
            if ( m_useFramesAsInput )
            {
                ImGui::Text( "Frame: %2.f", userSpecifiedTime );
            }
            else
            {
                ImGui::Text( "Time: %2.f", userSpecifiedTime );
            }

            //-------------------------------------------------------------------------

            bool bIsTimeOverridenByVariation = ( pVariationData->m_variationTimeValue >= 0 );
            if ( bIsTimeOverridenByVariation )
            {
                ImGui::TextColored( Colors::Orange.ToFloat4(), "Overridden by Variation" );
            }

            EndDrawInternalRegion( ctx );
        }

        VariationDataToolsNode::DrawInfoText( ctx, pUserContext );
    }
}