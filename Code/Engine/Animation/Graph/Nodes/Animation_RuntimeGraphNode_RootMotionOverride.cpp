#include "Animation_RuntimeGraphNode_RootMotionOverride.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/Events/AnimationEvent_RootMotion.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void RootMotionOverrideNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<RootMotionOverrideNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_desiredHeadingVelocityNodeIdx, pNode->m_pDesiredHeadingVelocityNode );
        context.SetOptionalNodePtrFromIndex( m_desiredFacingDirectionNodeIdx, pNode->m_pDesiredFacingDirectionNode );
        context.SetOptionalNodePtrFromIndex( m_linearVelocityLimitNodeIdx, pNode->m_pLinearVelocityLimitNode );
        context.SetOptionalNodePtrFromIndex( m_angularVelocityLimitNodeIdx, pNode->m_pAngularVelocityLimitNode );
        PassthroughNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void RootMotionOverrideNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() && ( m_pDesiredHeadingVelocityNode != nullptr || m_pDesiredFacingDirectionNode != nullptr ) );

        PassthroughNode::InitializeInternal( context, initialTime );

        if ( m_pDesiredHeadingVelocityNode != nullptr )
        {
            m_pDesiredHeadingVelocityNode->Initialize( context );
        }

        if ( m_pDesiredFacingDirectionNode != nullptr )
        {
            m_pDesiredFacingDirectionNode->Initialize( context );
        }

        if ( m_pAngularVelocityLimitNode != nullptr )
        {
            m_pAngularVelocityLimitNode->Initialize( context );
        }

        if ( m_pLinearVelocityLimitNode != nullptr )
        {
            m_pLinearVelocityLimitNode->Initialize( context );
        }

        m_blendTime = 0.0f;
        m_desiredBlendDuration = 0.0f;
        m_blendState = BlendState::None;
        m_isFirstUpdate = true;
    }

    void RootMotionOverrideNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && ( m_pDesiredHeadingVelocityNode != nullptr || m_pDesiredFacingDirectionNode != nullptr ) );

        if ( m_pDesiredHeadingVelocityNode != nullptr )
        {
            m_pDesiredHeadingVelocityNode->Shutdown( context );
        }

        if ( m_pDesiredFacingDirectionNode != nullptr )
        {
            m_pDesiredFacingDirectionNode->Shutdown( context );
        }

        if ( m_pAngularVelocityLimitNode != nullptr )
        {
            m_pAngularVelocityLimitNode->Shutdown( context );
        }

        if ( m_pLinearVelocityLimitNode != nullptr )
        {
            m_pLinearVelocityLimitNode->Shutdown( context );
        }

        PassthroughNode::ShutdownInternal( context );
    }

    //-------------------------------------------------------------------------

    float RootMotionOverrideNode::CalculateOverrideWeight( GraphContext& context )
    {
        EE_ASSERT( GetSettings<RootMotionOverrideNode>()->m_overrideFlags.IsFlagSet( OverrideFlags::ListenForEvents ) );

        //-------------------------------------------------------------------------

        SampledEvent const* pSampledEvent = nullptr;

        for ( auto const& sampledEvent : context.m_sampledEventsBuffer )
        {
            if ( sampledEvent.IsIgnored() || !sampledEvent.IsFromActiveBranch() || sampledEvent.IsStateEvent() )
            {
                continue;
            }

            // Animation Event
            if ( auto pRMEvent = sampledEvent.TryGetEvent<RootMotionEvent>() )
            {
                // If we are not blending, check all events to find the one with the greatest weight to determine the blend duration
                if ( m_blendState == BlendState::None )
                {
                    if ( pSampledEvent == nullptr || sampledEvent.GetWeight() > pSampledEvent->GetWeight() )
                    {
                        pSampledEvent = &sampledEvent;
                    }
                }
                else // Just checking if we still have an event or not
                {
                    pSampledEvent = &sampledEvent;
                    break;
                }
            }
        }

        // Update blend state
        //-------------------------------------------------------------------------

        if ( pSampledEvent != nullptr )
        {
            // Start blend in
            if ( m_blendState == BlendState::FullyOut )
            {
                m_desiredBlendDuration = pSampledEvent->GetEvent<RootMotionEvent>()->GetBlendTime();
                EE_ASSERT( m_desiredBlendDuration >= 0.0f );

                // If we have an event on the first update, then skip the blend
                if ( m_isFirstUpdate )
                {
                    m_blendState = BlendState::FullyIn;
                    m_blendTime = m_desiredBlendDuration;
                }
                else
                {
                    m_blendState = BlendState::BlendingIn;
                    m_blendTime = 0.0f;
                }
            }
            // Cancel blend out
            else if( m_blendState == BlendState::BlendingOut )
            {
                float const currentPercentageThroughBlend = m_blendTime / m_desiredBlendDuration;

                m_blendState = BlendState::BlendingIn;
                m_desiredBlendDuration = pSampledEvent->GetEvent<RootMotionEvent>()->GetBlendTime();
                m_blendTime = currentPercentageThroughBlend * m_desiredBlendDuration;

                EE_ASSERT( m_desiredBlendDuration >= 0.0f );
            }
        }
        else // No event
        {
            // Start blend out
            if ( m_blendState == BlendState::FullyIn )
            {
                m_blendState = BlendState::BlendingOut;
                m_blendTime = 0.0f;
            }
            // Cancel blend in
            else if ( m_blendState == BlendState::BlendingIn )
            {
                m_blendState = BlendState::BlendingOut;
                m_blendTime = m_desiredBlendDuration - m_blendTime;
            }
        }

        // Update blend
        //-------------------------------------------------------------------------

        if ( m_blendState == BlendState::BlendingIn || m_blendState == BlendState::BlendingOut )
        {
            EE_ASSERT( m_desiredBlendDuration > 0.0f );
            m_blendTime += context.m_deltaTime.ToFloat();
            if ( m_blendTime > m_desiredBlendDuration )
            {
                m_blendTime = m_desiredBlendDuration;
                m_blendState = ( m_blendState == BlendState::BlendingIn ) ? BlendState::FullyIn : BlendState::FullyOut;
            }
        }

        // Calculate override weight
        //-------------------------------------------------------------------------

        float overrideWeight = 0.0f;

        switch ( m_blendState )
        {
            case BlendState::FullyOut:
            {
                overrideWeight = 1.0f;
            }
            break;

            case BlendState::BlendingIn:
            {
                overrideWeight = 1.0f - ( m_blendTime / m_desiredBlendDuration );
            }
            break;

            case BlendState::BlendingOut:
            {
                overrideWeight = m_blendTime / m_desiredBlendDuration;
            }
            break;

            case BlendState::FullyIn:
            {
                overrideWeight = 0.0f;
            }
            break;
        }

        //-------------------------------------------------------------------------

        return overrideWeight;
    }

    void RootMotionOverrideNode::ModifyRootMotion( GraphContext& context, GraphPoseNodeResult& nodeResult )
    {
        auto pSettings = GetSettings<RootMotionOverrideNode>();
        EE_ASSERT( context.IsValid() && pSettings != nullptr );

        //-------------------------------------------------------------------------

        Transform adjustedDisplacementDelta = nodeResult.m_rootMotionDelta;

        // Heading
        //-------------------------------------------------------------------------

        bool isHeadingModificationAllowed = m_pDesiredHeadingVelocityNode != nullptr && pSettings->m_overrideFlags.AreAnyFlagsSet( OverrideFlags::AllowHeadingX, OverrideFlags::AllowHeadingX, OverrideFlags::AllowHeadingX );
        if ( isHeadingModificationAllowed )
        {
            Float3 const desiredHeadingVelocity = m_pDesiredHeadingVelocityNode->GetValue<Vector>( context ).ToFloat3();

            // Override the request axes with the desired heading
            Float3 translation = nodeResult.m_rootMotionDelta.GetTranslation();
            translation.m_x = pSettings->m_overrideFlags.IsFlagSet( OverrideFlags::AllowHeadingX ) ? desiredHeadingVelocity.m_x * context.m_deltaTime : translation.m_x;
            translation.m_y = pSettings->m_overrideFlags.IsFlagSet( OverrideFlags::AllowHeadingY ) ? desiredHeadingVelocity.m_y * context.m_deltaTime : translation.m_y;
            translation.m_z = pSettings->m_overrideFlags.IsFlagSet( OverrideFlags::AllowHeadingZ ) ? desiredHeadingVelocity.m_z * context.m_deltaTime : translation.m_z;

            Vector vTranslation( translation );

            // Apply max linear velocity limit
            float maxLinearVelocity = pSettings->m_maxLinearVelocity;
            if ( m_pLinearVelocityLimitNode != nullptr )
            {
                maxLinearVelocity = Math::Abs( m_pLinearVelocityLimitNode->GetValue<float>( context ) * 100 );
            }

            if ( maxLinearVelocity >= 0 )
            {
                float const maxLinearValue = context.m_deltaTime * maxLinearVelocity;
                if ( vTranslation.GetLengthSquared3() > ( maxLinearValue * maxLinearValue ) )
                {
                    vTranslation.Normalize3();
                    vTranslation *= maxLinearValue;
                }
            }

            adjustedDisplacementDelta.SetTranslation( vTranslation );
        }

        // Facing
        //-------------------------------------------------------------------------

        bool isFacingModificationAllowed = ( m_pDesiredFacingDirectionNode != nullptr );
        if ( isFacingModificationAllowed )
        {
            Vector desiredFacingCS = m_pDesiredFacingDirectionNode->GetValue<Vector>( context );

            // Remove pitch facing if this is not allowed
            if ( !pSettings->m_overrideFlags.IsFlagSet( OverrideFlags::AllowFacingPitch ) )
            {
                desiredFacingCS = desiredFacingCS.Get2D();
            }

            // Adjust Facing
            if ( !desiredFacingCS.IsNearZero3() )
            {
                desiredFacingCS.Normalize3();

                // Get the total delta rotation between our current facing and the desired facing
                Quaternion deltaRotation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, desiredFacingCS, Vector::WorldUp );

                // Apply max angular velocity limit
                float maxAngularVelocity = pSettings->m_maxAngularVelocity;
                if ( m_pAngularVelocityLimitNode != nullptr )
                {
                    maxAngularVelocity = Math::Abs( Math::DegreesToRadians * m_pAngularVelocityLimitNode->GetValue<float>( context ) );
                }

                if ( maxAngularVelocity >= 0 )
                {
                    float const maxAngularValue = context.m_deltaTime * maxAngularVelocity;
                    float const desiredRotationAngle = (float) deltaRotation.ToAxisAngle().m_angle;
                    if (  desiredRotationAngle > maxAngularValue )
                    {
                        float const T = 1.0f - ( ( desiredRotationAngle - maxAngularValue ) / desiredRotationAngle );
                        deltaRotation = Quaternion::SLerp( Quaternion::Identity, deltaRotation, T );
                    }
                }

                adjustedDisplacementDelta.SetRotation( deltaRotation );
            }
        }

        // Perform Modification
        //-------------------------------------------------------------------------

        float const overrideWeight = pSettings->m_overrideFlags.IsFlagSet( OverrideFlags::ListenForEvents ) ? CalculateOverrideWeight( context ) : 1.0f;

        // Leave original root motion intact
        if ( overrideWeight == 0.0f )
        {
            // Do nothing
        }
        // Full override
        else if ( overrideWeight == 1.0f )
        {
            nodeResult.m_rootMotionDelta = adjustedDisplacementDelta;

            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordModification( GetNodeIndex(), nodeResult.m_rootMotionDelta );
            #endif
        }
        // Blend
        else if ( overrideWeight > 0.0f )
        {
            nodeResult.m_rootMotionDelta = Blender::BlendRootMotionDeltas( nodeResult.m_rootMotionDelta, adjustedDisplacementDelta, overrideWeight );

            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordModification( GetNodeIndex(), nodeResult.m_rootMotionDelta );
            #endif
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }

    GraphPoseNodeResult RootMotionOverrideNode::Update( GraphContext& context )
    {
        GraphPoseNodeResult Result = PassthroughNode::Update( context );

        // Always modify root motion even if child is invalid
        ModifyRootMotion( context, Result );
        m_isFirstUpdate = false;
        return Result;
    }

    GraphPoseNodeResult RootMotionOverrideNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        GraphPoseNodeResult Result = PassthroughNode::Update( context, updateRange );

        // Always modify root motion even if child is invalid
        ModifyRootMotion( context, Result );
        m_isFirstUpdate = false;
        return Result;
    }
}