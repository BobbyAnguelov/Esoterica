#include "Animation_RuntimeGraph_Recording.h"
#include "Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/AnimationClip.h"
#include "Nodes/Animation_RuntimeGraphNode_ReferencedGraph.h"
#include "Nodes/Animation_RuntimeGraphNode_ExternalPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    RecordedGraphState::~RecordedGraphState()
    {
        EE_ASSERT( m_pNodes == nullptr );
        Reset();
    }

    void RecordedGraphState::Reset()
    {
        for ( auto pReferencedGraphData : m_referencedGraphData )
        {
            EE::Delete( pReferencedGraphData );
        }

        m_referencedGraphData.clear();
        m_initializedNodeIndices.clear();
        m_inputArchive.Reset();
        m_outputArchive.Reset();
    }

    void RecordedGraphState::PrepareForReading()
    {
        m_inputArchive.ReadFromData( m_outputArchive.GetBinaryData(), m_outputArchive.GetBinaryDataSize() );

        for ( RecordedReferencedGraphData* pReferencedGraphData : m_referencedGraphData )
        {
            pReferencedGraphData->m_graphState.m_inputArchive.ReadFromData( pReferencedGraphData->m_graphState.m_outputArchive.GetBinaryData(), pReferencedGraphData->m_graphState.m_outputArchive.GetBinaryDataSize() );
        }
    }

    static void GetGraphIDs( RecordedGraphState const& state, TVector<ResourceID>& outGraphIDs )
    {
        outGraphIDs.emplace_back( state.m_graphResourceID );

        for ( RecordedReferencedGraphData const* pReferencedGraphData : state.m_referencedGraphData )
        {
            GetGraphIDs( pReferencedGraphData->m_graphState, outGraphIDs );
        }
    }

    void RecordedGraphState::GetAllRecordedGraphResourceIDs( TVector<ResourceID>& outGraphIDs ) const
    {
        GetGraphIDs( *this, outGraphIDs );
    }

    //-------------------------------------------------------------------------

    RecordedGraphState* RecordedGraphState::CreateReferencedGraphStateRecording( int16_t referencedGraphNodeIdx )
    {
        EE_ASSERT( referencedGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, referencedGraphNodeIdx ) ); // If this fires it means we recording an uninitialized referenced graph!
        EE_ASSERT( !VectorContains( m_referencedGraphData, referencedGraphNodeIdx, [] ( RecordedReferencedGraphData const* pRGD, int16_t childGraphNodeIdx ) { return pRGD->m_referencedGraphNodeIdx == childGraphNodeIdx; } ) ); // Ensure we dont record the same graph twice

        RecordedReferencedGraphData *pReferencedGraphData = EE::New<RecordedReferencedGraphData>();
        pReferencedGraphData->m_referencedGraphNodeIdx = referencedGraphNodeIdx;
        m_referencedGraphData.emplace_back( pReferencedGraphData );

        return &pReferencedGraphData->m_graphState;
    }

    RecordedGraphState* RecordedGraphState::GetReferencedGraphRecording( int16_t referencedGraphNodeIdx )
    {
        EE_ASSERT( referencedGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, referencedGraphNodeIdx ) ); // If this fires it means we restoring an uninitialized referenced graph!

        auto foundIter = VectorFind( m_referencedGraphData, referencedGraphNodeIdx, [] ( RecordedReferencedGraphData const* pRGD, int16_t childGraphNodeIdx ) { return pRGD->m_referencedGraphNodeIdx == childGraphNodeIdx; } );
        EE_ASSERT( foundIter != m_referencedGraphData.end() ); // Ensure we have the desired data
        return &( *foundIter )->m_graphState;
    }

    //-------------------------------------------------------------------------

    RecordedGraphUpdateData::~RecordedGraphUpdateData()
    {
        for ( auto pExternalGraphData : m_externalGraphData )
        {
            EE::Delete( pExternalGraphData );
        }
        m_externalGraphData.clear();

        for ( auto pExternalPoseData : m_externalPoseData )
        {
            EE::Delete( pExternalPoseData );
        }
        m_externalPoseData.clear();

        m_timingInfo.Reset();
    }

    RecordedExternalGraphData *RecordedGraphUpdateData::CreateExternalGraphData()
    {
        auto pData = EE::New<RecordedExternalGraphData>();
        m_externalGraphData.emplace_back( pData );
        return pData;
    }

    RecordedExternalGraphData *RecordedGraphUpdateData::TryGetExternalGraphData( int16_t nodeIdx )
    {
        for ( auto &pExternalGraphData : m_externalGraphData )
        {
            if ( pExternalGraphData->m_externalGraphNodeIdx == nodeIdx )
            {
                return pExternalGraphData;
            }
        }

        return nullptr;
    }

    RecordedExternalPoseData *RecordedGraphUpdateData::CreateExternalPoseData()
    {
        auto pData = EE::New<RecordedExternalPoseData>();
        m_externalPoseData.emplace_back( pData );
        return pData;
    }

    void RecordedGraphUpdateData::SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons )
    {
        m_secondarySkeletons.clear();

        for ( auto const pSkeleton : secondarySkeletons )
        {
            m_secondarySkeletons.emplace_back( pSkeleton->GetResourceID() );
        }
    }

    //-------------------------------------------------------------------------
    // Recording
    //-------------------------------------------------------------------------

    void GraphRecording::Reset()
    {
        m_graphID.Clear();
        m_variationID = StringID();

        //-------------------------------------------------------------------------

        for ( auto pRecordedUpdateData : m_recordedUpdateData )
        {
            EE::Delete( pRecordedUpdateData );
        }
        m_recordedUpdateData.clear();

        for ( auto pRecordedGraphState : m_recordedGraphStates )
        {
            EE::Delete( pRecordedGraphState );
        }

        m_recordedGraphStates.clear();
    }

    bool GraphRecording::HasRecordedDataForGraph( ResourceID const& graphResourceID ) const
    {
        if ( m_recordedUpdateData.empty() )
        {
            return false;
        }

        EE_ASSERT( m_recordedUpdateData[0]->m_updateType == RecordedUpdateType::FirstRecording );
        EE_ASSERT( m_recordedUpdateData[0]->m_recordedGraphStateIdx != -1 );

        TVector<ResourceID> graphResourceIDs;
        m_recordedGraphStates[m_recordedUpdateData[0]->m_recordedGraphStateIdx]->GetAllRecordedGraphResourceIDs( graphResourceIDs );
        return VectorContains( graphResourceIDs, graphResourceID );
    }

    bool GraphRecording::IsRecordedDataForAStandaloneGraph() const
    {
        if ( m_recordedUpdateData.empty() )
        {
            return false;
        }

        EE_ASSERT( m_recordedUpdateData[0]->m_updateType == RecordedUpdateType::FirstRecording );
        EE_ASSERT( m_recordedUpdateData[0]->m_recordedGraphStateIdx != -1 );

        return m_recordedGraphStates[m_recordedUpdateData[0]->m_recordedGraphStateIdx]->m_isStandaloneGraph;
    }

    TInlineVector<ResourceID, 2> GraphRecording::GetAllRecordedGraphResourceIDs() const
    {
        EE_ASSERT( HasRecordedData() );

        TInlineVector<ResourceID, 2> graphResourceIDs;

        for ( RecordedGraphState const* pRecordedGraphState : m_recordedGraphStates )
        {
            VectorEmplaceBackUnique( graphResourceIDs, pRecordedGraphState->m_graphResourceID );
        }

        return graphResourceIDs;
    }

    TInlineVector<ResourceID, 2> GraphRecording::GetAllRecordedExternalClipResourceIDs() const
    {
        EE_ASSERT( HasRecordedData() );

        TInlineVector<ResourceID, 2> clipResourceIDs;

        for ( RecordedGraphUpdateData const* pRecordedUpdateData : m_recordedUpdateData )
        {
            for ( RecordedExternalPoseData const* pRecordedPoseData : pRecordedUpdateData->m_externalPoseData )
            {
                if ( pRecordedPoseData->m_clipResourceID0.IsValid() )
                {
                    VectorEmplaceBackUnique( clipResourceIDs, pRecordedPoseData->m_clipResourceID0 );
                }

                if ( pRecordedPoseData->m_clipResourceID1.IsValid() )
                {
                    VectorEmplaceBackUnique( clipResourceIDs, pRecordedPoseData->m_clipResourceID1 );
                }
            }
        }

        return clipResourceIDs;
    }

    TInlineVector<ResourceID, 2> GraphRecording::GetAllRecordedSkeletonResourceIDs() const
    {
        EE_ASSERT( HasRecordedData() );

        TInlineVector<ResourceID, 2> skeletonResourceIDs;

        for ( RecordedGraphUpdateData const* pRecordedUpdateData : m_recordedUpdateData )
        {
            for ( ResourceID const &skeletonID : pRecordedUpdateData->m_secondarySkeletons )
            {
                VectorEmplaceBackUnique( skeletonResourceIDs, skeletonID );
            }
        }

        return skeletonResourceIDs;
    }

    //-------------------------------------------------------------------------
    // Recording Playback
    //-------------------------------------------------------------------------

    GraphRecordingPlayer::~GraphRecordingPlayer()
    {
        EE_ASSERT( m_externalGraphInstances.empty() );
    }

    void GraphRecordingPlayer::StartPlayback( GraphInstance *pPlaybackGraphInstance, TInlineVector<GraphDefinition const*, 2> const& externalGraphDefs, TInlineVector<AnimationClip const*, 2> const& externalClips, GraphRecording *pRecording )
    {
        EE_ASSERT( pPlaybackGraphInstance != nullptr );
        EE_ASSERT( pRecording != nullptr && pRecording->HasRecordedData() );
        EE_ASSERT( pRecording->m_graphID == pRecording->m_graphID );
        EE_ASSERT( m_pPlaybackGraphInstance == nullptr );
        EE_ASSERT( m_pRecording == nullptr );

        m_pPlaybackGraphInstance = pPlaybackGraphInstance;
        m_pRecording = pRecording;
        m_currentViewedUpdateIdx = InvalidIndex;
        m_taskListTopologySize = m_taskListDataSize = 0;
        m_characterTransform = Transform::Identity;
        m_externalGraphDefinitions = externalGraphDefs;
        m_externalClips = externalClips;

        GoToRecordedUpdate( 0 );
    }

    void GraphRecordingPlayer::StopPlayback()
    {
        EE_ASSERT( m_pPlaybackGraphInstance != nullptr );
        EE_ASSERT( m_pRecording != nullptr );

        m_pPlaybackGraphInstance->DisconnectAllExternalGraphs();
        m_pPlaybackGraphInstance->ClearAllExternalPoses();

        m_pPlaybackGraphInstance = nullptr;

        m_pRecording = nullptr;
        m_currentViewedUpdateIdx = InvalidIndex;
        m_taskListTopologySize = m_taskListDataSize = 0;
        m_characterTransform = Transform::Identity;

        DestroyCreatedExternalGraphInstances();
    }

    Seconds GraphRecordingPlayer::GetDeltaTime() const
    {
        EE_ASSERT( IsPlaybackActive() && m_pRecording->HasRecordedData() );
        EE_ASSERT( m_currentViewedUpdateIdx >= 0 && m_currentViewedUpdateIdx < m_pRecording->GetNumRecordedUpdates() );

        return m_pRecording->m_recordedUpdateData[m_currentViewedUpdateIdx]->m_deltaTime;
    }

    RecordedUpdateType GraphRecordingPlayer::GetUpdateType() const
    {
        EE_ASSERT( IsPlaybackActive() && m_pRecording->HasRecordedData() );
        EE_ASSERT( m_currentViewedUpdateIdx >= 0 && m_currentViewedUpdateIdx < m_pRecording->GetNumRecordedUpdates() );

        return m_pRecording->m_recordedUpdateData[m_currentViewedUpdateIdx]->m_updateType;
    }

    GraphTimeInfo const &GraphRecordingPlayer::GetTimingInfo() const
    {
        EE_ASSERT( IsPlaybackActive() && m_pRecording->HasRecordedData() );
        EE_ASSERT( m_currentViewedUpdateIdx >= 0 && m_currentViewedUpdateIdx < m_pRecording->GetNumRecordedUpdates() );

        return m_pRecording->m_recordedUpdateData[m_currentViewedUpdateIdx]->m_timingInfo;
    }

    void GraphRecordingPlayer::DestroyCreatedExternalGraphInstances()
    {
        for ( auto &externalInstance : m_externalGraphInstances )
        {
            EE::Delete( externalInstance.m_pInstance );
            externalInstance.m_pInstance = nullptr;
        }

        m_externalGraphInstances.clear();
    }

    AnimationClip const* GraphRecordingPlayer::TryGetExternalClip( ResourceID const& ID ) const
    {
        for ( auto pClip : m_externalClips )
        {
            if ( pClip->GetResourceID() == ID )
            {
                return pClip;
            }
        }

        return nullptr;
    }

    GraphRecordingPlayer::StepResult GraphRecordingPlayer::StepRecording( int32_t steps )
    {
        EE_ASSERT( IsPlaybackActive() && m_pRecording->HasRecordedData() );

        bool hasExternalGraphStateChanged = false;

        auto DisconnectGraphFromNode = [this, &hasExternalGraphStateChanged] ( GraphDefinition::ExternalGraphSlot const &eg )
        {
            m_pPlaybackGraphInstance->DisconnectExternalGraph( eg.m_slotID );

            for ( int32_t e = 0; e < m_externalGraphInstances.size(); e++ )
            {
                if ( m_externalGraphInstances[e].m_nodeIdx == eg.m_nodeIdx )
                {
                    delete m_externalGraphInstances[e].m_pInstance;
                    m_externalGraphInstances.erase( m_externalGraphInstances.begin() + e );
                    break;
                }
            }

            hasExternalGraphStateChanged = true;
        };

        auto ConnectGraphToNode = [this, &hasExternalGraphStateChanged] ( RecordedExternalGraphData const *pExternalGraphData, GraphDefinition::ExternalGraphSlot const &eg )
        {
            int32_t const definitionIdx = VectorFindIndex( m_externalGraphDefinitions, pExternalGraphData->m_graphResourceID, [] ( GraphDefinition const* pDef, ResourceID const& resID ) { return pDef->GetResourceID() == resID; } );
            if ( definitionIdx == InvalidIndex )
            {
                return false;
            }

            auto pGraphState = m_pRecording->m_recordedGraphStates[pExternalGraphData->m_updateData.m_recordedGraphStateIdx];
            pGraphState->PrepareForReading();

            GraphInstance *pExternalInstance = new GraphInstance( m_externalGraphDefinitions[definitionIdx] );
            if ( !pExternalInstance->RestoreRecordedGraphState( *pGraphState ) )
            {
                return false;
            }

            m_pPlaybackGraphInstance->ConnectExternalGraph( eg.m_slotID, pExternalInstance );
            m_externalGraphInstances.emplace_back( eg.m_nodeIdx, pExternalInstance );

            hasExternalGraphStateChanged = true;

            return true;
        };

        //-------------------------------------------------------------------------

        bool const isStartingFromBeginning = ( m_currentViewedUpdateIdx == 0 );
        int32_t const startUpdateIdx = isStartingFromBeginning ? 0 : ( m_currentViewedUpdateIdx + 1 );
        int32_t const endUpdateIdx = m_currentViewedUpdateIdx + steps;
        EE_ASSERT( endUpdateIdx >= 0 && endUpdateIdx < m_pRecording->GetNumRecordedUpdates() );

        for ( auto i = startUpdateIdx; i <= endUpdateIdx; i++ )
        {
            RecordedGraphUpdateData const *pUpdateData = m_pRecording->m_recordedUpdateData[i];

            // Restore the main graph state
            //-------------------------------------------------------------------------

            if ( !m_pPlaybackGraphInstance->SetToRecordedState( *pUpdateData, m_pRecording->m_recordedGraphStates ) )
            {
                return StepResult::Failure;
            }

            // Manage external pose data
            //-------------------------------------------------------------------------

            for ( RecordedExternalPoseData const *pEPD : pUpdateData->m_externalPoseData )
            {
                /*auto pExternalGraphNode = static_cast<ExternalPoseNode*>( m_pPlaybackGraphInstance->m_nodes[pEPD->m_externalPoseNodeIdx] );

                ExternalPoseData poseData = pExternalGraphNode->m_poseData;

                poseData.m_pClip0 = TryGetExternalClip( pEPD->m_clipResourceID0 );
                poseData.m_startTime0 = pEPD->m_startTime0;
                poseData.m_endTime0 = pEPD->m_endTime0;

                poseData.m_pClip1 = TryGetExternalClip( pEPD->m_clipResourceID1 );
                poseData.m_startTime1 = pEPD->m_startTime1;
                poseData.m_endTime1 = pEPD->m_endTime1;

                poseData.m_blendWeight = pEPD->m_blendWeight;

                EE_ASSERT( poseData.IsValid( m_pPlaybackGraphInstance->GetPrimarySkeleton() ) );
                m_pPlaybackGraphInstance->SetExternalPose( pEPD->m_slotID, poseData );*/
            }

            // Manage external graph instances
            //-------------------------------------------------------------------------

            auto const &pGraphDefinition = m_pPlaybackGraphInstance->GetGraphDefinition();
            TVector<GraphDefinition::ExternalGraphSlot> const &externalGraphSlots = pGraphDefinition->GetExternalGraphSlots();

            bool const hasExternalGraphData = !pUpdateData->m_externalGraphData.empty();
            if ( ( hasExternalGraphData && pUpdateData->m_updateType == RecordedUpdateType::FirstRecording ) || pUpdateData->m_updateType == RecordedUpdateType::ExternalGraphChanged )
            {
                for ( GraphDefinition::ExternalGraphSlot const &eg : externalGraphSlots )
                {
                    bool const isExternalSlotFilled = m_pPlaybackGraphInstance->IsExternalGraphSlotFilled( eg.m_slotID );
                    auto const pExternalGraphData = pUpdateData->TryGetExternalGraphData( eg.m_nodeIdx );

                    // We dont have external data but there is a graph connected, then disconnect it
                    if ( pExternalGraphData == nullptr && isExternalSlotFilled )
                    {
                        DisconnectGraphFromNode( eg );
                    }
                    // We have external data but no connected graph, so connect a graph
                    else if ( pExternalGraphData != nullptr && !isExternalSlotFilled )
                    {
                        if ( !ConnectGraphToNode( pExternalGraphData, eg ) )
                        {
                            return StepResult::Failure;
                        }
                    }
                    // Both are set
                    else if ( pExternalGraphData != nullptr && isExternalSlotFilled )
                    {
                        // Ensure that the graph defs match what is expected
                        auto pReferenceNode = static_cast<ReferencedGraphNode*>( m_pPlaybackGraphInstance->m_nodes[eg.m_nodeIdx] );
                        if ( pReferenceNode->GetGraphDefinition()->GetResourceID() != pExternalGraphData->m_graphResourceID )
                        {
                            DisconnectGraphFromNode( eg );

                            if ( !ConnectGraphToNode( pExternalGraphData, eg ) )
                            {
                                return StepResult::Failure;
                            }
                        }
                    }
                }
            }

            // Update external graph instance parameters
            //-------------------------------------------------------------------------

            if ( pUpdateData->m_updateType == RecordedUpdateType::TimeStep )
            {
                for ( auto const &pEGD : pUpdateData->m_externalGraphData )
                {
                    auto pExternalGraphNode = static_cast<ReferencedGraphNode*>( m_pPlaybackGraphInstance->m_nodes[pEGD->m_externalGraphNodeIdx] );
                    EE_ASSERT( pExternalGraphNode->IsExternalGraphSlot() && pExternalGraphNode->HasInstance() );

                    bool bSetParametersForExternalGraphInstance = false;

                    for ( auto &eg : m_externalGraphInstances )
                    {
                        if ( eg.m_nodeIdx == pEGD->m_externalGraphNodeIdx )
                        {
                            eg.m_pInstance->RestoreRecordedGraphParameters( pEGD->m_updateData );
                            bSetParametersForExternalGraphInstance = true;
                            break;
                        }
                    }

                    EE_ASSERT( bSetParametersForExternalGraphInstance );
                }
            }

            // Update secondary skeletons
            //-------------------------------------------------------------------------

            if ( i == endUpdateIdx )
            {
                SecondarySkeletonList const secondarySkeletons = GetSecondarySkeletonsForUpdate( i );
                m_pPlaybackGraphInstance->SetSecondarySkeletons( secondarySkeletons );
            }

            // Evaluate the graph
            //-------------------------------------------------------------------------

            if ( pUpdateData->m_updateType == RecordedUpdateType::Reset || pUpdateData->m_updateType == RecordedUpdateType::ExternalGraphChanged )
            {
                m_taskListTopologySize = m_taskListDataSize = 0;
            }
            else
            {
                // Evaluate graph
                m_pPlaybackGraphInstance->EvaluateGraph( pUpdateData->m_deltaTime, pUpdateData->m_characterWorldTransform, nullptr, nullptr );

                // Explicitly end root motion debug update for intermediate steps
                #if EE_DEVELOPMENT_TOOLS
                if ( i < ( steps - 1 ) )
                {
                    RecordedGraphUpdateData const *pNextFrameData = m_pRecording->m_recordedUpdateData[i + 1];
                    m_pPlaybackGraphInstance->EndRootMotionDebuggerUpdate( pNextFrameData->m_characterWorldTransform );
                }
                #endif
            }

            // Evaluate tasks
            //-------------------------------------------------------------------------
            // We need to evaluate the pose for a few updates before the desired end update since we need to ensure that the cached pose buffers are filled correctly

            if ( i > ( endUpdateIdx - 2 ) )
            {
                int32_t const nextFrameIdx = ( i < m_pRecording->GetNumRecordedUpdates() - 1 ) ? i + 1 : i;
                RecordedGraphUpdateData const *pCurrentUpdateData = m_pRecording->m_recordedUpdateData[i];
                RecordedGraphUpdateData const *pNextUpdateData = m_pRecording->m_recordedUpdateData[nextFrameIdx];

                // Do we need to evaluate the the pose tasks for this client
                if ( m_pPlaybackGraphInstance->DoesTaskSystemNeedUpdate() )
                {
                    // Use the transform from the next frame as the end transform of the character used to evaluate the pose tasks
                    m_pPlaybackGraphInstance->ExecutePrePhysicsPoseTasks( pNextUpdateData->m_characterWorldTransform );
                    m_pPlaybackGraphInstance->ExecutePostPhysicsPoseTasks();

                    m_taskListTopologySize = pCurrentUpdateData->m_serializedTopologyData.size();
                    m_taskListDataSize = pCurrentUpdateData->m_serializedTaskData.size();
                }
            }
        }

        return hasExternalGraphStateChanged ? StepResult::SuccessExternalGraphsChanged : StepResult::Success;
    }

    GraphRecordingPlayer::StepResult GraphRecordingPlayer::GoToRecordedUpdate( int32_t recordedUpdateIdx )
    {
        EE_ASSERT( IsPlaybackActive() && m_pRecording->HasRecordedData() );
        EE_ASSERT( recordedUpdateIdx >= 0 && recordedUpdateIdx < m_pRecording->GetNumRecordedUpdates() );

        if ( !m_pRecording->IsValidRecordedUpdateIndex( recordedUpdateIdx ) )
        {
            return StepResult::Failure;
        }

        if ( recordedUpdateIdx == m_currentViewedUpdateIdx )
        {
            return StepResult::Success;
        }

        // Step the state
        //-------------------------------------------------------------------------

        StepResult result = StepResult::Failure;

        if ( m_currentViewedUpdateIdx != InvalidIndex && recordedUpdateIdx == ( m_currentViewedUpdateIdx + 1 ) )
        {
            StepRecording( 1 );
        }
        else // Re-evaluate the entire graph to the new index point
        {
            bool const hadExternalGraph = !m_externalGraphInstances.empty();

            m_pPlaybackGraphInstance->DisconnectAllExternalGraphs();
            m_pPlaybackGraphInstance->ResetGraphState();
            DestroyCreatedExternalGraphInstances();

            m_currentViewedUpdateIdx = 0;
            StepRecording( recordedUpdateIdx );

            if ( result == StepResult::Success && hadExternalGraph )
            {
                result = StepResult::SuccessExternalGraphsChanged;
            }
        }

        m_currentViewedUpdateIdx = recordedUpdateIdx;

        // Set character world Transform
        //-------------------------------------------------------------------------

        int32_t const nextFrameIdx = ( recordedUpdateIdx < m_pRecording->GetNumRecordedUpdates() - 1 ) ? recordedUpdateIdx + 1 : recordedUpdateIdx;
        RecordedGraphUpdateData const *pNextUpdateData = m_pRecording->m_recordedUpdateData[nextFrameIdx];
        m_characterTransform = pNextUpdateData->m_characterWorldTransform;

        return result;
    }

    SecondarySkeletonList GraphRecordingPlayer::GetSecondarySkeletonsForUpdate( int32_t recordedUpdateIdx ) const
    {
        SecondarySkeletonList skeletons;

        if ( recordedUpdateIdx != InvalidIndex )
        {
            // Resolve skeleton resource IDs to actual data
            for ( ResourceID const& skeletonResourceID : m_pRecording->m_recordedUpdateData[recordedUpdateIdx]->m_secondarySkeletons )
            {
                for ( auto pSkeleton : m_skeletons )
                {
                    if ( pSkeleton->GetResourceID() == skeletonResourceID )
                    {
                        skeletons.emplace_back( pSkeleton );
                        break;
                    }
                }
            }
        }

        return skeletons;
    }
}