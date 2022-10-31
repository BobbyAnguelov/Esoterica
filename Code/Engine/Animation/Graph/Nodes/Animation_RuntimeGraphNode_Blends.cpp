#include "Animation_RuntimeGraphNode_Blends.h"
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
    ParameterizedBlendNode::Parameterization ParameterizedBlendNode::Parameterization::CreateParameterization( TInlineVector<float, 5> values )
    {
        struct IndexValuePair
        {
            int16_t           m_idx;
            float           m_value;
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

    void ParameterizedBlendNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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

        if ( IsValid() )
        {
            for ( auto pSourceNode : m_sourceNodes )
            {
                pSourceNode->Initialize( context, initialTime );
            }

            m_pInputParameterValueNode->Initialize( context );

            InitializeParameterization( context );
            SelectBlendRange( context );
            auto const& blendRange = GetParameterization().m_blendRanges[m_selectedRangeIdx];
            auto Source0 = m_sourceNodes[blendRange.m_inputIdx0];
            auto Source1 = m_sourceNodes[blendRange.m_inputIdx1];
            EE_ASSERT( Source0 != nullptr && Source1 != nullptr );

            auto pSettings = GetSettings<RangedBlendNode>();
            if ( pSettings->m_isSynchronized )
            {
                SyncTrack const& SyncTrackSource0 = Source0->GetSyncTrack();
                SyncTrack const& SyncTrackSource1 = Source1->GetSyncTrack();
                m_blendedSyncTrack = SyncTrack( SyncTrackSource0, SyncTrackSource1, m_blendWeight );
                m_duration = SyncTrack::CalculateDurationSynchronized( Source0->GetDuration(), Source1->GetDuration(), SyncTrackSource0.GetNumEvents(), SyncTrackSource1.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_blendWeight );
            }
            else
            {
                m_duration = Math::Lerp( Source0->GetDuration(), Source1->GetDuration(), m_blendWeight );
            }

            m_previousTime = GetSyncTrack().GetPercentageThrough( initialTime );
            m_currentTime = m_previousTime;
        }
    }

    void ParameterizedBlendNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( IsValid() )
        {
            ShutdownParameterization( context );

            //-------------------------------------------------------------------------

            m_pInputParameterValueNode->Shutdown( context );

            for ( auto Source : m_sourceNodes )
            {
                Source->Shutdown( context );
            }
        }

        PoseNode::ShutdownInternal( context );
    }

    void ParameterizedBlendNode::SelectBlendRange( GraphContext& context)
    {
        auto const& parameterization = GetParameterization();

        m_selectedRangeIdx = InvalidIndex;
        m_blendWeight = -1.0f;

        // Get Parameter clamped to parameterization range
        float inputParameterValue = m_pInputParameterValueNode->GetValue<float>( context );
        inputParameterValue = parameterization.m_parameterRange.GetClampedValue( inputParameterValue );

        // Find matching source nodes and blend weight
        auto const numBlendRanges = parameterization.m_blendRanges.size();
        for ( auto i = 0; i < numBlendRanges; i++ )
        {
            if ( parameterization.m_blendRanges[i].m_parameterValueRange.ContainsInclusive( inputParameterValue ) )
            {
                m_selectedRangeIdx = i;
                m_blendWeight = parameterization.m_blendRanges[i].m_parameterValueRange.GetPercentageThrough( inputParameterValue );
                break;
            }
        }

        // Ensure we found a valid range
        EE_ASSERT( m_blendWeight != -1 );
    }

    GraphPoseNodeResult ParameterizedBlendNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( !IsValid() )
        {
            return result;
        }

        //-------------------------------------------------------------------------

        // If this node is synchronized, call the synchronize update
        auto pSettings = GetSettings<RangedBlendNode>();
        if ( pSettings->m_isSynchronized )
        {
            Percentage const deltaPercentage = Percentage( context.m_deltaTime / m_duration );
            Percentage const fromTime = m_currentTime;
            Percentage const toTime = Percentage::Clamp( m_currentTime + deltaPercentage );

            SyncTrackTimeRange UpdateRange;
            UpdateRange.m_startTime = m_blendedSyncTrack.GetTime( fromTime );
            UpdateRange.m_endTime = m_blendedSyncTrack.GetTime( toTime );
            return Update( context, UpdateRange );
        }
        else // Update in a asynchronous manner
        {
            MarkNodeActive( context );
            SelectBlendRange( context );

            auto const& parameterization = GetParameterization();
            BlendRange const& selectedBlendRange = parameterization.m_blendRanges[m_selectedRangeIdx];
            auto pSource0 = m_sourceNodes[selectedBlendRange.m_inputIdx0];
            auto pSource1 = m_sourceNodes[selectedBlendRange.m_inputIdx1];
            EE_ASSERT( pSource0 != nullptr && pSource1 != nullptr );

            //-------------------------------------------------------------------------

            if ( m_blendWeight == 0.0f )
            {
                result = pSource0->Update( context );
                m_duration = pSource0->GetDuration();
            }
            else if ( m_blendWeight == 1.0f )
            {
                result = pSource1->Update( context );
                m_duration = pSource1->GetDuration();
            }
            else
            {
                // Update Source 0
                GraphPoseNodeResult const sourceResult0 = pSource0->Update( context );
                #if EE_DEVELOPMENT_TOOLS
                int16_t const rootMotionActionIdxSource0 = context.GetRootMotionDebugger()->GetLastActionIndex();
                #endif

                // Update Source 1
                GraphPoseNodeResult const sourceResult1 = pSource1->Update( context );
                #if EE_DEVELOPMENT_TOOLS
                int16_t const rootMotionActionIdxSource1 = context.GetRootMotionDebugger()->GetLastActionIndex();
                #endif

                // Update internal time
                m_duration = Math::Lerp( pSource0->GetDuration(), pSource1->GetDuration(), m_blendWeight );

                // Update events
                result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( sourceResult0.m_sampledEventRange, sourceResult1.m_sampledEventRange, m_blendWeight );

                // Do we need to blend between the two nodes?
                if ( sourceResult0.HasRegisteredTasks() && sourceResult1.HasRegisteredTasks() )
                {
                    result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult0.m_taskIdx, sourceResult1.m_taskIdx, m_blendWeight );
                    result.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult0.m_rootMotionDelta, sourceResult1.m_rootMotionDelta, m_blendWeight);

                    #if EE_DEVELOPMENT_TOOLS
                    context.GetRootMotionDebugger()->RecordBlend( GetNodeIndex(), rootMotionActionIdxSource0, rootMotionActionIdxSource1, result.m_rootMotionDelta );
                    #endif
                }
                else // Keep the result that has a pose
                {
                    result = ( sourceResult0.HasRegisteredTasks() ) ? sourceResult0 : sourceResult1;
                }
            }

            // Update internal time
            if ( m_duration > 0 )
            {
                auto const deltaPercentage = Percentage( context.m_deltaTime / m_duration );
                m_previousTime = m_currentTime;
                m_currentTime = m_currentTime + deltaPercentage;
            }
            else
            {
                m_previousTime = m_currentTime = 0.0f;
            }

            // Update time / duration for the remaining source nodes
            // For unsynchronized update, we unfortunately need to update all nodes but we ensure that no unnecessary tasks are registered
            // Ensure that all event weights from the inactive nodes are also set to 0
            //-------------------------------------------------------------------------

            // Clear source nodes ptrs we didn't update
            if ( m_blendWeight == 0.0f )
            {
                pSource1 = nullptr;
            }
            else if ( m_blendWeight == 1.0f )
            {
                pSource0 = nullptr;
            }

            for ( auto pSourceNode : m_sourceNodes )
            {
                if ( pSourceNode != pSource0 && pSourceNode != pSource1 )
                {
                    auto const taskMarker = context.m_pTaskSystem->GetCurrentTaskIndexMarker();
                    GraphPoseNodeResult const transientUpdateResult = pSourceNode->Update( context );
                    context.m_sampledEventsBuffer.MarkEventsAsIgnored( transientUpdateResult.m_sampledEventRange );
                    context.m_pTaskSystem->RollbackToTaskIndexMarker( taskMarker );
                }
            }
        }

        return result;
    }

    GraphPoseNodeResult ParameterizedBlendNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            MarkNodeActive( context );
            SelectBlendRange( context );

            auto const& parameterization = GetParameterization();
            BlendRange const& selectedBlendRange = parameterization.m_blendRanges[m_selectedRangeIdx];
            auto pSource0 = m_sourceNodes[selectedBlendRange.m_inputIdx0];
            auto pSource1 = m_sourceNodes[selectedBlendRange.m_inputIdx1];
            EE_ASSERT( pSource0 != nullptr && pSource1 != nullptr );

            //-------------------------------------------------------------------------

            // For synchronized updates, we can skip the updated for one of the sources if the weight is fully 0 or 1, since we can always update it to the correct time later.
            // Note: for additive blends we always need to update both nodes

            if ( m_blendWeight == 0.0f  )
            {
                result = pSource0->Update( context, updateRange );
            }
            else if ( m_blendWeight == 1.0f )
            {
                result = pSource1->Update( context, updateRange );
            }
            else // We need to update both sources
            {
                // Update Source 0
                GraphPoseNodeResult const sourceResult0 = pSource0->Update( context, updateRange );
                #if EE_DEVELOPMENT_TOOLS
                int16_t const rootMotionActionIdxSource0 = context.GetRootMotionDebugger()->GetLastActionIndex();
                #endif

                // Update Source 1
                GraphPoseNodeResult const sourceResult1 = pSource1->Update( context, updateRange );
                #if EE_DEVELOPMENT_TOOLS
                int16_t const rootMotionActionIdxSource1 = context.GetRootMotionDebugger()->GetLastActionIndex();
                #endif

                if ( sourceResult0.HasRegisteredTasks() && sourceResult1.HasRegisteredTasks() )
                {
                    result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult0.m_taskIdx, sourceResult1.m_taskIdx, m_blendWeight );
                    result.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult0.m_rootMotionDelta, sourceResult1.m_rootMotionDelta, m_blendWeight );

                    #if EE_DEVELOPMENT_TOOLS
                    context.GetRootMotionDebugger()->RecordBlend( GetNodeIndex(), rootMotionActionIdxSource0, rootMotionActionIdxSource1, result.m_rootMotionDelta );
                    #endif
                }
                else
                {
                    result = ( sourceResult0.HasRegisteredTasks() ) ? sourceResult0 : sourceResult1;
                }

                result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( sourceResult0.m_sampledEventRange, sourceResult1.m_sampledEventRange, m_blendWeight );
            }

            // Update internal time and events
            SyncTrack const& syncTrack0 = pSource0->GetSyncTrack();
            SyncTrack const& syncTrack1 = pSource1->GetSyncTrack();
            m_blendedSyncTrack = SyncTrack( syncTrack0, syncTrack1, m_blendWeight );
            m_duration = SyncTrack::CalculateDurationSynchronized( pSource0->GetDuration(), pSource1->GetDuration(), syncTrack0.GetNumEvents(), syncTrack1.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_blendWeight );
            m_previousTime = GetSyncTrack().GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = GetSyncTrack().GetPercentageThrough( updateRange.m_endTime );
        }

        return result;
    }

    //-------------------------------------------------------------------------

    void RangedBlendNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<RangedBlendNode>( context, options );
        ParameterizedBlendNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    //-------------------------------------------------------------------------

    void VelocityBlendNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<VelocityBlendNode>( context, options );
        ParameterizedBlendNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
    }

    void VelocityBlendNode::InitializeParameterization( GraphContext& context )
    {
        if ( IsValid() )
        {
            auto pSettings = GetSettings<VelocityBlendNode>();

            // Get source node speeds
            //-------------------------------------------------------------------------

            TInlineVector<float, 5> values;
            int32_t const numSources = (int32_t) pSettings->m_sourceNodeIndices.size();
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
    }

    void VelocityBlendNode::ShutdownParameterization( GraphContext& context )
    {
        m_parameterization.Reset();
    }
}