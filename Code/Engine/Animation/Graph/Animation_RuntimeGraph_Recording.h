#pragma once
#include "Engine/_Module/API.h"
#include "Animation_RuntimeGraph_Context.h"
#include "Engine/Animation/AnimationTarget.h"
#include "Engine/Animation/AnimationSyncTrack.h"
#include "Base/Time/Time.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Resource/ResourceID.h"
#include "Animation_RuntimeGraph_TimingInfo.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphNode;
    class GraphDefinition;
    class AnimationClip;

    //-------------------------------------------------------------------------
    // Recorded Full Graph State
    //-------------------------------------------------------------------------

    struct RecordedReferencedGraphData;

    // Records the state of a given graph instance
    class EE_ENGINE_API RecordedGraphState
    {

    public:

        ~RecordedGraphState();

        // Clears all recorded data and allows for a new recording to occur
        void Reset();

        // Prepares recording to be read back
        void PrepareForReading();

        // Get a unique list of the various graphs recorded
        void GetAllRecordedGraphResourceIDs( TVector<ResourceID>& outGraphIDs ) const;

        // Referenced graphs
        //-------------------------------------------------------------------------

        RecordedGraphState* CreateReferencedGraphStateRecording( int16_t referencedGraphNodeIdx );

        RecordedGraphState* GetReferencedGraphRecording( int16_t referencedGraphNodeIdx );
        RecordedGraphState *GetReferencedGraphRecording( int16_t referencedGraphNodeIdx ) const { return const_cast<RecordedGraphState*>( this )->GetReferencedGraphRecording( referencedGraphNodeIdx ); }

        // Serialization helpers
        //-------------------------------------------------------------------------

        template<typename T>
        inline void WriteValue( T& value )
        {
            m_outputArchive << value;
        }
        template<typename T>
        inline void WriteValue( T const& value )
        {
            m_outputArchive << value;
        }

        template<typename T>
        inline void ReadValue( T& value ) const
        {
            m_inputArchive << value;
        }

        // Node accessor
        //-------------------------------------------------------------------------

        template<typename T>
        inline T* GetNode( int16_t nodeIdx ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_pNodes->size() );
            return (T*) ( *m_pNodes )[nodeIdx];
        }

    public:

        ResourceID                                          m_graphResourceID;
        StringID                                            m_variationID;
        TVector<int16_t>                                    m_initializedNodeIndices;
        TVector<RecordedReferencedGraphData*>               m_referencedGraphData;
        bool                                                m_isStandaloneGraph = false;
        TVector<GraphNode*>*                                m_pNodes = nullptr; // Used during restoring state

    private:

        Serialization::BinaryOutputArchive                  m_outputArchive;
        mutable Serialization::BinaryInputArchive           m_inputArchive;
    };

    struct RecordedReferencedGraphData
    {
        int16_t                                             m_referencedGraphNodeIdx = InvalidIndex;
        RecordedGraphState                                  m_graphState;
    };

    //-------------------------------------------------------------------------
    // Recorded Per-Frame Graph Data
    //-------------------------------------------------------------------------

    enum class RecordedUpdateType
    {
        Unknown,
        FirstRecording,
        Reset,
        TimeStep,
        ExternalGraphChanged
    };

    struct RecordedExternalGraphData;
    struct RecordedExternalPoseData;

    // All the data needed to replay a given recorded frame
    struct RecordedGraphUpdateData
    {
        union ParameterData
        {
            ParameterData() {}

            bool                                            m_bool;
            StringID                                        m_ID;
            float                                           m_float;
            Float3                                          m_vector;
            Target                                          m_target;
        };

    public:

        RecordedGraphUpdateData() = default;
        ~RecordedGraphUpdateData();

        RecordedExternalGraphData* CreateExternalGraphData();
        RecordedExternalGraphData* TryGetExternalGraphData( int16_t nodeIdx );
        inline RecordedExternalGraphData const *TryGetExternalGraphData( int16_t nodeIdx ) const { return const_cast<RecordedGraphUpdateData*>( this )->TryGetExternalGraphData( nodeIdx ); }

        RecordedExternalPoseData *CreateExternalPoseData();

        RecordedGraphUpdateData( RecordedGraphUpdateData const & ) = delete;
        RecordedGraphUpdateData( RecordedGraphUpdateData && ) = delete;
        RecordedGraphUpdateData &operator=( RecordedGraphUpdateData const & ) = delete;
        RecordedGraphUpdateData &operator=( RecordedGraphUpdateData && ) = delete;

        void SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons );

    public:

        Transform                                           m_characterWorldTransform;
        SyncTrackTimeRange                                  m_updateRange;
        TVector<ParameterData>                              m_parameterData;
        Seconds                                             m_deltaTime = 0.0f;
        Blob                                                m_serializedTopologyData;
        Blob                                                m_serializedTaskData;
        GraphTimeInfo                                       m_timingInfo;
        int32_t                                             m_recordedGraphStateIdx = InvalidIndex;
        RecordedUpdateType                                  m_updateType = RecordedUpdateType::Unknown;
        TVector<ResourceID>                                 m_secondarySkeletons;
        TVector<RecordedExternalGraphData*>                 m_externalGraphData;
        TVector<RecordedExternalPoseData*>                  m_externalPoseData;
    };

    struct RecordedExternalGraphData
    {
        int16_t                                             m_externalGraphNodeIdx = InvalidIndex;
        ResourceID                                          m_graphResourceID;
        RecordedGraphUpdateData                             m_updateData;
    };

    struct RecordedExternalPoseData
    {
        int16_t                                             m_externalPoseNodeIdx = InvalidIndex;
        StringID                                            m_slotID;

        ResourceID                                          m_clipResourceID0;
        Percentage                                          m_startTime0 = 0.0f;
        Percentage                                          m_endTime0 = 0.0f;

        ResourceID                                          m_clipResourceID1;
        Percentage                                          m_startTime1 = 0.0f;
        Percentage                                          m_endTime1 = 0.0f;

        float                                               m_blendWeight = 0.0f;
    };

    //-------------------------------------------------------------------------
    // Graph Recording
    //-------------------------------------------------------------------------

    // Records information about each update for the recorded graph instance
    class EE_ENGINE_API GraphRecording
    {
        friend GraphInstance;

    public:

        GraphRecording() = default;
        ~GraphRecording() { Reset(); }

        void Reset();

        inline bool HasRecordedData() const { return !m_recordedUpdateData.empty() && m_graphID.IsValid(); }

        inline ResourceID const &GetRecordedGraphID() const { return m_graphID; }
        inline StringID GetRecordedGraphVariationID() const { return m_variationID; }

        bool HasRecordedDataForGraph( ResourceID const& graphResourceID ) const;
        bool IsRecordedDataForAStandaloneGraph() const;
        inline int32_t GetNumRecordedUpdates() const { return int32_t( m_recordedUpdateData.size() ); }
        inline bool IsValidRecordedUpdateIndex( int32_t updateIdx ) const { return updateIdx >= 0 && updateIdx < m_recordedUpdateData.size(); }

        TInlineVector<ResourceID, 2> GetAllRecordedGraphResourceIDs() const;
        TInlineVector<ResourceID, 2> GetAllRecordedExternalClipResourceIDs() const;
        TInlineVector<ResourceID, 2> GetAllRecordedSkeletonResourceIDs() const;

    private:

        GraphRecording( GraphRecording const & ) = delete;
        GraphRecording( GraphRecording && ) = delete;
        GraphRecording &operator=( GraphRecording const & ) = delete;
        GraphRecording &operator=( GraphRecording && ) = delete;

        RecordedGraphUpdateData *CreateUpdateData()
        {
            auto pUpdateData = EE::New<RecordedGraphUpdateData>();
            m_recordedUpdateData.emplace_back( pUpdateData );
            return pUpdateData;
        }

    public:

        ResourceID                                          m_graphID;
        StringID                                            m_variationID;
        TVector<RecordedGraphUpdateData*>                   m_recordedUpdateData;
        TVector<RecordedGraphState*>                        m_recordedGraphStates;
    };

    //-------------------------------------------------------------------------
    // Graph Recording Playback
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphRecordingPlayer
    {
    public:

        enum class StepResult
        {
            Success,
            SuccessExternalGraphsChanged,
            Failure,
        };

    private:

        struct RecordedExternalGraph
        {
            int16_t                                         m_nodeIdx = InvalidIndex;
            GraphInstance                                   *m_pInstance = nullptr;
        };

    public:

        ~GraphRecordingPlayer();

        void StartPlayback( GraphInstance *pPlaybackGraphInstance, TInlineVector<GraphDefinition const*, 2> const& externalGraphDefs, TInlineVector<AnimationClip const*, 2> const& externalClips, GraphRecording *pRecording );
        void StopPlayback();
        bool IsPlaybackActive() const { return m_pPlaybackGraphInstance != nullptr && m_pRecording != nullptr; }

        // Run the recording to the specified update index
        StepResult GoToRecordedUpdate( int32_t recordedUpdateIdx );

        // Get the currently viewed update index
        int32_t GetCurrentUpdateIndex() const { return m_currentViewedUpdateIdx; }

        // Get the currently viewed update index
        int32_t GetNumRecordedUpdates() const { return m_pRecording->GetNumRecordedUpdates(); }

        // Get the update type for the currently viewed index
        RecordedUpdateType GetUpdateType() const;

        // Get the timing info for the currently viewed index
        GraphTimeInfo const& GetTimingInfo() const;

        // Get the delta time for the currently viewed index
        Seconds GetDeltaTime() const;

        // Get the task list topology size for the currently viewed index
        size_t GetTaskListTopologySize() const { return m_taskListTopologySize; }

        // Get the task list data size for the currently viewed index
        size_t GetTaskListDataSize() const { return m_taskListDataSize; }

        // Get the character transform at the currently viewed index
        Transform const &GetRecordedCharacterTransform() const { return m_characterTransform; }

        // Get the recorded set of secondary skeletons
        SecondarySkeletonList GetSecondarySkeletonsForUpdate( int32_t recordedUpdateIdx ) const;

    private:

        void DestroyCreatedExternalGraphInstances();

        AnimationClip const* TryGetExternalClip( ResourceID const& ID ) const;

        // Steps the recording from the current point forward N steps
        StepResult StepRecording( int32_t steps );

    private:

        GraphInstance                                       *m_pPlaybackGraphInstance = nullptr;
        GraphRecording                                      *m_pRecording = nullptr;
        int32_t                                             m_currentViewedUpdateIdx = InvalidIndex;
        size_t                                              m_taskListTopologySize = 0;
        size_t                                              m_taskListDataSize = 0;
        Transform                                           m_characterTransform;
        TInlineVector<GraphDefinition const*, 2>            m_externalGraphDefinitions;
        TInlineVector<AnimationClip const*, 2>              m_externalClips;
        TInlineVector<Skeleton const*,2>                    m_skeletons;
        TVector<RecordedExternalGraph>                      m_externalGraphInstances;
    };
}