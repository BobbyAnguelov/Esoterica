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
                m_selectedOptionIdx = InvalidIndex;
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Selector: Failed to select a valid option!" );
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
            EE_ASSERT( context.GetSampledEventsBuffer()->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    void SelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool SelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
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

        AnimationClipReferenceNode::InitializeInternal( context, initialTime );

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
                m_selectedOptionIdx = InvalidIndex;
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Clip Selector: Failed to select a valid option!" );
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

        AnimationClipReferenceNode::ShutdownInternal( context );
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
            EE_ASSERT( context.GetSampledEventsBuffer()->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    bool AnimationClipSelectorNode::HasAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->HasAnimation();
        }

        return false;
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

    void AnimationClipSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        AnimationClipReferenceNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool AnimationClipSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !AnimationClipReferenceNode::RestoreGraphState( inState ) )
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

    //-------------------------------------------------------------------------

    static int32_t PickOption( TInlineVector<int16_t, 10> const& options, TInlineVector<uint8_t, 10> const& weights, float parameterValue )
    {
        EE_ASSERT( options.size() == weights.size() );

        if ( options.empty() )
        {
            return InvalidIndex;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( Math::IsInRangeInclusive( parameterValue, float( -INT32_MAX ), float( INT32_MAX ) ) );
        int64_t const seed = Math::FloorToInt64( Math::Abs( parameterValue ) );

        //-------------------------------------------------------------------------

        TInlineVector<int16_t, 40> virtualOptions;
        int32_t const numOptions = (int32_t) options.size();
        for ( int32_t i = 0; i < numOptions; i++ )
        {
            for ( int32_t j = 0; j < weights[i]; j++ )
            {
                virtualOptions.emplace_back( options[i] );
            }
        }

        int64_t const numVirtualOptions = (int64_t) virtualOptions.size();
        if ( numVirtualOptions == 0 )
        {
            return InvalidIndex;
        }

        int16_t const selectedIdx = virtualOptions[seed % numVirtualOptions];
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

            // Generate options and weight lists
            TInlineVector<int16_t, 10> options;
            TInlineVector<uint8_t, 10> weights;
            auto pDefinition = GetDefinition<ParameterizedSelectorNode>();
            if ( pDefinition->m_ignoreInvalidOptions )
            {
                for ( int16_t i = 0; i < numOptions; i++ )
                {
                    m_optionNodes[i]->Initialize( context, SyncTrackTime() );
                    if ( m_optionNodes[i]->IsValid() )
                    {
                        options.emplace_back( i );
                        weights.emplace_back( pDefinition->m_hasWeightsSet ? pDefinition->m_optionWeights[i] : uint8_t( 1 ) );
                    }
                    m_optionNodes[i]->Shutdown( context );
                }
            }
            else
            {
                for ( int16_t i = 0; i < numOptions; i++ )
                {
                    options.emplace_back( i );
                    weights.emplace_back( pDefinition->m_hasWeightsSet ? pDefinition->m_optionWeights[i] : uint8_t( 1 ) );
                }
            }

            // Get parameter value
            m_pParameterNode->Initialize( context );
            float const parameterValue = m_pParameterNode->GetValue<float>( context );
            m_pParameterNode->Shutdown( context );

            // Calculate selected index
            int32_t const selectedIdx = PickOption( options, weights, parameterValue );
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
                m_selectedOptionIdx = InvalidIndex;
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Parameterized Selector: Failed to select a valid option!" );
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
            EE_ASSERT( context.GetSampledEventsBuffer()->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    void ParameterizedSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool ParameterizedSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
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

            // Generate options and weight lists
            TInlineVector<int16_t, 10> options;
            TInlineVector<uint8_t, 10> weights;
            auto pDefinition = GetDefinition<ParameterizedAnimationClipSelectorNode>();
            if ( pDefinition->m_ignoreInvalidOptions )
            {
                for ( int16_t i = 0; i < numOptions; i++ )
                {
                    if ( m_optionNodes[i]->HasAnimation() )
                    {
                        m_optionNodes[i]->Initialize( context, SyncTrackTime() );
                        if ( m_optionNodes[i]->IsValid() )
                        {
                            options.emplace_back( i );
                            weights.emplace_back( pDefinition->m_hasWeightsSet ? pDefinition->m_optionWeights[i] : uint8_t( 1 ) );
                        }
                        m_optionNodes[i]->Shutdown( context );
                    }
                }
            }
            else
            {
                for ( int16_t i = 0; i < numOptions; i++ )
                {
                    options.emplace_back( i );
                    weights.emplace_back( pDefinition->m_hasWeightsSet ? pDefinition->m_optionWeights[i] : uint8_t( 1 ) );
                }
            }

            // Get parameter value
            m_pParameterNode->Initialize( context );
            float const parameterValue = m_pParameterNode->GetValue<float>( context );
            m_pParameterNode->Shutdown( context );

            // Calculate selected index
            int32_t const selectedIdx = PickOption( options, weights, parameterValue );
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

        AnimationClipReferenceNode::InitializeInternal( context, initialTime );

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
                m_selectedOptionIdx = InvalidIndex;
            }
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "Paramaterized Clip Selector: Failed to select a valid option!" );
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

        AnimationClipReferenceNode::ShutdownInternal( context );
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
            EE_ASSERT( context.GetSampledEventsBuffer()->IsValidRange( result.m_sampledEventRange ) );
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
        }

        return result;
    }

    bool ParameterizedAnimationClipSelectorNode::HasAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->HasAnimation();
        }

        return false;
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

    void ParameterizedAnimationClipSelectorNode::RecordGraphState( RecordedGraphState& outState )
    {
        AnimationClipReferenceNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool ParameterizedAnimationClipSelectorNode::RestoreGraphState( RecordedGraphState const& inState )
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

    //-------------------------------------------------------------------------

    void IDBasedSelectorNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDBasedSelectorNode>( context, options );

        for ( int16_t nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        context.SetNodePtrFromIndex( m_parameterNodeIdx, pNode->m_pParameterNode );
        context.SetOptionalNodePtrFromIndex( m_fallbackNodeIdx, pNode->m_pFallbackNode );
    }

    int32_t IDBasedSelectorNode::SelectOption( GraphContext &context ) const
    {
        EE_ASSERT( context.IsValid() );

        auto pDefinition = GetDefinition<IDBasedSelectorNode>();
        EE_ASSERT( pDefinition->m_optionIDs.size() == m_optionNodes.size() );

        StringID const inputID = m_pParameterNode->GetValue<StringID>( context );

        for ( int32_t i = 0; i < m_optionNodes.size(); i++ )
        {
            if ( pDefinition->m_optionIDs[i] == inputID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    void IDBasedSelectorNode::InitializeInternal( GraphContext &context, SyncTrackTime const &initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pSelectedNode == nullptr );

        PoseNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        m_selectedOptionIdx = SelectOption( context );
        if ( m_selectedOptionIdx != InvalidIndex )
        {
            m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];

        }
        else if ( m_pFallbackNode != nullptr )
        {
            m_pSelectedNode = m_pFallbackNode;
        }

        //-------------------------------------------------------------------------

        auto TryInitializeSelectedNode = [this] ( GraphContext &context, SyncTrackTime const &initialTime )
        {
            m_pSelectedNode->Initialize( context, initialTime );

            if ( m_pSelectedNode->IsValid() )
            {
                m_duration = m_pSelectedNode->GetDuration();
                m_previousTime = m_pSelectedNode->GetPreviousTime();
                m_currentTime = m_pSelectedNode->GetCurrentTime();

                return true;
            }
            else
            {
                m_pSelectedNode->Shutdown( context );
                m_pSelectedNode = nullptr;
                m_selectedOptionIdx = InvalidIndex;
            }

            return false;
        };

        if ( m_pSelectedNode != nullptr )
        {
            if ( !TryInitializeSelectedNode( context, initialTime ) )
            {
                m_selectedOptionIdx = InvalidIndex;

                if ( m_pFallbackNode != nullptr )
                {
                    m_pSelectedNode = m_pFallbackNode;
                    TryInitializeSelectedNode( context, initialTime );
                }
            }
        }

        if ( m_pSelectedNode == nullptr || !m_pSelectedNode->IsValid() )
        {
            #if EE_DEVELOPMENT_TOOLS
            StringID const inputID = m_pParameterNode->GetValue<StringID>( context );
            context.LogWarning( GetNodeIndex(), "ID Selector: Failed to select a valid option for ID: %s!", inputID.IsValid() ? inputID.c_str() : "" );
            #endif
        }
    }

    void IDBasedSelectorNode::ShutdownInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult IDBasedSelectorNode::Update( GraphContext &context, SyncTrackTimeRange const *pUpdateRange )
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

    void IDBasedSelectorNode::RecordGraphState( RecordedGraphState &outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool IDBasedSelectorNode::RestoreGraphState( RecordedGraphState const &inState )
    {
        if ( !PoseNode::RestoreGraphState( inState ) )
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
            m_pSelectedNode = m_pFallbackNode;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void IDBasedAnimationClipSelectorNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDBasedAnimationClipSelectorNode>( context, options );

        for ( int16_t nodeIdx : m_optionNodeIndices )
        {
            context.SetNodePtrFromIndex( nodeIdx, pNode->m_optionNodes.emplace_back() );
        }

        context.SetNodePtrFromIndex( m_parameterNodeIdx, pNode->m_pParameterNode );
        context.SetOptionalNodePtrFromIndex( m_fallbackNodeIdx, pNode->m_pFallbackNode );
    }

    int32_t IDBasedAnimationClipSelectorNode::SelectOption( GraphContext &context ) const
    {
        EE_ASSERT( context.IsValid() );

        auto pDefinition = GetDefinition<IDBasedAnimationClipSelectorNode>();
        EE_ASSERT( pDefinition->m_optionIDs.size() == m_optionNodes.size() );

        StringID const inputID = m_pParameterNode->GetValue<StringID>( context );

        for ( int32_t i = 0; i < m_optionNodes.size(); i++ )
        {
            if ( pDefinition->m_optionIDs[i] == inputID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    void IDBasedAnimationClipSelectorNode::InitializeInternal( GraphContext &context, SyncTrackTime const &initialTime )
    {
        EE_ASSERT( context.IsValid() );

        AnimationClipReferenceNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        m_selectedOptionIdx = SelectOption( context );
        if ( m_selectedOptionIdx != InvalidIndex )
        {
            m_pSelectedNode = m_optionNodes[m_selectedOptionIdx];

        }
        else if ( m_pFallbackNode != nullptr )
        {
            m_pSelectedNode = m_pFallbackNode;
        }

        //-------------------------------------------------------------------------

        auto TryInitializeSelectedNode = [this] ( GraphContext &context, SyncTrackTime const &initialTime )
        {
            m_pSelectedNode->Initialize( context, initialTime );

            if ( m_pSelectedNode->IsValid() )
            {
                m_duration = m_pSelectedNode->GetDuration();
                m_previousTime = m_pSelectedNode->GetPreviousTime();
                m_currentTime = m_pSelectedNode->GetCurrentTime();

                return true;
            }
            else
            {
                m_pSelectedNode->Shutdown( context );
                m_pSelectedNode = nullptr;
                m_selectedOptionIdx = InvalidIndex;
            }

            return false;
        };

        if ( m_pSelectedNode != nullptr )
        {
            if ( !TryInitializeSelectedNode( context, initialTime ) )
            {
                m_selectedOptionIdx = InvalidIndex;

                if ( m_pFallbackNode != nullptr )
                {
                    m_pSelectedNode = m_pFallbackNode;
                    TryInitializeSelectedNode( context, initialTime );
                }
            }
        }

        if ( m_pSelectedNode == nullptr || !m_pSelectedNode->IsValid() )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogWarning( GetNodeIndex(), "ID Clip Selector: Failed to select a valid option!" );
            #endif
        }
    }

    void IDBasedAnimationClipSelectorNode::ShutdownInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->Shutdown( context );
            m_pSelectedNode = nullptr;
        }

        AnimationClipReferenceNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult IDBasedAnimationClipSelectorNode::Update( GraphContext &context, SyncTrackTimeRange const *pUpdateRange )
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

    bool IDBasedAnimationClipSelectorNode::HasAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->HasAnimation();
        }

        return false;
    }

    AnimationClip const *IDBasedAnimationClipSelectorNode::GetAnimation() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->GetAnimation();
        }

        return nullptr;
    }

    void IDBasedAnimationClipSelectorNode::DisableRootMotionSampling()
    {
        if ( m_pSelectedNode != nullptr )
        {
            m_pSelectedNode->DisableRootMotionSampling();
        }
    }

    bool IDBasedAnimationClipSelectorNode::IsLooping() const
    {
        if ( m_pSelectedNode != nullptr )
        {
            return m_pSelectedNode->IsLooping();
        }

        return false;
    }

    void IDBasedAnimationClipSelectorNode::RecordGraphState( RecordedGraphState &outState )
    {
        AnimationClipReferenceNode::RecordGraphState( outState );
        outState.WriteValue( m_selectedOptionIdx );
    }

    bool IDBasedAnimationClipSelectorNode::RestoreGraphState( RecordedGraphState const &inState )
    {
        if ( !AnimationClipReferenceNode::RestoreGraphState( inState ) )
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
            m_pSelectedNode = m_pFallbackNode;
        }

        return true;
    }
}