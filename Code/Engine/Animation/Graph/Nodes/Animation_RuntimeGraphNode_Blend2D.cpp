#include "Animation_RuntimeGraphNode_Blend2D.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Line.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void Blend2DNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<Blend2DNode>( context, options );
        context.SetNodePtrFromIndex( m_inputParameterNodeIdx0, pNode->m_pInputParameterNode0 );
        context.SetNodePtrFromIndex( m_inputParameterNodeIdx1, pNode->m_pInputParameterNode1 );

        pNode->m_sourceNodes.reserve( m_sourceNodeIndices.size() );
        for ( auto sourceIdx : m_sourceNodeIndices )
        {
            context.SetNodePtrFromIndex( sourceIdx, pNode->m_sourceNodes.emplace_back() );
        }
    }

    bool Blend2DNode::IsValid() const
    {
        if ( !PoseNode::IsValid() )
        {
            return false;
        }

        if ( !m_pInputParameterNode0->IsValid() )
        {
            return false;
        }

        if ( !m_pInputParameterNode1->IsValid() )
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

    void Blend2DNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pInputParameterNode0 != nullptr && m_pInputParameterNode1 != nullptr && m_sourceNodes.size() > 1 );

        PoseNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        if ( IsValid() )
        {
            m_pInputParameterNode0->Initialize( context );
            m_pInputParameterNode1->Initialize( context );

            for ( auto pSourceNode : m_sourceNodes )
            {
                pSourceNode->Initialize( context, initialTime );
            }

            //-------------------------------------------------------------------------

            EvaluateBlendSpace( context );

            m_previousTime = m_currentTime = m_blendedSyncTrack.GetPercentageThrough( initialTime );
        }
    }

    void Blend2DNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( IsValid() )
        {
            for ( auto Source : m_sourceNodes )
            {
                Source->Shutdown( context );
            }

            m_pInputParameterNode1->Shutdown( context );
            m_pInputParameterNode0->Shutdown( context );
        }

        m_bsr.Reset();

        PoseNode::ShutdownInternal( context );
    }

    //-------------------------------------------------------------------------

    void Blend2DNode::EvaluateBlendSpace( GraphContext& context )
    {
        // Ensure we only update the blend space once per update
        //-------------------------------------------------------------------------

        if ( context.m_updateID == m_bsr.m_updateID )
        {
            return;
        }

        m_bsr.m_updateID = context.m_updateID;

        //-------------------------------------------------------------------------

        auto pSettings = GetSettings<Blend2DNode>();

        Float2 const point( m_pInputParameterNode0->GetValue<float>( context ), m_pInputParameterNode1->GetValue<float>( context ) );

        #if EE_DEVELOPMENT_TOOLS
        m_bsr.m_parameter = point;
        #endif

        //-------------------------------------------------------------------------
        // Check all triangles
        //-------------------------------------------------------------------------

        bool enclosingTriangleFound = false;

        int32_t const numIndices = (int32_t) pSettings->m_indices.size();

        for ( int32_t i = 0; i < numIndices; i += 3 )
        {
            uint8_t i0 = pSettings->m_indices[i];
            uint8_t i1 = pSettings->m_indices[i + 1];
            uint8_t i2 = pSettings->m_indices[i + 2];
            Vector const& a = pSettings->m_values[i0];
            Vector const& b = pSettings->m_values[i1];
            Vector const& c = pSettings->m_values[i2];

            // Check if we are inside this triangle, if we are, then calculate the result and early out
            Float3 bcc;
            if ( Math::CalculateBarycentricCoordinates( point, a, b, c, bcc ) )
            {
                TInlineVector<TPair<uint8_t, float>, 3> indexWeights = { { i0, bcc[0] }, { i1, bcc[1] }, { i2, bcc[2] } };
                eastl::sort( indexWeights.begin(), indexWeights.end(), [] ( TPair<uint8_t, float> const& a, TPair<uint8_t, float> const& b ) { return a.second < b.second; } );

                // If one weight is nearly one, we dont need to blend
                if ( Math::IsNearEqual( indexWeights[2].second, 1.0f ) )
                {
                    m_bsr.m_pSource0 = m_sourceNodes[indexWeights[2].first];
                    m_bsr.m_pSource1 = m_bsr.m_pSource2 = nullptr;
                    m_bsr.m_blendWeightBetween0And1 =  m_bsr.m_blendWeightBetween1And2 = 0.0f;
                }
                else // Calculate blend weights
                {
                    m_bsr.m_pSource0 = m_sourceNodes[indexWeights[0].first]; // lowest weight
                    m_bsr.m_pSource1 = m_sourceNodes[indexWeights[1].first];
                    m_bsr.m_pSource2 = m_sourceNodes[indexWeights[2].first]; // highest weight
                    m_bsr.m_blendWeightBetween0And1 = indexWeights[1].second / ( indexWeights[0].second + indexWeights[1].second ); // Calculate weight based on ratio of contribution
                    m_bsr.m_blendWeightBetween1And2 = indexWeights[2].second;
                }

                enclosingTriangleFound = true;
                break;
            }
        }

        //-------------------------------------------------------------------------
        // Project onto hull
        //-------------------------------------------------------------------------

        if ( !enclosingTriangleFound )
        {
            struct Edge
            {
                Edge() = default;

                Edge( Vector const& a, Vector const& b )
                    : m_segment( a, b )
                {}

                int32_t         m_startHullIdx = InvalidIndex;
                LineSegment     m_segment = LineSegment( Vector::Zero, Vector::One );
                float           m_scalarProjection = 0.0f;
            };

            float closestDistance = FLT_MAX;
            Edge closestEdge;

            // Hull has the first index duplicated at the end
            int32_t const numHullPoints = (int32_t) pSettings->m_hullIndices.size();
            for ( int32_t i = 1; i < numHullPoints; i++ )
            {
                uint8_t const idx0 = pSettings->m_hullIndices[i-1];
                uint8_t const idx1 = pSettings->m_hullIndices[i];

                Edge edge( pSettings->m_values[idx0], pSettings->m_values[idx1] );
                edge.m_startHullIdx = i - 1;
                edge.m_scalarProjection = edge.m_segment.ScalarProjectionOnSegment( point );

                Vector const closestPointOnEdge = edge.m_segment.GetPointAlongLine( edge.m_scalarProjection );
                float const distanceToPoint = closestPointOnEdge.GetDistance2( point );
                if ( distanceToPoint < closestDistance )
                {
                    closestDistance = distanceToPoint;
                    closestEdge = edge;
                }
            }

            //-------------------------------------------------------------------------

            EE_ASSERT( closestDistance != FLT_MAX );

            #if EE_DEVELOPMENT_TOOLS
            m_bsr.m_parameter = closestEdge.m_segment.GetPointAlongLine( closestEdge.m_scalarProjection );
            #endif

            // Generate the result for the closest hull edge
            //-------------------------------------------------------------------------

            float const weight = closestEdge.m_scalarProjection / closestEdge.m_segment.GetLength();

            m_bsr.m_pSource0 = m_sourceNodes[pSettings->m_hullIndices[closestEdge.m_startHullIdx]];
            m_bsr.m_pSource1 = m_sourceNodes[pSettings->m_hullIndices[closestEdge.m_startHullIdx + 1]];
            m_bsr.m_pSource2 = nullptr;
            m_bsr.m_blendWeightBetween0And1 = weight;
            m_bsr.m_blendWeightBetween1And2 = 0.0f;
        }

        //-------------------------------------------------------------------------
        // Finalize result
        //-------------------------------------------------------------------------

        EE_ASSERT( m_bsr.m_pSource0 != nullptr );
        EE_ASSERT( m_bsr.m_blendWeightBetween0And1 >= 0 && m_bsr.m_blendWeightBetween0And1 <= 1.0f );
        EE_ASSERT( m_bsr.m_blendWeightBetween1And2 >= 0 && m_bsr.m_blendWeightBetween1And2 < 1.0f ); // 3-way blend is guaranteed to not have a 1.0f weight

        //-------------------------------------------------------------------------

        // Check if we can simplify the first blend
        if ( m_bsr.m_pSource1 != nullptr )
        {
            // If we're fully in source 1, remove the first blend and only keep the second one
            if ( m_bsr.m_blendWeightBetween0And1 == 1.0f )
            {
                m_bsr.m_pSource0 = m_bsr.m_pSource1;
                m_bsr.m_pSource1 = m_bsr.m_pSource2;
                m_bsr.m_pSource2 = nullptr;
                m_bsr.m_blendWeightBetween0And1 = m_bsr.m_blendWeightBetween1And2;
                m_bsr.m_blendWeightBetween1And2 = 0.0f;
            }
            // We're fully in the first source so there's nothing to do
            else if ( m_bsr.m_blendWeightBetween0And1 == 0.0f )
            {
                if ( m_bsr.m_pSource2 == nullptr )
                {
                    m_bsr.m_pSource1 = nullptr;
                }
                else // Only keep second blend
                {
                    EE_ASSERT( m_bsr.m_blendWeightBetween1And2 > 0.0f && m_bsr.m_blendWeightBetween1And2 < 1.0f ); // Ensure this cannot be simplified further
                    m_bsr.m_pSource0 = m_bsr.m_pSource1;
                    m_bsr.m_pSource1 = m_bsr.m_pSource2;
                    m_bsr.m_pSource2 = nullptr;
                    m_bsr.m_blendWeightBetween0And1 = m_bsr.m_blendWeightBetween1And2;
                    m_bsr.m_blendWeightBetween1And2 = 0.0f;
                }
            }
        }

        // Calculate blended sync-track and duration
        //-------------------------------------------------------------------------

        // 1-way blend
        if ( m_bsr.m_pSource1 == nullptr )
        {
            m_blendedSyncTrack = m_bsr.m_pSource0->GetSyncTrack();
            m_duration = m_bsr.m_pSource0->GetDuration();
        }
        else // 2-way blend
        {
            SyncTrack const& syncTrack0 = m_bsr.m_pSource0->GetSyncTrack();
            SyncTrack const& syncTrack1 = m_bsr.m_pSource1->GetSyncTrack();
            m_blendedSyncTrack = SyncTrack( syncTrack0, syncTrack1, m_bsr.m_blendWeightBetween0And1 );
            m_duration = SyncTrack::CalculateDurationSynchronized( m_bsr.m_pSource0->GetDuration(), m_bsr.m_pSource1->GetDuration(), syncTrack0.GetNumEvents(), syncTrack1.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_bsr.m_blendWeightBetween0And1 );
        }

        // 3-way blend
        if ( m_bsr.m_pSource2 != nullptr )
        {
            SyncTrack const& syncTrack2 = m_bsr.m_pSource2->GetSyncTrack();
            Seconds const durationOf2WayBlend = m_duration;
            int32_t const numEventsIn2WayBlendedSyncTrack = m_blendedSyncTrack.GetNumEvents(); // Cache this since it might change when we calculate the new blended result
            m_blendedSyncTrack = SyncTrack( m_blendedSyncTrack, syncTrack2, m_bsr.m_blendWeightBetween1And2 );
            m_duration = SyncTrack::CalculateDurationSynchronized( durationOf2WayBlend, m_bsr.m_pSource2->GetDuration(), numEventsIn2WayBlendedSyncTrack, syncTrack2.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_bsr.m_blendWeightBetween1And2 );
        }
    }

    GraphPoseNodeResult Blend2DNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( !IsValid() )
        {
            return result;
        }

        //-------------------------------------------------------------------------

        EvaluateBlendSpace( context );

        Percentage const deltaPercentage = ( m_duration > 0.0f ) ? Percentage( context.m_deltaTime / m_duration ) : 0.0f;
        Percentage const fromTime = m_currentTime;
        Percentage const toTime = Percentage::Clamp( m_currentTime + deltaPercentage );

        SyncTrackTimeRange updateRange;
        updateRange.m_startTime = m_blendedSyncTrack.GetTime( fromTime );
        updateRange.m_endTime = m_blendedSyncTrack.GetTime( toTime );
        result = Update( context, updateRange );

        //-------------------------------------------------------------------------

        return result;
    }

    GraphPoseNodeResult Blend2DNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( !IsValid() )
        {
            return result;
        }

        //-------------------------------------------------------------------------

        MarkNodeActive( context );
        EvaluateBlendSpace( context );

        #if EE_DEVELOPMENT_TOOLS
        int16_t baseMotionActionIdx = InvalidIndex;
        #endif

        // Only a single source
        //-------------------------------------------------------------------------

        if ( m_bsr.m_pSource1 == nullptr )
        {
            result = m_bsr.m_pSource0->Update( context, updateRange );

            m_duration = m_bsr.m_pSource0->GetDuration();
            m_previousTime = m_bsr.m_pSource0->GetPreviousTime();
            m_currentTime = m_bsr.m_pSource0->GetCurrentTime();
            m_blendedSyncTrack = m_bsr.m_pSource0->GetSyncTrack();
        }

        // 2-Way Blend
        //-------------------------------------------------------------------------

        else
        {
            // Update Source 0
            GraphPoseNodeResult const sourceResult0 = m_bsr.m_pSource0->Update( context, updateRange );
            #if EE_DEVELOPMENT_TOOLS
            int16_t const rootMotionActionIdxSource0 = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update Source 1
            GraphPoseNodeResult const sourceResult1 = m_bsr.m_pSource1->Update( context, updateRange );
            #if EE_DEVELOPMENT_TOOLS
            int16_t const rootMotionActionIdxSource1 = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Register Tasks
            //-------------------------------------------------------------------------

            if ( sourceResult0.HasRegisteredTasks() && sourceResult1.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult0.m_taskIdx, sourceResult1.m_taskIdx, m_bsr.m_blendWeightBetween0And1 );
                result.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult0.m_rootMotionDelta, sourceResult1.m_rootMotionDelta, m_bsr.m_blendWeightBetween0And1 );

                #if EE_DEVELOPMENT_TOOLS
                baseMotionActionIdx = context.GetRootMotionDebugger()->RecordBlend( GetNodeIndex(), rootMotionActionIdxSource0, rootMotionActionIdxSource1, result.m_rootMotionDelta );
                #endif
            }
            else
            {
                result = ( sourceResult0.HasRegisteredTasks() ) ? sourceResult0 : sourceResult1;
            }

            result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( sourceResult0.m_sampledEventRange, sourceResult1.m_sampledEventRange, m_bsr.m_blendWeightBetween0And1 );

            // Update internal time and events
            //-------------------------------------------------------------------------

            m_previousTime = m_blendedSyncTrack.GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = m_blendedSyncTrack.GetPercentageThrough( updateRange.m_endTime );
        }

        // 3-Way Blend
        //-------------------------------------------------------------------------

        if ( m_bsr.m_pSource2 != nullptr )
        {
            GraphPoseNodeResult const baseResult = result;

            // Update Source 2
            GraphPoseNodeResult const sourceResult2 = m_bsr.m_pSource2->Update( context, updateRange );
            #if EE_DEVELOPMENT_TOOLS
            int16_t const rootMotionActionIdxSource2 = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Register Tasks
            //-------------------------------------------------------------------------

            if ( baseResult.HasRegisteredTasks() && sourceResult2.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), baseResult.m_taskIdx, sourceResult2.m_taskIdx, m_bsr.m_blendWeightBetween1And2 );
                result.m_rootMotionDelta = Blender::BlendRootMotionDeltas( baseResult.m_rootMotionDelta, sourceResult2.m_rootMotionDelta, m_bsr.m_blendWeightBetween1And2 );

                #if EE_DEVELOPMENT_TOOLS
                context.GetRootMotionDebugger()->RecordBlend( GetNodeIndex(), baseMotionActionIdx, rootMotionActionIdxSource2, result.m_rootMotionDelta );
                #endif
            }
            else
            {
                result = ( baseResult.HasRegisteredTasks() ) ? baseResult : sourceResult2;
            }

            result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( baseResult.m_sampledEventRange, sourceResult2.m_sampledEventRange, m_bsr.m_blendWeightBetween1And2 );

            // Update internal time and events
            //-------------------------------------------------------------------------

            // Nothing to do as we've already set the correct time as part of the 2-way blend
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Blend2DNode::RecordGraphState( RecordedGraphState& outState )
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
        outState.WriteValue( FindSourceIndex( m_bsr.m_pSource2 ) );
        outState.WriteValue( m_bsr.m_blendWeightBetween0And1 );
        outState.WriteValue( m_bsr.m_blendWeightBetween1And2 );
    }

    void Blend2DNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        int32_t idx = InvalidIndex;

        inState.ReadValue( idx );
        m_bsr.m_pSource0 = idx != InvalidIndex ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( idx );
        m_bsr.m_pSource1 = idx != InvalidIndex ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( idx );
        m_bsr.m_pSource2 = idx != InvalidIndex ? m_sourceNodes[idx] : nullptr;

        inState.ReadValue( m_bsr.m_blendWeightBetween0And1 );
        inState.ReadValue( m_bsr.m_blendWeightBetween1And2 );
    }
    #endif
}
