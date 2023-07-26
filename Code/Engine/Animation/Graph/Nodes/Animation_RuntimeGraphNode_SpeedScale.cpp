#include "Animation_RuntimeGraphNode_SpeedScale.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void SpeedScaleNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SpeedScaleNode>( context, options );
        context.SetNodePtrFromIndex( m_scaleValueNodeIdx, pNode->m_pScaleValueNode );
        PassthroughNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void SpeedScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pScaleValueNode != nullptr );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pScaleValueNode->Initialize( context );

        auto pSettings = GetSettings<SpeedScaleNode>();
        m_blendWeight = ( pSettings->m_blendInTime > 0.0f ) ? 0.0f : 1.0f;
    }

    void SpeedScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pScaleValueNode != nullptr );
        m_pScaleValueNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult SpeedScaleNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        MarkNodeActive( context );

        GraphPoseNodeResult result;

        // Synchronized Update
        if ( pUpdateRange != nullptr )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Attempting to run a speed scale node in a synchronized manner, this is an invalid operation!" );
            #endif

            result = PassthroughNode::Update( context, pUpdateRange );
        }
        else // Unsynchronized Update
        {
            // Record old delta time
            auto const deltaTime = context.m_deltaTime;

            // Adjust the delta time for the child node
            //-------------------------------------------------------------------------

            float actualDuration = 0.0f;
            if ( m_pChildNode->IsValid() )
            {
                // Get expected scale
                float speedScale = m_pScaleValueNode->GetValue<float>( context );
                if ( speedScale < 0.0f )
                {
                    speedScale = 0.0f;

                    #if EE_DEVELOPMENT_TOOLS
                    context.LogWarning( GetNodeIndex(), "Negative speed scale is not supported!" );
                    #endif
                }

                // Perform blend
                auto pSettings = GetSettings<SpeedScaleNode>();
                if ( pSettings->m_blendInTime > 0.0f && m_blendWeight < 1.0f )
                {
                    float const blendWeightDelta = context.m_deltaTime / pSettings->m_blendInTime;
                    m_blendWeight = Math::Clamp( m_blendWeight + blendWeightDelta, 0.0f, 1.0f );
                    speedScale = Math::Lerp( 1.0f, speedScale, m_blendWeight );
                }

                // Zero scale is equivalent to a single pose animation
                if ( Math::IsNearZero( speedScale ) )
                {
                    context.m_deltaTime = 0.0f;
                    actualDuration = 0.0f;
                }
                else // Apply scale
                {
                    context.m_deltaTime *= speedScale;
                    actualDuration = m_pChildNode->GetDuration() / speedScale;
                }
            }

            // Update the child node
            //-------------------------------------------------------------------------

            result = PassthroughNode::Update( context );

            // Override node values
            //-------------------------------------------------------------------------

            if ( m_pChildNode->IsValid() )
            {
                m_duration = actualDuration;
            }

            // Reset the delta time
            //-------------------------------------------------------------------------

            context.m_deltaTime = deltaTime;
        }

        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void SpeedScaleNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_blendWeight );
    }

    void SpeedScaleNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_blendWeight );
    }
    #endif

    //-------------------------------------------------------------------------

    void DurationScaleNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<DurationScaleNode>( context, options );
        context.SetNodePtrFromIndex( m_durationValueNodeIdx, pNode->m_pDurationValueNode );
        PassthroughNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void DurationScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pDurationValueNode != nullptr );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pDurationValueNode->Initialize( context );
    }

    void DurationScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pDurationValueNode != nullptr );
        m_pDurationValueNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult DurationScaleNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        MarkNodeActive( context );

        GraphPoseNodeResult result;

        if( pUpdateRange )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Attempting to run a speed scale node in a synchronized manner, this is an invalid operation!" );
            #endif

            result = PassthroughNode::Update( context, pUpdateRange );
        }
        else
        {
            // Record old delta time
            auto const deltaTime = context.m_deltaTime;

            // Adjust the delta time for the child node
            //-------------------------------------------------------------------------

            float actualDuration = 0.0f;
            if ( m_pChildNode->IsValid() )
            {
                // Get expected scale
                float desiredDuration = m_pDurationValueNode->GetValue<float>( context );
                if ( desiredDuration < 0.0f )
                {
                    desiredDuration = 0.0f;

                    #if EE_DEVELOPMENT_TOOLS
                    context.LogWarning( GetNodeIndex(), "Negative duration is not supported!" );
                    #endif
                }

                // Zero length is equivalent to a single pose animation
                if ( Math::IsNearZero( desiredDuration ) )
                {
                    context.m_deltaTime = 0.0f;
                    actualDuration = 0.0f;
                }
                else // Apply scale
                {
                    float const scaleMultiplier = desiredDuration * m_pChildNode->GetDuration();
                    context.m_deltaTime *= scaleMultiplier;
                    actualDuration = desiredDuration;
                }
            }

            // Update the child node
            //-------------------------------------------------------------------------

            result = PassthroughNode::Update( context );

            // Override node values
            //-------------------------------------------------------------------------

            if ( m_pChildNode->IsValid() )
            {
                m_duration = actualDuration;
            }

            // Reset the delta time
            //-------------------------------------------------------------------------

            context.m_deltaTime = deltaTime;
        }

        return result;
    }

    //-------------------------------------------------------------------------

    void VelocityBasedSpeedScaleNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VelocityBasedSpeedScaleNode>( context, options );
        context.SetNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
        context.SetNodePtrFromIndex( m_desiredVelocityValueNodeIdx, pNode->m_pDesiredVelocityValueNode );
    }

    void VelocityBasedSpeedScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pDesiredVelocityValueNode != nullptr );
        PoseNode::InitializeInternal( context, initialTime );
        m_pDesiredVelocityValueNode->Initialize( context );

        auto pSettings = GetSettings<SpeedScaleNode>();
        m_blendWeight = ( pSettings->m_blendInTime > 0.0f ) ? 0.0f : 1.0f;

        //-------------------------------------------------------------------------

        m_previousTime = m_currentTime = 0.0f;
        m_duration = 0.0f;

        //-------------------------------------------------------------------------

        m_pChildNode->Initialize( context, initialTime );

        if ( m_pChildNode->IsValid() )
        {
            m_duration = m_pChildNode->GetDuration();
            m_previousTime = m_pChildNode->GetPreviousTime();
            m_currentTime = m_pChildNode->GetCurrentTime();
        }
    }

    void VelocityBasedSpeedScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pDesiredVelocityValueNode != nullptr );
        m_pChildNode->Shutdown( context );
        m_pDesiredVelocityValueNode->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    SyncTrack const& VelocityBasedSpeedScaleNode::GetSyncTrack() const
    {
        if ( IsValid() )
        {
            return m_pChildNode->GetSyncTrack();
        }
        else
        {
            return SyncTrack::s_defaultTrack;
        }
    }

    GraphPoseNodeResult VelocityBasedSpeedScaleNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        MarkNodeActive( context );

        GraphPoseNodeResult result;

        //-------------------------------------------------------------------------

        if ( !IsValid() )
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            return result;
        }

        //-------------------------------------------------------------------------

        // Synchronized Update
        if ( pUpdateRange != nullptr )
        {

            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Attempting to run a speed scale node in a synchronized manner, this is an invalid operation!" );
            #endif

            // Forward child node results
            result = m_pChildNode->Update( context, pUpdateRange );
            m_duration = m_pChildNode->GetDuration();
            m_previousTime = m_pChildNode->GetPreviousTime();
            m_currentTime = m_pChildNode->GetCurrentTime();
        }
        else // Unsynchronized Update
        {
            // Record old delta time
            auto const deltaTime = context.m_deltaTime;

            // Adjust the delta time for the child node
            //-------------------------------------------------------------------------

            float actualDuration = 0.0f;

            // Get the desired velocity
            float desiredVelocity = m_pDesiredVelocityValueNode->GetValue<float>( context );
            if ( desiredVelocity < 0.0f )
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Requesting a negative velocity is not supported!" );
                #endif

                desiredVelocity = 0.0f;
            }

            // Calculate multiplier
            //-------------------------------------------------------------------------

            auto speedMultiplier = 1.0f;
            if ( Math::IsNearZero( desiredVelocity ) )
            {
                speedMultiplier = 0.0f;
            }
            else
            {
                float const averageVelocity = m_pChildNode->GetAnimation()->GetAverageLinearVelocity();
                speedMultiplier = desiredVelocity / averageVelocity;
                EE_ASSERT( speedMultiplier >= 0.0f );
            }

            // Adjust delta time
            //-------------------------------------------------------------------------

            // Zero length is equivalent to a single pose animation
            if ( Math::IsNearZero( speedMultiplier ) )
            {
                context.m_deltaTime = 0.0f;
                actualDuration = 0.0f;
            }
            else // Apply scale
            {
                context.m_deltaTime *= speedMultiplier;
                actualDuration = m_pChildNode->GetDuration() / speedMultiplier;
            }

            // Blend
            //-------------------------------------------------------------------------

            auto pSettings = GetSettings<VelocityBasedSpeedScaleNode>();
            if ( pSettings->m_blendInTime > 0.0f && m_blendWeight < 1.0f )
            {
                float const blendWeightDelta = context.m_deltaTime / pSettings->m_blendInTime;
                m_blendWeight = Math::Clamp( m_blendWeight + blendWeightDelta, 0.0f, 1.0f );
                speedMultiplier = Math::Lerp( 1.0f, speedMultiplier, m_blendWeight );
            }

            context.m_deltaTime *= speedMultiplier;
            actualDuration = m_pChildNode->GetDuration() / speedMultiplier;

            // Update the child node
            //-------------------------------------------------------------------------

            // Forward child node results
            result = m_pChildNode->Update( context );
            m_duration = m_pChildNode->GetDuration();
            m_previousTime = m_pChildNode->GetPreviousTime();
            m_currentTime = m_pChildNode->GetCurrentTime();

            // Override node values
            //-------------------------------------------------------------------------

            if ( m_pChildNode->IsValid() )
            {
                m_duration = actualDuration;
            }

            // Reset the time delta
            //-------------------------------------------------------------------------

            context.m_deltaTime = deltaTime;
        }

        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void VelocityBasedSpeedScaleNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_blendWeight );
    }

    void VelocityBasedSpeedScaleNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_blendWeight );
    }
    #endif
}