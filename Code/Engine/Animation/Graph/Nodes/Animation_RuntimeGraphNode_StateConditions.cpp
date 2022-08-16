#include "Animation_RuntimeGraphNode_StateConditions.h"
#include "Animation_RuntimeGraphNode_State.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void StateCompletedConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<StateCompletedConditionNode>( context, options );
        context.SetNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
        context.SetOptionalNodePtrFromIndex( m_transitionDurationOverrideNodeIdx, pNode->m_pDurationOverrideNode );
    }

    void StateCompletedConditionNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );

        BoolValueNode::InitializeInternal( context );
        m_result = false;

        if ( m_pDurationOverrideNode != nullptr )
        {
            m_pDurationOverrideNode->Initialize( context );
        }
    }

    void StateCompletedConditionNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );

        if ( m_pDurationOverrideNode != nullptr )
        {
            m_pDurationOverrideNode->Shutdown( context );
        }

        BoolValueNode::ShutdownInternal( context );
    }

    void StateCompletedConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        auto pSettings = GetSettings<StateCompletedConditionNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            float transitionDuration = pSettings->m_transitionDuration;
            if ( m_pDurationOverrideNode != nullptr )
            {
                transitionDuration = m_pDurationOverrideNode->GetValue<float>( context );
            }

            Percentage const transitionTime( transitionDuration / m_pSourceStateNode->GetDuration() );
            Percentage const transitionPoint = 1.0f - transitionTime;

            m_result = ( m_pSourceStateNode->GetCurrentTime() >= transitionPoint );
            MarkNodeActive( context );
        }

        // Set Result
        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void TimeConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TimeConditionNode>( context, options );
        context.SetNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void TimeConditionNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        BoolValueNode::InitializeInternal( context );
        m_result = false;
    }

    void TimeConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() );
        auto pSettings = GetSettings<TimeConditionNode>();

        //-------------------------------------------------------------------------

        auto DoComparision = [this, pSettings] ( float a, float b )
        {
            switch ( pSettings->m_operator )
            {   
                case Operator::LessThan:
                {
                    return a < b;
                }
                break;

                case Operator::LessThanEqual:
                {
                    return a <= b;
                }
                break;

                case Operator::GreaterThan:
                {
                    return a > b;
                }
                break;

                case Operator::GreaterThanEqual:
                {
                    return a >= b;
                }
                break;
            }

            EE_UNREACHABLE_CODE();
            return false;
        };

        //-------------------------------------------------------------------------

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            float const comparisonValue = ( m_pInputValueNode != nullptr ) ? m_pInputValueNode->GetValue<float>( context ) : pSettings->m_comparand;

            switch ( pSettings->m_type )
            {
                case ComparisonType::PercentageThroughState:
                {
                    m_result = DoComparision( m_pSourceStateNode->GetCurrentTime().ToFloat(), comparisonValue );
                }
                break;

                case ComparisonType::PercentageThroughSyncEvent:
                {
                    auto const syncTrackTime = m_pSourceStateNode->GetSyncTrack().GetTime( m_pSourceStateNode->GetCurrentTime() );
                    m_result = DoComparision( syncTrackTime.m_percentageThrough, comparisonValue );
                }
                break;

                case ComparisonType::LoopCount:
                {
                    float const loopCount = (float) m_pSourceStateNode->GetCurrentTime().GetLoopCount();
                    m_result = DoComparision( loopCount, comparisonValue );
                }
                break;

                case ComparisonType::ElapsedTime:
                {
                    m_result = DoComparision( m_pSourceStateNode->GetElapsedTimeInState(), comparisonValue );
                }
                break;
            }
        }

        // Set Result
        *( (bool*) pOutValue ) = m_result;
    }
}