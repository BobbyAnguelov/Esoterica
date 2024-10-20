#include "Animation_RuntimeGraphNode_SpeedScale.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    GraphPoseNodeResult SpeedScaleBaseNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        MarkNodeActive( context );

        //-------------------------------------------------------------------------

        float const speedScale = CalculateSpeedScaleMultiplier( context );
        EE_ASSERT( speedScale >= 0.0f );

        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;

        // Synchronized Update
        //-------------------------------------------------------------------------
        // This is only guaranteed to work with transitions as the duration of the node affects the sync update steps

        if ( pUpdateRange != nullptr )
        {
            result = PassthroughNode::Update( context, pUpdateRange );

            // Simplify the duration, this will inform the parent that we are scaled and potentially affect the sync update.
            if ( m_pChildNode->IsValid() )
            {
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
        else // Unsynchronized Update
        {
            // Record old delta time
            auto const deltaTime = context.m_deltaTime;

            // Adjust the delta time for the child node
            //-------------------------------------------------------------------------

            float actualDuration = 0.0f;
            if ( m_pChildNode->IsValid() )
            {
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

    //-------------------------------------------------------------------------

    void SpeedScaleNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SpeedScaleNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pScaleValueNode );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void SpeedScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PassthroughNode::InitializeInternal( context, initialTime );

        if ( m_pScaleValueNode != nullptr )
        {
            m_pScaleValueNode->Initialize( context );
        }
    }

    void SpeedScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pScaleValueNode != nullptr )
        {
            m_pScaleValueNode->Shutdown( context );
        }

        PassthroughNode::ShutdownInternal( context );
    }

    float SpeedScaleNode::CalculateSpeedScaleMultiplier( GraphContext& context ) const
    {
        float speedScaleMultiplier = 1.0f;

        if ( m_pScaleValueNode != nullptr )
        {
            speedScaleMultiplier = m_pScaleValueNode->GetValue<float>( context );
            if ( speedScaleMultiplier < 0.0f )
            {
                speedScaleMultiplier = 0.0f;

                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Negative speed scale is not supported!" );
                #endif
            }
        }
        else
        {
            auto pDefinition = GetDefinition<SpeedScaleNode>();
            EE_ASSERT( pDefinition->m_defaultInputValue > 0.0f );
            speedScaleMultiplier = pDefinition->m_defaultInputValue;
        }

        return speedScaleMultiplier;
    }

    //-------------------------------------------------------------------------

    void DurationScaleNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<DurationScaleNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pDurationValueNode );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void DurationScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PassthroughNode::InitializeInternal( context, initialTime );

        if ( m_pDurationValueNode != nullptr )
        {
            m_pDurationValueNode->Initialize( context );
        }
    }

    void DurationScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pDurationValueNode != nullptr )
        {
            m_pDurationValueNode->Shutdown( context );
        }

        PassthroughNode::ShutdownInternal( context );
    }

    float DurationScaleNode::CalculateSpeedScaleMultiplier( GraphContext& context ) const
    {
        float desiredDuration = 0.0f;

        if ( m_pDurationValueNode != nullptr )
        {
            desiredDuration = m_pDurationValueNode->GetValue<float>( context );

            if ( desiredDuration < 0.0f )
            {
                desiredDuration = 0.0f;

                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Negative duration is not supported!" );
                #endif
            }
        }
        else
        {
            auto pDefinition = GetDefinition<DurationScaleNode>();
            EE_ASSERT( pDefinition->m_defaultInputValue > 0.0f );
            desiredDuration = pDefinition->m_defaultInputValue;
        }

        //-------------------------------------------------------------------------

        float speedScaleMultiplier = 1.0f;
        float const childDuration = m_pChildNode->IsValid() ? m_pChildNode->GetDuration().ToFloat() : -1.0f;
        if ( childDuration > 0.0f )
        {
            speedScaleMultiplier = childDuration / desiredDuration;
        }

        return speedScaleMultiplier;
    }

    //-------------------------------------------------------------------------

    void VelocityBasedSpeedScaleNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VelocityBasedSpeedScaleNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pDesiredVelocityValueNode );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void VelocityBasedSpeedScaleNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PassthroughNode::InitializeInternal( context, initialTime );

        if ( m_pDesiredVelocityValueNode != nullptr )
        {
            m_pDesiredVelocityValueNode->Initialize( context );
        }
    }

    void VelocityBasedSpeedScaleNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pDesiredVelocityValueNode != nullptr )
        {
            m_pDesiredVelocityValueNode->Shutdown( context );
        }

        PassthroughNode::ShutdownInternal( context );
    }

    float VelocityBasedSpeedScaleNode::CalculateSpeedScaleMultiplier( GraphContext& context ) const
    {
        float desiredVelocity = 0.0f;

        if ( m_pDesiredVelocityValueNode != nullptr )
        {
            desiredVelocity = m_pDesiredVelocityValueNode->GetValue<float>( context );

            if ( desiredVelocity < 0.0f )
            {
                desiredVelocity = 0.0f;

                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Requesting a negative velocity is not supported!" );
                #endif
            }
        }
        else
        {
            auto pDefinition = GetDefinition<VelocityBasedSpeedScaleNode>();
            EE_ASSERT( pDefinition->m_defaultInputValue > 0.0f );
            desiredVelocity = pDefinition->m_defaultInputValue;
        }

        //-------------------------------------------------------------------------

        float speedScaleMultiplier = 1.0f;
        if ( Math::IsNearZero( desiredVelocity ) )
        {
            speedScaleMultiplier = 0.0f;
        }
        else
        {
            float const averageVelocity = static_cast<AnimationClipReferenceNode*>( m_pChildNode )->GetAnimation()->GetAverageLinearVelocity();
            speedScaleMultiplier = desiredVelocity / averageVelocity;
            EE_ASSERT( speedScaleMultiplier >= 0.0f );
        }

        return speedScaleMultiplier;
    }
}