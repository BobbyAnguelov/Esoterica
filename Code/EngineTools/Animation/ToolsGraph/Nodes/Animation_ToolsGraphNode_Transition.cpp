#include "Animation_ToolsGraphNode_Transition.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "EngineTools/NodeGraph/NodeGraph_Style.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TransitionToolsNode::TransitionToolsNode()
        : ResultToolsNode()
    {
        CreateInputPin( "Condition", GraphValueType::Bool );
        CreateInputPin( "Duration Override", GraphValueType::Float );
        CreateInputPin( "Time Offset Override", GraphValueType::Float );
        CreateInputPin( "Start Bone Mask", GraphValueType::BoneMask );
        CreateInputPin( "Target Sync ID", GraphValueType::ID );
    }

    void TransitionToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        BeginDrawInternalRegion( ctx );

        //-------------------------------------------------------------------------

        ImGui::Text( "Duration: %.2fs", m_duration.ToFloat() );

        bool const isInstantTransition = ( m_duration == 0.0f );
        if ( !isInstantTransition )
        {
            if ( m_clampDurationToSource )
            {
                ImGui::Text( "Clamped To Source" );
            }

            switch ( m_rootMotionBlend )
            {
                case RootMotionBlendMode::Blend:
                ImGui::Text( "Blend Root Motion" );
                break;

                case RootMotionBlendMode::Additive:
                ImGui::Text( "Blend Root Motion (Additive)" );
                break;

                case RootMotionBlendMode::IgnoreSource:
                ImGui::Text( "Ignore Source Root Motion" );
                break;

                case RootMotionBlendMode::IgnoreTarget:
                ImGui::Text( "Ignore Target Root Motion" );
                break;
            }
        }

        //-------------------------------------------------------------------------

        switch ( m_timeMatchMode )
        {
            case TimeMatchMode::None:
            break;

            case TimeMatchMode::Synchronized:
            {
                ImGui::Text( "Synchronized" );
            }
            break;

            case TimeMatchMode::MatchSourceSyncEventIndex:
            {
                ImGui::Text( "Match Sync Idx" );
            }
            break;

            case TimeMatchMode::MatchSourceSyncEventIndexAndPercentage:
            {
                ImGui::Text( "Match Sync Idx and %%" );
            }
            break;

            case TimeMatchMode::MatchSyncEventID:
            {
                ImGui::Text( "Match Sync ID" );
            }
            break;

            case TimeMatchMode::MatchSyncEventIDAndPercentage:
            {
                ImGui::Text( "Match Sync ID and %%" );
            }
            break;

            case TimeMatchMode::MatchClosestSyncEventID:
            {
                ImGui::Text( "Match Closest Sync ID" );
            }
            break;

            case TimeMatchMode::MatchClosestSyncEventIDAndPercentage:
            {
                ImGui::Text( "Match Closest Sync ID and %%" );
            }
            break;

            case TimeMatchMode::MatchSourceSyncEventPercentage:
            {
                ImGui::Text( "Match Sync %% Only" );
            }
            break;

            case TimeMatchMode::MatchTimeInSeconds:
            {
                ImGui::Text( "Match Time In Seconds" );
            }
            break;

            case TimeMatchMode::OffsetTimeInSeconds:
            {
                ImGui::Text( "Offset Time In Seconds" );
            }
            break;
        }

        if ( m_timeMatchMode == TimeMatchMode::MatchTimeInSeconds || m_timeMatchMode == TimeMatchMode::OffsetTimeInSeconds )
        {
            ImGui::Text( "Time Offset: %.2fs", m_timeOffset );
        }
        else
        {
            ImGui::Text( "Sync Offset: %.2f", m_timeOffset );
        }

        //-------------------------------------------------------------------------

        if ( m_canBeForced )
        {
            ImGui::Text( "Forced" );
        }

        EndDrawInternalRegion( ctx );
    }

    Color TransitionToolsNode::GetTitleBarColor() const
    {
        return m_canBeForced ? Colors::Salmon : FlowToolsNode::GetTitleBarColor();
    }

    Color TransitionToolsNode::GetHighlightOutlineColor( NodeGraph::UserContext* pUserContext ) const
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            if ( !m_canBeForced )
            {
                auto const endStateRuntimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetEndStateID() );
                if ( endStateRuntimeNodeIdx != InvalidIndex )
                {
                    // If the target state is currently active and we're not allowed to be forced
                    if ( pGraphNodeContext->IsNodeActive( endStateRuntimeNodeIdx ) )
                    {
                        return NodeGraph::Style::s_invalidBorderColor;
                    }
                }
            }

            // Some nodes dont have runtime representations
            auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                auto pDebugNode = pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx );
                if ( pDebugNode->IsInitialized() && !pDebugNode->IsValid() )
                {
                    return NodeGraph::Style::s_invalidBorderColor;
                }
                else if ( pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
                {
                    return NodeGraph::Style::s_activeBorderColor;
                }
            }
        }

        return ResultToolsNode::GetHighlightOutlineColor( pUserContext );
    }

    UUID TransitionToolsNode::GetEndStateID() const
    {
        return Cast<TransitionConduitToolsNode>( GetParentNode() )->GetEndStateID();
    }

    //-------------------------------------------------------------------------

    class TransitionEditingRules : public PG::TTypeEditingRules<TransitionToolsNode>
    {
        using PG::TTypeEditingRules<TransitionToolsNode>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            static StringID const boneMaskBlendPropertyID( "m_boneMaskBlendInTimePercentage" );
            if ( propertyID == boneMaskBlendPropertyID )
            {
                if( m_pTypeInstance->GetConnectedInputNode<FlowToolsNode>( 3 ) == nullptr )
                {
                    return HiddenState::Hidden;
                }
            }

            //-------------------------------------------------------------------------

            static StringID const clampPropertyID( "m_clampDurationToSource" );
            static StringID const easingPropertyID( "m_blendWeightEasing" );
            static StringID const rootMotionPropertyID( "m_rootMotionBlend" );

            if ( propertyID == clampPropertyID || propertyID == easingPropertyID || propertyID == boneMaskBlendPropertyID || propertyID == rootMotionPropertyID )
            {
                return ( m_pTypeInstance->m_duration <= 0.0f ) ? HiddenState::Hidden : HiddenState::Visible;
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( TransitionToolsNode, TransitionEditingRules );

    //-------------------------------------------------------------------------

    TransitionConduitToolsNode::TransitionConduitToolsNode()
        : NodeGraph::TransitionConduitNode()
    {
        CreateSecondaryGraph<FlowGraph>( GraphType::TransitionConduit );
    }

    TransitionConduitToolsNode::TransitionConduitToolsNode( NodeGraph::StateNode const* pStartState, NodeGraph::StateNode const* pEndState )
        : NodeGraph::TransitionConduitNode( pStartState, pEndState )
    {
        CreateSecondaryGraph<FlowGraph>( GraphType::TransitionConduit );
    }

    bool TransitionConduitToolsNode::HasTransitions() const
    {
        return !GetSecondaryGraph()->FindAllNodesOfType<TransitionToolsNode>().empty();
    }

    Color TransitionConduitToolsNode::GetConduitColor( NodeGraph::UserContext* pUserContext ) const
    {
        // No Transitions
        if ( !HasTransitions() )
        {
            return NodeGraph::Style::s_connectionColorInvalid;
        }

        // Is this transition active?
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() && isAnyChildActive )
        {
            return NodeGraph::Style::s_connectionColorValid;
        }

        //-------------------------------------------------------------------------

        return Colors::Transparent;
    }

    void TransitionConduitToolsNode::PreDrawUpdate( NodeGraph::UserContext* pUserContext )
    {
        isAnyChildActive = false;
        m_transitionProgress = 0.0f;

        //-------------------------------------------------------------------------

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        if ( isPreviewing )
        {
            auto childTransitions = GetSecondaryGraph()->FindAllNodesOfType<TransitionToolsNode>();
            for ( auto pTransition : childTransitions )
            {
                auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( pTransition->GetID() );
                if ( runtimeNodeIdx != InvalidIndex && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
                {
                    float progress = 0.0f;
                    auto pTransitionNode = static_cast<TransitionNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
                    if ( pTransitionNode->IsInitialized() )
                    {
                        progress = pTransitionNode->GetProgressPercentage();
                    }

                    m_transitionProgress = Percentage( Math::Max( progress, 0.001f ) );
                    isAnyChildActive = true;
                    break;
                }
            }
        }
    }
}