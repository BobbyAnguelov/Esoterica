#include "Animation_RuntimeGraphNode_Blend1D.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    // Creates a parameterization for a given set of values (each value corresponds to an input node and are initially ordered as such)
    ParameterizedBlendNode::Parameterization ParameterizedBlendNode::Parameterization::CreateParameterization( TInlineVector<float, 5> const& values )
    {
        struct IndexValuePair
        {
            int16_t             m_idx;
            float               m_value;
        };

        // Create sorted list of index/value pairs
        //-------------------------------------------------------------------------

        TInlineVector<IndexValuePair, 10> sortedIndexValuePairs;
        int16_t const numSources = (int16_t) values.size();
        sortedIndexValuePairs.resize( numSources );
        for ( int16_t i = 0; i < numSources; i++ )
        {
            sortedIndexValuePairs[i].m_idx = i;
            sortedIndexValuePairs[i].m_value = values[i];
        }

        // Sort the options based on value
        //-------------------------------------------------------------------------

        auto SortPredicate = [] ( IndexValuePair const& a, IndexValuePair const& b )
        {
            if ( a.m_value == b.m_value )
            {
                return a.m_idx < b.m_idx;
            }

            return a.m_value < b.m_value;
        };

        eastl::sort( sortedIndexValuePairs.begin(), sortedIndexValuePairs.end(), SortPredicate );

        // Create the parameterization
        //-------------------------------------------------------------------------

        Parameterization parameterization;

        int32_t const numBlendRanges = numSources - 1;
        parameterization.m_blendRanges.resize( numBlendRanges );
        for ( auto i = 0; i < numBlendRanges; i++ )
        {
            EE_ASSERT( sortedIndexValuePairs[i].m_value <= sortedIndexValuePairs[i + 1].m_value );
            parameterization.m_blendRanges[i].m_inputIdx0 = sortedIndexValuePairs[i].m_idx;
            parameterization.m_blendRanges[i].m_inputIdx1 = sortedIndexValuePairs[i + 1].m_idx;
            parameterization.m_blendRanges[i].m_parameterValueRange = FloatRange( sortedIndexValuePairs[i].m_value, sortedIndexValuePairs[i + 1].m_value );
        }

        parameterization.m_parameterRange = FloatRange( sortedIndexValuePairs.front().m_value, sortedIndexValuePairs.back().m_value );

        return parameterization;
    }

    //-------------------------------------------------------------------------

    void ParameterizedBlendNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        EE_ASSERT( options == InstantiationOptions::NodeAlreadyCreated );

        auto pNode = reinterpret_cast<ParameterizedBlendNode*>( context.m_nodePtrs[m_nodeIdx] );
        context.SetNodePtrFromIndex( m_inputParameterValueNodeIdx, pNode->m_pInputParameterValueNode );

        pNode->m_sourceNodes.reserve( m_sourceNodeIndices.size() );
        for ( auto sourceIdx : m_sourceNodeIndices )
        {
            context.SetNodePtrFromIndex( sourceIdx, pNode->m_sourceNodes.emplace_back() );
        }
    }

    bool ParameterizedBlendNode::IsValid() const
    {
        if ( !PoseNode::IsValid() )
        {
            return false;
        }

        if ( !m_pInputParameterValueNode->IsValid() )
        {
            return false;
        }

        for ( auto pSource : m_sourceNodes )
        {
            if ( !pSource->IsValid() )
            {
                return false;
            }
        }

        return true;
    }

    void ParameterizedBlendNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pInputParameterValueNode != nullptr && m_sourceNodes.size() > 1 );

        PoseNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        m_pInputParameterValueNode->Initialize( context );

        for ( auto pSourceNode : m_sourceNodes )
        {
            pSourceNode->Initialize( context, initialTime );
        }

        //-------------------------------------------------------------------------

        if ( IsValid() )
        {
            EvaluateBlendSpace( context );
            m_previousTime = m_currentTime = m_blendedSyncTrack.GetPercentageThrough( initialTime );
        }
    }

    void ParameterizedBlendNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        for ( auto Source : m_sourceNodes )
        {
            Source->Shutdown( context );
        }

        m_pInputParameterValueNode->Shutdown( context );

        //-------------------------------------------------------------------------

        m_bsr.Reset();

        PoseNode::ShutdownInternal( context );
    }

    void ParameterizedBlendNode::EvaluateBlendSpace( GraphContext& context)
    {
        // Ensure we only update the blend space once per update
        //-------------------------------------------------------------------------

        if ( context.m_updateID == m_bsr.m_updateID )
        {
            return;
        }

        m_bsr.m_updateID = context.m_updateID;

        // Evaluate parameter
        //-------------------------------------------------------------------------

        int32_t selectedRangeIdx = InvalidIndex;

        // Get Parameter clamped to parameterization range
        float inputParameterValue = m_pInputParameterValueNode->GetValue<float>( context );
        inputParameterValue = m_parameterization.m_parameterRange.GetClampedValue( inputParameterValue );

        // Find matching source nodes and blend weight
        auto const numBlendRanges = m_parameterization.m_blendRanges.size();
        for ( auto i = 0; i < numBlendRanges; i++ )
        {
            if ( m_parameterization.m_blendRanges[i].m_parameterValueRange.ContainsInclusive( inputParameterValue ) )
            {
                selectedRangeIdx = i;
                m_bsr.m_blendWeight = m_parameterization.m_blendRanges[i].m_parameterValueRange.GetPercentageThrough( inputParameterValue );
                break;
            }
        }

        // Ensure we found a valid range
        EE_ASSERT( selectedRangeIdx != InvalidIndex );

        // Calculate final blend / sync track / duration
        //-------------------------------------------------------------------------

        auto const& blendRange = m_parameterization.m_blendRanges[selectedRangeIdx];
        if ( m_bsr.m_blendWeight == 0.0f )
        {
            m_bsr.m_pSource0 = m_sourceNodes[blendRange.m_inputIdx0];
            m_bsr.m_pSource1 = nullptr;

            m_blendedSyncTrack = m_bsr.m_pSource0->GetSyncTrack();
            m_duration = m_bsr.m_pSource0->GetDuration();
        }
        else if ( m_bsr.m_blendWeight == 1.0f )
        {
            m_bsr.m_pSource0 = m_sourceNodes[blendRange.m_inputIdx1];
            m_bsr.m_pSource1 = nullptr;

            m_blendedSyncTrack = m_bsr.m_pSource0->GetSyncTrack();
            m_duration = m_bsr.m_pSource0->GetDuration();
        }
        else
        {
            m_bsr.m_pSource0 = m_sourceNodes[blendRange.m_inputIdx0];
            m_bsr.m_pSource1 = m_sourceNodes[blendRange.m_inputIdx1];

            SyncTrack const& syncTrack0 = m_bsr.m_pSource0->GetSyncTrack();
            SyncTrack const& syncTrack1 = m_bsr.m_pSource1->GetSyncTrack();
            m_blendedSyncTrack = SyncTrack( syncTrack0, syncTrack1, m_bsr.m_blendWeight );
            m_duration = SyncTrack::CalculateDurationSynchronized( m_bsr.m_pSource0->GetDuration(), m_bsr.m_pSource1->GetDuration(), syncTrack0.GetNumEvents(), syncTrack1.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_bsr.m_blendWeight );
        }
    }

    GraphPoseNodeResult ParameterizedBlendNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( !IsValid() )
        {
            return result;
        }

        MarkNodeActive( context );
        EvaluateBlendSpace( context );

        // Calculate actual update range
        //-------------------------------------------------------------------------

        SyncTrackTimeRange updateRange;
        if ( pUpdateRange != nullptr )
        {
            updateRange = *pUpdateRange;
        }
        else
        {
            auto* pDefinition = GetDefinition<ParameterizedBlendNode>();

            Percentage const deltaPercentage = ( m_duration > 0.0f ) ? Percentage( context.m_deltaTime / m_duration ) : Percentage( 0.0f );
            Percentage const fromTime = m_currentTime;
            Percentage const toTime = Percentage::Clamp( m_currentTime + deltaPercentage, pDefinition->m_allowLooping );

            updateRange.m_startTime = m_blendedSyncTrack.GetTime( fromTime );
            updateRange.m_endTime = m_blendedSyncTrack.GetTime( toTime );
        }

        // Only a single source
        //-------------------------------------------------------------------------

        if ( m_bsr.m_pSource1 == nullptr )
        {
            result = m_bsr.m_pSource0->Update( context, &updateRange );
            m_previousTime = m_bsr.m_pSource0->GetPreviousTime();
            m_currentTime = m_bsr.m_pSource0->GetCurrentTime();
        }

        // 2-Way Blend
        //-------------------------------------------------------------------------

        else
        {
            // Update Source 0
            GraphPoseNodeResult const sourceResult0 = m_bsr.m_pSource0->Update( context, &updateRange );
            #if EE_DEVELOPMENT_TOOLS
            int16_t const rootMotionActionIdxSource0 = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update Source 1
            GraphPoseNodeResult const sourceResult1 = m_bsr.m_pSource1->Update( context, &updateRange );
            #if EE_DEVELOPMENT_TOOLS
            int16_t const rootMotionActionIdxSource1 = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Register Tasks
            //-------------------------------------------------------------------------

            if ( sourceResult0.HasRegisteredTasks() && sourceResult1.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult0.m_taskIdx, sourceResult1.m_taskIdx, m_bsr.m_blendWeight );
                result.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult0.m_rootMotionDelta, sourceResult1.m_rootMotionDelta, m_bsr.m_blendWeight );

                #if EE_DEVELOPMENT_TOOLS
                context.GetRootMotionDebugger()->RecordBlend( GetNodeIndex(), rootMotionActionIdxSource0, rootMotionActionIdxSource1, result.m_rootMotionDelta );
                #endif
            }
            else
            {
                result = ( sourceResult0.HasRegisteredTasks() ) ? sourceResult0 : sourceResult1;
            }

            result.m_sampledEventRange = context.m_pSampledEventsBuffer->BlendEventRanges( sourceResult0.m_sampledEventRange, sourceResult1.m_sampledEventRange, m_bsr.m_blendWeight );

            // Update internal time
            //-------------------------------------------------------------------------

            m_previousTime = GetSyncTrack().GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = GetSyncTrack().GetPercentageThrough( updateRange.m_endTime );
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void ParameterizedBlendNode::GetDebugInfo( int16_t& outSourceIdx0, int16_t& outSourceIdx1, float& outBlendweight ) const
    {
        outSourceIdx0 = ( m_bsr.m_pSource0 != nullptr ) ? m_bsr.m_pSource0->GetNodeIndex() : InvalidIndex;
        outSourceIdx1 = ( m_bsr.m_pSource1 != nullptr ) ? m_bsr.m_pSource1->GetNodeIndex() : InvalidIndex;
        outBlendweight = m_bsr.m_blendWeight;
    }

    void ParameterizedBlendNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        auto FindSourceIndex = [&] ( PoseNode* pNode ) -> int32_t
        {
            if ( pNode == nullptr )
            {
                return InvalidIndex;
            }

            int32_t const numSourceNodes = (int32_t) m_sourceNodes.size();
            for ( int32_t i = 0; i < numSourceNodes; i++ )
            {
                if ( m_sourceNodes[i] == pNode )
                {
                    return i;
                }
            }

            return InvalidIndex;
        };

        outState.WriteValue( FindSourceIndex( m_bsr.m_pSource0 ) );
        outState.WriteValue( FindSourceIndex( m_bsr.m_pSource1 ) );
        outState.WriteValue( m_bsr.m_blendWeight );
        outState.WriteValue( m_blendedSyncTrack );
    }

    void ParameterizedBlendNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        //-------------------------------------------------------------------------

        int32_t idx = InvalidIndex;

        inState.ReadValue( idx );
        m_bsr.m_pSource0 = idx != InvalidIndex ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( idx );
        m_bsr.m_pSource0 = idx != InvalidIndex ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( m_bsr.m_blendWeight );
        inState.ReadValue( m_blendedSyncTrack );
    }
    #endif

    //-------------------------------------------------------------------------

    void Blend1DNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<Blend1DNode>( context, options );
        ParameterizedBlendNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        pNode->m_parameterization = m_parameterization;
    }

    //-------------------------------------------------------------------------

    void VelocityBlendNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VelocityBlendNode>( context, options );
        ParameterizedBlendNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void VelocityBlendNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        ParameterizedBlendNode::InitializeInternal( context, initialTime );

        if ( !m_lazyInitializationPerformed )
        {
            CreateParameterizationFromSpeeds();
            m_lazyInitializationPerformed = true;
        }
    }

    void VelocityBlendNode::CreateParameterizationFromSpeeds()
    {
        // Get source node speeds
        //-------------------------------------------------------------------------

        TInlineVector<float, 5> values;
        int32_t const numSources = (int32_t) m_sourceNodes.size();
        for ( int16_t i = 0; i < numSources; i++ )
        {
            // The editor tooling guarantees that the source nodes are actually clip references!
            AnimationClip const* pAnimation = reinterpret_cast<AnimationClipReferenceNode const*>( m_sourceNodes[i] )->GetAnimation();
            EE_ASSERT( pAnimation != nullptr );
            values.emplace_back( pAnimation->GetAverageLinearVelocity() );
        }

        // Create parameterization
        //-------------------------------------------------------------------------

        m_parameterization = Parameterization::CreateParameterization( values );
    }

    #if EE_DEVELOPMENT_TOOLS
    void VelocityBlendNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        //-------------------------------------------------------------------------

        if ( !m_lazyInitializationPerformed )
        {
            CreateParameterizationFromSpeeds();
        }

        //-------------------------------------------------------------------------

        int32_t idx = InvalidIndex;

        inState.ReadValue( idx );
        m_bsr.m_pSource0 = ( idx != InvalidIndex ) ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( idx );
        m_bsr.m_pSource0 = ( idx != InvalidIndex ) ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( m_bsr.m_blendWeight );
        inState.ReadValue( m_blendedSyncTrack );
    }
    #endif
}
