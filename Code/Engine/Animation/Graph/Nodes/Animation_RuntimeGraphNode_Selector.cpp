#include "Animation_RuntimeGraphNode_Selector.h"


//-------------------------------------------------------------------------

namespace EE::Animation
{
    void SelectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SelectorNode>( context, options );

        for ( auto nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        for ( auto nodeIdx : m_conditionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_conditions.emplace_back() );
        }
    }

    int32_t SelectorNode::SelectOption( GraphContext& context ) const
    {
        EE_ASSERT( context.IsValid() );

        // Select a valid option
        if ( m_optionNodes.size() > 0 )
        {
            int32_t selectedIdx = InvalidIndex;

            auto const numOptions = m_optionNodes.size();
            EE_ASSERT( m_optionNodes.size() == m_conditions.size() );

            // Initialize conditions
            for ( auto pConditionNode : m_conditions )
            {
                pConditionNode->Initialize( context );
            }

            // Check all conditions
            for ( auto i = 0; i < numOptions; i++ )
            {
                if ( m_conditions[i]->GetValue<bool>( context ) )
                {
                    selectedIdx = i;
                    break;
                }
            }

            // Shutdown all conditions
            for ( auto pConditionNode : m_conditions )
            {
                pConditionNode->Shutdown( context );
            }

            return selectedIdx;
        }
        else
        {
            return InvalidIndex;
        }
    }

    void SelectorNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );

        // Select option and try to create transient data for it
        m_selectedOptionIdx = SelectOption( context );
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
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Failed to select a valid option!" );
            #endif
        }
    }

    void SelectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult SelectorNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
            EE_ASSERT( context.m_pSampledEventsBuffer->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void SelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    void SelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_selectedOptionIdx );
        m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];
    }
    #endif

    //-------------------------------------------------------------------------

    void AnimationClipSelectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<AnimationClipSelectorNode>( context, options );

        for ( auto nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        for ( auto nodeIdx : m_conditionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_conditions.emplace_back() );
        }
    }

    int32_t AnimationClipSelectorNode::SelectOption( GraphContext& context ) const
    {
        EE_ASSERT( context.IsValid() );

        // Select a valid option
        if ( m_optionNodes.size() > 0 )
        {
            int32_t selectedIdx = InvalidIndex;
            auto const numOptions = m_optionNodes.size();
            EE_ASSERT( m_optionNodes.size() == m_conditions.size() );

            // Initialize conditions
            for ( auto pConditionNode : m_conditions )
            {
                pConditionNode->Initialize( context );
            }

            // Check all conditions
            for ( auto i = 0; i < numOptions; i++ )
            {
                if ( m_conditions[i]->GetValue<bool>( context ) )
                {
                    selectedIdx = i;
                    break;
                }
            }

            // Shutdown all conditions
            for ( auto pConditionNode : m_conditions )
            {
                pConditionNode->Shutdown( context );
            }

            return selectedIdx;
        }
        else
        {
            return InvalidIndex;
        }
    }

    void AnimationClipSelectorNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );

        // Select option and try to create transient data for it
        m_selectedOptionIdx = SelectOption( context );
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
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Failed to select a valid option!" );
            #endif
        }
    }

    void AnimationClipSelectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult AnimationClipSelectorNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
            EE_ASSERT( context.m_pSampledEventsBuffer->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    AnimationClip const* AnimationClipSelectorNode::GetAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->GetAnimation();
        }

        return nullptr;
    }

    void AnimationClipSelectorNode::DisableRootMotionSampling()
    {
        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->DisableRootMotionSampling();
        }
    }

    bool AnimationClipSelectorNode::IsLooping() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->IsLooping();
        }

        return false;
    }

    #if EE_DEVELOPMENT_TOOLS
    void AnimationClipSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        AnimationClipReferenceNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    void AnimationClipSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        AnimationClipReferenceNode::RestoreGraphState( inState );
        inState.ReadValue( m_selectedOptionIdx );
        m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];
    }
    #endif

    //-------------------------------------------------------------------------

    static int32_t PickOption( int32_t numOptions, TInlineVector<uint8_t, 5> const &optionWeights, int32_t seed )
    {
        int32_t selectedIdx = InvalidIndex;

        if ( optionWeights.empty() )
        {
            selectedIdx = seed % numOptions;
        }
        else
        {
            EE_ASSERT( optionWeights.size() == numOptions );

            // Create bucket boundaries based on the option weights, basically allocate an entry for each option n times, where n is the weight
            int32_t* pOptionBucketBoundaries = EE_STACK_ARRAY_ALLOC( int32_t, optionWeights.size() );

            int32_t totalWeightedOptions = 0;
            for ( auto i = 0; i < numOptions; i++ )
            {
                EE_ASSERT( optionWeights[i] > 0 );
                pOptionBucketBoundaries[i] = totalWeightedOptions + optionWeights[i];
                totalWeightedOptions += optionWeights[i];
            }

            EE_ASSERT( totalWeightedOptions > 0 );

            // Select a value from the new set of options
            int32_t const weightedIdx = seed % totalWeightedOptions;

            // Find the bucket that we rolled into and return that
            for ( int32_t i = 0; i < numOptions; i++ )
            {
                if ( weightedIdx <= pOptionBucketBoundaries[i] )
                {
                    selectedIdx = i;
                    break;
                }
            }
        }

        return selectedIdx;
    }

    void ParameterizedSelectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ParameterizedSelectorNode>( context, options );

        for ( auto nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        context.SetNodePtrFromIndex( m_parameterNodeIdx, pNode->m_pParameterNode );
    }

    int32_t ParameterizedSelectorNode::SelectOption( GraphContext& context ) const
    {
        EE_ASSERT( context.IsValid() );

        // Select a valid option
        if ( m_optionNodes.size() > 0 )
        {
            int32_t const numOptions = (int32_t) m_optionNodes.size();

            // Get parameter value
            m_pParameterNode->Initialize( context );
            float const parameterValue = m_pParameterNode->GetValue<float>( context );
            m_pParameterNode->Shutdown( context );
            int32_t const seed = Math::FloorToInt( Math::Abs( parameterValue ) );

            // Calculate selected index
            auto pDefinition = GetDefinition<ParameterizedSelectorNode>();
            int32_t const selectedIdx = PickOption( numOptions, pDefinition->m_optionWeights, seed );
            return selectedIdx;
        }
        else
        {
            return InvalidIndex;
        }
    }

    void ParameterizedSelectorNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );

        // Select option and try to create transient data for it
        m_selectedOptionIdx = SelectOption( context );
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
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Failed to select a valid option!" );
            #endif
        }
    }

    void ParameterizedSelectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult ParameterizedSelectorNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
            EE_ASSERT( context.m_pSampledEventsBuffer->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ParameterizedSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    void ParameterizedSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_selectedOptionIdx );
        m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];
    }
    #endif

    //-------------------------------------------------------------------------

    void ParameterizedAnimationClipSelectorNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ParameterizedAnimationClipSelectorNode>( context, options );

        for ( auto nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        context.SetNodePtrFromIndex( m_parameterNodeIdx, pNode->m_pParameterNode );
    }

    int32_t ParameterizedAnimationClipSelectorNode::SelectOption( GraphContext& context ) const
    {
        EE_ASSERT( context.IsValid() );

        // Select a valid option
        if ( m_optionNodes.size() > 0 )
        {
            int32_t const numOptions = (int32_t) m_optionNodes.size();

            // Get parameter value
            m_pParameterNode->Initialize( context );
            float const parameterValue = m_pParameterNode->GetValue<float>( context );
            m_pParameterNode->Shutdown( context );
            int32_t const seed = Math::FloorToInt( Math::Abs( parameterValue ) );

            // Calculate selected index
            auto pDefinition = GetDefinition<ParameterizedAnimationClipSelectorNode>();
            int32_t const selectedIdx = PickOption( numOptions, pDefinition->m_optionWeights, seed );
            return selectedIdx;
        }
        else
        {
            return InvalidIndex;
        }
    }

    void ParameterizedAnimationClipSelectorNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );

        // Select option and try to create transient data for it
        m_selectedOptionIdx = SelectOption( context );
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
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Failed to select a valid option!" );
            #endif
        }
    }

    void ParameterizedAnimationClipSelectorNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult ParameterizedAnimationClipSelectorNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
            EE_ASSERT( context.m_pSampledEventsBuffer->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    AnimationClip const* ParameterizedAnimationClipSelectorNode::GetAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->GetAnimation();
        }

        return nullptr;
    }

    void ParameterizedAnimationClipSelectorNode::DisableRootMotionSampling()
    {
        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->DisableRootMotionSampling();
        }
    }

    bool ParameterizedAnimationClipSelectorNode::IsLooping() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->IsLooping();
        }

        return false;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ParameterizedAnimationClipSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    void ParameterizedAnimationClipSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_selectedOptionIdx );
        m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];
    }
    #endif
}