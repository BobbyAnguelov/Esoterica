#include "Animation_RuntimeGraphNode_SpeedScale.h"
#include "System/Log.h"
#include "Engine/Animation/AnimationClip.h"

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
        m_blendWeight = 1.0f;
    }

    void SpeedScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pScaleValueNode != nullptr );
        m_pScaleValueNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult SpeedScaleNode::Update( GraphContext& context )
    {
        // Record old delta time
        auto const deltaTime = context.m_deltaTime;

        // Adjust the delta time for the child node
        //-------------------------------------------------------------------------

        if ( m_pChildNode->IsValid() )
        {
            auto speedScale = 1.0f;
            if ( m_pScaleValueNode != nullptr )
            {
                speedScale = GetSpeedScale( context );

                if ( m_blendWeight < 1.0f )
                {
                    auto pSettings = GetSettings<SpeedScaleNode>();
                    EE_ASSERT( pSettings->m_blendTime > 0.0f );
                    float const blendWeightDelta = context.m_deltaTime / pSettings->m_blendTime;
                    m_blendWeight = Math::Clamp( m_blendWeight + blendWeightDelta, 0.0f, 1.0f );
                    speedScale = Math::Lerp( 1.0f, speedScale, m_blendWeight );
                }

                if ( Math::IsNearZero( speedScale ) )
                {
                    context.m_deltaTime = 0.0f;
                    m_duration = 0.0f;
                }
                else
                {
                    context.m_deltaTime *= speedScale;
                    m_duration = m_pChildNode->GetDuration() / speedScale;
                }
            }
        }

        // Update the child node
        //-------------------------------------------------------------------------

        GraphPoseNodeResult result = PassthroughNode::Update( context );

        // Reset the delta time
        //-------------------------------------------------------------------------

        context.m_deltaTime = deltaTime;
        return result;
    }

    GraphPoseNodeResult SpeedScaleNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        #if EE_DEVELOPMENT_TOOLS
        context.LogWarning( GetNodeIndex(), "Attempting to run a speed scale node in a synchronized manner, this is an invalid operation!" );
        #endif

        m_blendWeight = ( Math::IsNearZero( GetSettings<SpeedScaleNode>()->m_blendTime ) ) ? 1.0f : 0.0f;
        return PassthroughNode::Update( context, updateRange );
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
        m_blendWeight = 1.0f;

        //-------------------------------------------------------------------------

        m_previousTime = m_currentTime = 0.0f;
        m_duration = 1.0f;

        //-------------------------------------------------------------------------

        m_pChildNode->Initialize(context, initialTime);

        if (m_pChildNode->IsValid())
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
        m_pChildNode->Shutdown(context);
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

    GraphPoseNodeResult VelocityBasedSpeedScaleNode::Update( GraphContext& context )
    {
        // Record old delta time
        auto const deltaTime = context.m_deltaTime;

        // Adjust the delta time for the child node
        //-------------------------------------------------------------------------

        if ( m_pChildNode->IsValid() )
        {
            auto speedMultiplier = 1.0f;
            if ( m_pDesiredVelocityValueNode != nullptr )
            {
                float const desiredVelocity = m_pDesiredVelocityValueNode->GetValue<float>( context );
                if ( desiredVelocity > 0.0f )
                {
                    auto const pReferenceNode = static_cast<AnimationClipReferenceNode*>( m_pChildNode );
                    EE_ASSERT( pReferenceNode->IsValid() );

                    float const averageVelocity = pReferenceNode->GetAnimation()->GetAverageLinearVelocity();
                    if ( !Math::IsNearZero( averageVelocity ) )
                    {
                        speedMultiplier = desiredVelocity / averageVelocity;
                    }
                }
                else
                {
                    #if EE_DEVELOPMENT_TOOLS
                    context.LogWarning( GetNodeIndex(), "Requesting a negative velocity is not supported!" );
                    #endif
                }

                //-------------------------------------------------------------------------

                if ( m_blendWeight < 1.0f )
                {
                    auto pSettings = GetSettings<VelocityBasedSpeedScaleNode>();
                    EE_ASSERT( pSettings->m_blendTime > 0.0f );
                    float const BlendWeightDelta = context.m_deltaTime / pSettings->m_blendTime;
                    m_blendWeight = Math::Clamp( m_blendWeight + BlendWeightDelta, 0.0f, 1.0f );
                    speedMultiplier = Math::Lerp( 1.0f, speedMultiplier, m_blendWeight );
                }

                context.m_deltaTime *= speedMultiplier;
                m_duration = m_pChildNode->GetDuration() / speedMultiplier;
            }
        }

        // Update the child node
        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;

        // Forward child node results
        if ( IsValid() )
        {
            result = m_pChildNode->Update( context );
            m_duration = m_pChildNode->GetDuration();
            m_previousTime = m_pChildNode->GetPreviousTime();
            m_currentTime = m_pChildNode->GetCurrentTime();
        }
        else
        {
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
        }

        // Reset the time delta
        //-------------------------------------------------------------------------

        context.m_deltaTime = deltaTime;
        return result;
    }

    GraphPoseNodeResult VelocityBasedSpeedScaleNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        m_blendWeight = ( Math::IsNearZero( GetSettings<VelocityBasedSpeedScaleNode>()->m_blendTime ) ) ? 1.0f : 0.0f;
        
        GraphPoseNodeResult result;

        // Forward child node results
        if ( IsValid() )
        {
            result = m_pChildNode->Update( context, updateRange );
            m_duration = m_pChildNode->GetDuration();
            m_previousTime = m_pChildNode->GetPreviousTime();
            m_currentTime = m_pChildNode->GetCurrentTime();
        }
        else
        {
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
        }

        return result;
    }
}