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
    void Blend2DNode::CalculateBlendSpaceWeights( TInlineVector<Float2, 10> const &points, TInlineVector<uint8_t, 30> const &indices, TInlineVector<uint8_t, 10> const &hullIndices, Float2 const &point, BlendSpaceResult &result )
    {
        result.Reset();
        result.m_finalParameter = point;

        //-------------------------------------------------------------------------

        bool bEnclosingTriangleFound = false;
        int32_t const numIndices = (int32_t) indices.size();

        for ( int32_t i = 0; i < numIndices; i += 3 )
        {
            uint8_t i0 = indices[i];
            uint8_t i1 = indices[i + 1];
            uint8_t i2 = indices[i + 2];
            Float2 const a = points[i0];
            Float2 const b = points[i1];
            Float2 const c = points[i2];

            // Check if we are inside this triangle, if we are, then calculate the result and early out
            Float3 bcc = Float3::Zero;
            if ( Math::CalculateBarycentricCoordinates( point, a, b, c, bcc ) )
            {
                struct IndexWeight
                {
                    uint8_t m_nIdx;
                    float m_Weight;
                };

                TInlineVector<IndexWeight, 3> indexWeights = { { i0, bcc[0] }, { i1, bcc[1] }, { i2, bcc[2] } };
                eastl::sort( indexWeights.begin(), indexWeights.end(), [] ( IndexWeight const &a, IndexWeight const &b ) { return a.m_Weight < b.m_Weight; } );

                // If one weight is nearly one, we dont need to blend
                if ( Math::IsNearEqual( indexWeights[2].m_Weight, 1.0f, 1.0e-04f ) )
                {
                    result.m_sourceIndices[0] = indexWeights[2].m_nIdx;
                    result.m_sourceIndices[2] = result.m_sourceIndices[1] = InvalidIndex;
                    result.m_blendWeightBetween0And1 = result.m_blendWeightBetween1And2 = 0.0f;
                }
                else // Calculate blend weights
                {
                    result.m_sourceIndices[0] = indexWeights[0].m_nIdx; // lowest weight
                    result.m_sourceIndices[1] = indexWeights[1].m_nIdx;
                    result.m_sourceIndices[2] = indexWeights[2].m_nIdx; // highest weight
                    result.m_blendWeightBetween0And1 = indexWeights[1].m_Weight / ( indexWeights[0].m_Weight + indexWeights[1].m_Weight ); // Calculate weight based on ratio of contribution
                    result.m_blendWeightBetween1And2 = indexWeights[2].m_Weight;
                }

                bEnclosingTriangleFound = true;
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( !bEnclosingTriangleFound )
        {
            struct Edge
            {
                Edge() = default;

                Edge( int32_t nHullIndex, Float2 const &a, Float2 const &b )
                    : m_nStartHullIdx( nHullIndex )
                    , m_segmentStart( a.m_x, a.m_y, 0.0f )
                    , m_segmentEnd( b.m_x, b.m_y, 0.0f )
                {
                }

                void Evaluate( Float2 const &point )
                {
                    LineSegment ls( m_segmentStart, m_segmentEnd );
                    Vector const cp = ls.VectorProjectionOnSegment( point, m_T );
                    m_closestPointOnEdge = cp.ToFloat2();
                }

                inline float GetLength() const
                {
                    return ( m_segmentEnd - m_segmentStart ).GetLength2();
                }

                int32_t			m_nStartHullIdx = InvalidIndex;
                Vector			m_segmentStart = Vector::Zero;
                Vector			m_segmentEnd = Vector::One;
                Float2		    m_closestPointOnEdge;
                float			m_T = 0.0f; // Parameter representing the closest point between the start and the end
            };

            float closestDistance = FLT_MAX;
            Edge closestEdge;

            // Hull has the first index duplicated at the end
            int32_t const numHullPoints = (int32_t) hullIndices.size();
            for ( int32_t i = 1; i < numHullPoints; i++ )
            {
                uint8_t const idx0 = hullIndices[i - 1];
                uint8_t const idx1 = hullIndices[i];

                Edge edge( i - 1, points[idx0], points[idx1] );
                edge.Evaluate( point );

                float const flDistanceToPoint = Vector( edge.m_closestPointOnEdge ).GetDistance2( point );
                if ( flDistanceToPoint < closestDistance )
                {
                    closestDistance = flDistanceToPoint;
                    closestEdge = edge;
                }
            }

            //-------------------------------------------------------------------------

            EE_ASSERT( closestDistance != FLT_MAX );
            result.m_finalParameter = closestEdge.m_closestPointOnEdge;

            // Generate the result for the closest hull edge
            //-------------------------------------------------------------------------

            float const flWeight = closestEdge.m_T;

            result.m_sourceIndices[0] = hullIndices[closestEdge.m_nStartHullIdx];
            result.m_sourceIndices[1] = hullIndices[closestEdge.m_nStartHullIdx + 1];
            result.m_sourceIndices[2] = InvalidIndex;
            result.m_blendWeightBetween0And1 = flWeight;
            result.m_blendWeightBetween1And2 = 0.0f;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( result.m_sourceIndices[0] != InvalidIndex );
        EE_ASSERT( result.m_blendWeightBetween0And1 >= 0 && result.m_blendWeightBetween0And1 <= 1.0f );
        EE_ASSERT( result.m_blendWeightBetween1And2 >= 0 && result.m_blendWeightBetween1And2 < 1.0f ); // 3-way blend is guaranteed to not have a 1.0f weight

        //-------------------------------------------------------------------------

        // Check if we can simplify the first blend
        if ( result.m_sourceIndices[0] != InvalidIndex )
        {
            // If we're fully in source 1, remove the first blend and only keep the second one
            if ( Math::IsNearEqual( result.m_blendWeightBetween0And1, 1.0f ) )
            {
                result.m_sourceIndices[0] = result.m_sourceIndices[1];
                result.m_sourceIndices[1] = result.m_sourceIndices[2];
                result.m_sourceIndices[2] = InvalidIndex;
                result.m_blendWeightBetween0And1 = result.m_blendWeightBetween1And2;
                result.m_blendWeightBetween1And2 = 0.0f;
            }
            // We're fully in the first source so there's nothing to do
            else if ( Math::IsNearZero( result.m_blendWeightBetween0And1 ) )
            {
                if ( result.m_sourceIndices[2] == InvalidIndex )
                {
                    result.m_sourceIndices[1] = InvalidIndex;
                }
                else // Only keep second blend
                {
                    EE_ASSERT( result.m_blendWeightBetween1And2 > 0.0f && result.m_blendWeightBetween1And2 < 1.0f ); // Ensure this cannot be simplified further
                    result.m_sourceIndices[0] = result.m_sourceIndices[1];
                    result.m_sourceIndices[1] = result.m_sourceIndices[2];
                    result.m_sourceIndices[2] = InvalidIndex;
                    result.m_blendWeightBetween0And1 = result.m_blendWeightBetween1And2;
                    result.m_blendWeightBetween1And2 = 0.0f;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void Blend2DNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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

        m_pInputParameterNode0->Initialize( context );
        m_pInputParameterNode1->Initialize( context );

        for ( auto pSourceNode : m_sourceNodes )
        {
            pSourceNode->Initialize( context, initialTime );
        }

        if ( IsValid() )
        {
            EvaluateBlendSpace( context );
            m_previousTime = m_currentTime = m_blendedSyncTrack.GetPercentageThrough( initialTime );
        }
    }

    void Blend2DNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        for ( auto Source : m_sourceNodes )
        {
            Source->Shutdown( context );
        }

        m_pInputParameterNode1->Shutdown( context );
        m_pInputParameterNode0->Shutdown( context );

        //-------------------------------------------------------------------------

        m_bsr.Reset();

        PoseNode::ShutdownInternal( context );
    }

    //-------------------------------------------------------------------------

    void Blend2DNode::EvaluateBlendSpace( GraphContext& context )
    {
        // Ensure we only update the blend space once per update
        if ( context.m_updateID == m_blendSpaceUpdateID )
        {
            return;
        }

        // Calculate the weights
        //-------------------------------------------------------------------------

        auto pDefinition = GetDefinition<Blend2DNode>();
        Float2 const point( m_pInputParameterNode0->GetValue<float>( context ), m_pInputParameterNode1->GetValue<float>( context ) );
        CalculateBlendSpaceWeights( pDefinition->m_values, pDefinition->m_indices, pDefinition->m_hullIndices, point, m_bsr );

        // Calculate blended sync-track and duration
        //-------------------------------------------------------------------------

        // 1-way blend
        if ( m_bsr.m_sourceIndices[1] == InvalidIndex )
        {
            PoseNode *pSource = m_sourceNodes[m_bsr.m_sourceIndices[0]];
            m_blendedSyncTrack = pSource->GetSyncTrack();
            m_duration = pSource->GetDuration();
        }
        else // 2-way blend
        {
            PoseNode *pSource0 = m_sourceNodes[m_bsr.m_sourceIndices[0]];
            PoseNode *pSource1 = m_sourceNodes[m_bsr.m_sourceIndices[1]];

            SyncTrack const& syncTrack0 = pSource0->GetSyncTrack();
            SyncTrack const& syncTrack1 = pSource1->GetSyncTrack();
            m_blendedSyncTrack = SyncTrack( syncTrack0, syncTrack1, m_bsr.m_blendWeightBetween0And1 );
            m_duration = SyncTrack::CalculateDurationSynchronized( pSource0->GetDuration(), pSource1->GetDuration(), syncTrack0.GetNumEvents(), syncTrack1.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_bsr.m_blendWeightBetween0And1 );
        }

        // 3-way blend
        if ( m_bsr.m_sourceIndices[2] != InvalidIndex )
        {
            PoseNode *pSource2 = m_sourceNodes[m_bsr.m_sourceIndices[2]];

            SyncTrack const& syncTrack2 = pSource2->GetSyncTrack();
            Seconds const durationOf2WayBlend = m_duration;
            int32_t const numEventsIn2WayBlendedSyncTrack = m_blendedSyncTrack.GetNumEvents(); // Cache this since it might change when we calculate the new blended result
            m_blendedSyncTrack = SyncTrack( m_blendedSyncTrack, syncTrack2, m_bsr.m_blendWeightBetween1And2 );
            m_duration = SyncTrack::CalculateDurationSynchronized( durationOf2WayBlend, pSource2->GetDuration(), numEventsIn2WayBlendedSyncTrack, syncTrack2.GetNumEvents(), m_blendedSyncTrack.GetNumEvents(), m_bsr.m_blendWeightBetween1And2 );
        }

        m_blendSpaceUpdateID = context.m_updateID;
    }

    GraphPoseNodeResult Blend2DNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( !IsValid() )
        {
            return result;
        }

        #if EE_DEVELOPMENT_TOOLS
        int16_t baseMotionActionIdx = InvalidIndex;
        #endif

        //-------------------------------------------------------------------------

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
            auto* pDefinition = GetDefinition<Blend2DNode>();

            Percentage const deltaPercentage = ( m_duration > 0.0f ) ? Percentage( context.m_deltaTime / m_duration ) : Percentage( 0.0f );
            Percentage const fromTime = m_currentTime;
            Percentage const toTime = Percentage::Clamp( m_currentTime + deltaPercentage, pDefinition->m_allowLooping );

            updateRange.m_startTime = m_blendedSyncTrack.GetTime( fromTime );
            updateRange.m_endTime = m_blendedSyncTrack.GetTime( toTime );
        }

        // Only a single source
        //-------------------------------------------------------------------------

        if ( m_bsr.m_sourceIndices[1] == InvalidIndex )
        {
            PoseNode *pSource = m_sourceNodes[m_bsr.m_sourceIndices[0]];
            result = pSource->Update( context, &updateRange );

            m_duration = pSource->GetDuration();
            m_previousTime = pSource->GetPreviousTime();
            m_currentTime = pSource->GetCurrentTime();
            m_blendedSyncTrack = pSource->GetSyncTrack();
        }

        // 2-Way Blend
        //-------------------------------------------------------------------------

        else
        {
            PoseNode *pSource0 = m_sourceNodes[m_bsr.m_sourceIndices[0]];
            PoseNode *pSource1 = m_sourceNodes[m_bsr.m_sourceIndices[1]];

            // Update Source 0
            GraphPoseNodeResult const sourceResult0 = pSource0->Update( context, &updateRange );
            #if EE_DEVELOPMENT_TOOLS
            int16_t const rootMotionActionIdxSource0 = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update Source 1
            GraphPoseNodeResult const sourceResult1 = pSource1->Update( context, &updateRange );
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

            result.m_sampledEventRange = context.m_pSampledEventsBuffer->BlendEventRanges( sourceResult0.m_sampledEventRange, sourceResult1.m_sampledEventRange, m_bsr.m_blendWeightBetween0And1 );

            // Update internal time and events
            //-------------------------------------------------------------------------

            m_previousTime = m_blendedSyncTrack.GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = m_blendedSyncTrack.GetPercentageThrough( updateRange.m_endTime );
        }

        // 3-Way Blend
        //-------------------------------------------------------------------------

        if ( m_bsr.m_sourceIndices[2] != InvalidIndex )
        {
            GraphPoseNodeResult const baseResult = result;

            PoseNode *pSource2 = m_sourceNodes[m_bsr.m_sourceIndices[2]];

            // Update Source 2
            GraphPoseNodeResult const sourceResult2 = pSource2->Update( context, &updateRange );

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

            result.m_sampledEventRange = context.m_pSampledEventsBuffer->BlendEventRanges( baseResult.m_sampledEventRange, sourceResult2.m_sampledEventRange, m_bsr.m_blendWeightBetween1And2 );

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

        outState.WriteValue( m_bsr.m_sourceIndices[0] );
        outState.WriteValue( m_bsr.m_sourceIndices[1] );
        outState.WriteValue( m_bsr.m_sourceIndices[2] );
        outState.WriteValue( m_bsr.m_blendWeightBetween0And1 );
        outState.WriteValue( m_bsr.m_blendWeightBetween1And2 );
        outState.WriteValue( m_blendedSyncTrack );
    }

    void Blend2DNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        inState.ReadValue( m_bsr.m_sourceIndices[0] );
        inState.ReadValue( m_bsr.m_sourceIndices[1] );
        inState.ReadValue( m_bsr.m_sourceIndices[2] );
        inState.ReadValue( m_bsr.m_blendWeightBetween0And1 );
        inState.ReadValue( m_bsr.m_blendWeightBetween1And2 );
        inState.ReadValue( m_blendedSyncTrack );
    }
    #endif
}
