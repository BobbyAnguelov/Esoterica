#include "Animation_RuntimeGraphNode_TargetSelector.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void TargetSelectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TargetSelectorNode>( context, options );

        for ( auto nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        context.SetNodePtrFromIndex( m_parameterNodeIdx, pNode->m_pParameterNode );
    }

    int32_t TargetSelectorNode::SelectOption( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        auto pDefinition = GetDefinition<TargetSelectorNode>();
        EE_ASSERT( !Math::IsNearZero( pDefinition->m_orientationScoreWeight + pDefinition->m_positionScoreWeight ) );

        int32_t selectedIdx = InvalidIndex;

        // Generate the set of options
        //-------------------------------------------------------------------------

        m_selectionOptions.clear();
        float maxDistance = 0.0f;

        bool const hasInitialTime = initialTime.m_eventIdx != 0 || !Math::IsNearZero( initialTime.m_percentageThrough );

        int32_t const numOptionNodes = (int32_t) m_optionNodes.size();
        for ( int32_t i = 0; i < numOptionNodes; i++ )
        {
            AnimationClip const* pClip = m_optionNodes[i]->IsValid() ? m_optionNodes[i]->GetAnimation() : nullptr;
            if ( pClip != nullptr && pClip->GetRootMotion().IsValid() )
            {
                Option& opt = m_selectionOptions.emplace_back();
                opt.m_optionIdx = i;

                if ( hasInitialTime )
                {
                    Percentage const startTime = pClip->GetSyncTrack().GetPercentageThrough( initialTime );
                    Transform const endTransform = pClip->GetRootMotion().GetDelta( startTime, Percentage( 1.0f ) );
                    opt.m_endOrientation = endTransform.GetRotation();
                    opt.m_endPoint = endTransform.GetTranslation();
                }
                else
                {
                    Transform const& endTransform = pClip->GetRootMotion().GetTotalDelta();
                    opt.m_endOrientation = endTransform.GetRotation();
                    opt.m_endPoint = endTransform.GetTranslation();
                }

                maxDistance = Math::Max( maxDistance, opt.m_endPoint.GetLength3() );
            }
            else
            {
                if ( !pDefinition->m_ignoreInvalidOptions )
                {
                    #if EE_DEVELOPMENT_TOOLS
                    context.LogError( GetNodeIndex(), "Invalid input detected for target selector" );
                    #endif
                    return selectedIdx;
                }
            }
        }

        // Scoring
        //-------------------------------------------------------------------------

        Quaternion const targetOrientation = m_targetTransform.GetRotation();
        Vector const targetPosition = m_targetTransform.GetTranslation();

        int32_t const numOptions = (int32_t) m_selectionOptions.size();
        for ( int32_t i = 0; i < numOptions; i++ )
        {
            Option& opt = m_selectionOptions[i];

            // Calculate Orientation Score
            //-------------------------------------------------------------------------

            if ( pDefinition->m_orientationScoreWeight > 0.0f )
            {
                Quaternion qDelta = Quaternion::Delta( opt.m_endOrientation, targetOrientation );
                qDelta.MakeShortestPath();
                opt.m_orientationScore = 1.0f - Math::Abs( qDelta.GetAngle().ToFloat() / Math::Pi );
                opt.m_orientationScore *= pDefinition->m_orientationScoreWeight;
            }

            // Calculate Position
            //-------------------------------------------------------------------------

            if ( pDefinition->m_positionScoreWeight > 0.0f && !Math::IsNearZero( maxDistance ) )
            {
                float const distance = opt.m_endPoint.GetDistance3( targetPosition );
                opt.m_positionScore = 1.0f - ( distance / maxDistance );
                opt.m_positionScore *= pDefinition->m_positionScoreWeight;
            }
        }

        // Pick the highest score
        //-------------------------------------------------------------------------

        float highestScore = -FLT_MAX;
        for ( int32_t i = 0; i < numOptions; i++ )
        {
            float const finalScore = m_selectionOptions[i].m_orientationScore + m_selectionOptions[i].m_positionScore;
            if ( finalScore > highestScore )
            {
                selectedIdx = m_selectionOptions[i].m_optionIdx;
                highestScore = finalScore;
            }
        }

        #if EE_DEVELOPMENT_TOOLS
        m_selectionWorldTransform = context.m_worldTransform;
        #endif

        return selectedIdx;
    }

    void TargetSelectorNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        AnimationClipReferenceNode::InitializeInternal( context, initialTime );

        Target const& target = m_pParameterNode->GetValue<Target>( context );
        if ( !target.IsTargetSet() || target.IsBoneTarget() )
        {
            m_selectedOptionIdx = InvalidIndex;

            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Invalid target provided to target selector!" );
            #endif
        }
        else // Select option
        {
            m_targetTransform = target.GetTransform();
            if ( GetDefinition<TargetSelectorNode>()->m_isWorldSpaceTarget )
            {
                m_targetTransform = m_targetTransform * context.m_worldTransformInverse;
            }

            m_selectedOptionIdx = SelectOption( context, initialTime );
            if ( m_selectedOptionIdx != InvalidIndex )
            {
                m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];
                m_pSelectedNode->Initialize( context, initialTime );

                if ( m_pSelectedNode->IsValid() )
                {
                    m_duration = m_pSelectedNode->GetDuration();
                    m_previousTime = m_pSelectedNode->GetPreviousTime();
                    m_currentTime = m_pSelectedNode->GetCurrentTime();
                }
                else
                {
                    m_pSelectedNode->Shutdown( context );
                    m_pSelectedNode = nullptr;
                    m_selectedOptionIdx = InvalidIndex;
                }
            }
            else
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Failed to select a valid option!" );
                #endif
            }
        }
    }

    void TargetSelectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        AnimationClipReferenceNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult TargetSelectorNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;
        if ( IsValid() )
        {
            MarkNodeActive( context );

            // Copy node instance data, this node acts like a passthrough
            result = m_pSelectedNode->Update( context, pUpdateRange );
            m_duration = m_pSelectedNode->GetDuration();
            m_previousTime = m_pSelectedNode->GetPreviousTime();
            m_currentTime = m_pSelectedNode->GetCurrentTime();
            EE_ASSERT( context.GetSampledEventsBuffer()->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    bool TargetSelectorNode::HasAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->HasAnimation();
        }

        return false;
    }

    AnimationClip const* TargetSelectorNode::GetAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->GetAnimation();
        }

        return nullptr;
    }

    void TargetSelectorNode::DisableRootMotionSampling()
    {
        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->DisableRootMotionSampling();
        }
    }

    bool TargetSelectorNode::IsLooping() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->IsLooping();
        }

        return false;
    }

    void TargetSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        AnimationClipReferenceNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool TargetSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if( !AnimationClipReferenceNode::RestoreGraphState( inState ) )
        {
            return false;
        }

        inState.ReadValue( m_selectedOptionIdx );

        if ( m_selectedOptionIdx != InvalidIndex )
        {
            m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];
        }
        else
        {
            m_pSelectedNode = nullptr;
        }

        return true;
    }

    #if EE_DEVELOPMENT_TOOLS
    void TargetSelectorNode::DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx )
    {
        Transform worldTargetTransform = m_targetTransform * m_selectionWorldTransform;

        drawCtx.DrawPoint( worldTargetTransform.GetTranslation(), Colors::Yellow, 5.0f );
        drawCtx.DrawArrow( worldTargetTransform.GetTranslation(), worldTargetTransform.GetTranslation() + ( worldTargetTransform.GetForwardVector() * 0.25f ), Colors::Yellow, 2.0f );
        drawCtx.DrawText3D( worldTargetTransform.GetTranslation(), "Target", Colors::White);

        //-------------------------------------------------------------------------

        for ( Option const& opt : m_selectionOptions )
        {
            float const finalScore = opt.m_positionScore + opt.m_orientationScore;
            Transform optionTransform = Transform( opt.m_endOrientation, opt.m_endPoint ) * m_selectionWorldTransform;

            drawCtx.DrawPoint( optionTransform.GetTranslation(), Color::EvaluateRedGreenGradient( finalScore ), 5.0f );
            drawCtx.DrawArrow( optionTransform.GetTranslation(), optionTransform.GetTranslation() + ( optionTransform.GetForwardVector() * 0.25f ), Colors::HotPink, 2.0f );
            
            InlineString str( InlineString::CtorSprintf(), "%s\n%2.f - Pos: %.2f, Orient: %.2f", "anim", finalScore, opt.m_positionScore, opt.m_orientationScore);
            drawCtx.DrawText3D( optionTransform.GetTranslation(), str.c_str(), Colors::White );
        }
    }
    #endif
}