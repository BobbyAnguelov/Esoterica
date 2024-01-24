#include "Animation_ToolsGraphNode_Transition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "../../../Core/PropertyGrid/PropertyGridTypeEditingRules.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    TransitionToolsNode::TransitionToolsNode()
        : FlowToolsNode()
    {
        CreateInputPin( "Condition", GraphValueType::Bool );
        CreateInputPin( "Duration Override", GraphValueType::Float );
        CreateInputPin( "Sync Event Override", GraphValueType::Float );
        CreateInputPin( "Start Bone Mask", GraphValueType::BoneMask );
        CreateInputPin( "Target Sync ID", GraphValueType::ID );
    }

    void TransitionToolsNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenameable() );
        VisualGraph::ScopedNodeModification const snm( this );
        m_name = newName;
    }

    void TransitionToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        BeginDrawInternalRegion( ctx );

        //-------------------------------------------------------------------------

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

        //-------------------------------------------------------------------------

        ImGui::Text( "Duration: %.2fs", m_duration.ToFloat() );

        if ( m_clampDurationToSource )
        {
            ImGui::Text( "Clamped To Source" );
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
        }

        ImGui::Text( "Sync Offset: %.2f", m_syncEventOffset );

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

    //-------------------------------------------------------------------------

    class TransitionEditingRules : public PG::TTypeEditingRules<TransitionToolsNode>
    {
        using PG::TTypeEditingRules<TransitionToolsNode>::TTypeEditingRules;

        virtual bool IsReadOnly( StringID const& propertyID ) override
        {
            return false;
        }

        virtual bool IsHidden( StringID const& propertyID ) override
        {
            StringID const boneMaskBlendPropertyID( "m_boneMaskBlendInTimePercentage" );
            if ( propertyID == boneMaskBlendPropertyID )
            {
                return m_pTypeInstance->GetConnectedInputNode<FlowToolsNode>( 3 ) == nullptr;
            }

            return false;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( TransitionEditingRulesFactory, TransitionToolsNode, TransitionEditingRules );

    //-------------------------------------------------------------------------

    void TransitionConduitToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::TransitionConduit::Initialize( pParent );
        SetSecondaryGraph( EE::New<FlowGraph>( GraphType::TransitionTree ) );
    }

    bool TransitionConduitToolsNode::HasTransitions() const
    {
        return !GetSecondaryGraph()->FindAllNodesOfType<TransitionToolsNode>().empty();
    }

    Color TransitionConduitToolsNode::GetColor( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::NodeVisualState visualState ) const
    {
        // Is this an blocked transition
        if ( visualState == VisualGraph::NodeVisualState::None && !HasTransitions() )
        {
            return s_connectionColorInvalid;
        }

        // Is this transition active?
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() && isAnyChildActive )
        {
            return s_connectionColorValid;
        }

        //-------------------------------------------------------------------------

        return VisualGraph::SM::TransitionConduit::GetColor( ctx, pUserContext, visualState );
    }

    void TransitionConduitToolsNode::PreDrawUpdate( VisualGraph::UserContext* pUserContext )
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
                    auto pTransitionNode = static_cast<GraphNodes::TransitionNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
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