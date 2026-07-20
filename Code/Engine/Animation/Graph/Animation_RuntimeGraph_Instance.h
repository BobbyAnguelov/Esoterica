#pragma once
#include "Animation_RuntimeGraph_Definition.h"
#include "Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Animation_RuntimeGraph_Context.h"
#include "Nodes/Animation_RuntimeGraphNode_Parameters.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Base/Types/PointerID.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsWorld;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class TaskSystem;
    class GraphNode;
    class PoseNode;
    class ExternalPoseNode;
    class ReferencedGraphNode;
    class GraphRecording;
    class RecordedGraphState;
    struct ExternalPoseData;
    struct RecordedGraphUpdateData;
    enum class TaskSystemDebugMode;

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    enum class GraphDebugMode
    {
        Off,
        On,
    };
    #endif

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphInstance
    {
        friend class AnimationDebugView;
        friend class GraphRecordingPlayer;

    public:

        #if EE_DEVELOPMENT_TOOLS
        struct DebuggableGraphInstance
        {
            PointerID GetID() const { return PointerID( m_pInstance ); }
            inline bool IsExternalGraph() const { return m_externalSlotID.IsValid(); }

        public:

            SourcePath          m_path;
            StringID            m_externalSlotID;
            GraphInstance*      m_pInstance = nullptr;
        };
        #endif

    public:

        // Main instance
        inline GraphInstance( GraphDefinition const* pGraphDefinition ) : GraphInstance( pGraphDefinition, nullptr ) {}

        // Referenced graph instance
        explicit GraphInstance( GraphDefinition const* pGraphDefinition, GraphContext *pParentContext );

        ~GraphInstance();

        // Info
        //-------------------------------------------------------------------------

        inline GraphDefinition const* GetGraphDefinition() const { return m_pGraphDefinition; }
        inline ResourceID const& GetDefinitionResourceID() const { return m_pGraphDefinition->GetResourceID(); }
        inline StringID const& GetVariationID() const { return m_pGraphDefinition->m_variationID; }

        // Is this a full self-contained instance i.e. not a referenced or a external graph instance
        inline bool IsStandaloneInstance() const { return m_isStandaloneGraphInstance; }

        // Pose
        //-------------------------------------------------------------------------

        // Get the final primary pose from the task system
        inline Skeleton const *GetPrimarySkeleton() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetSkeleton(); }

        // Get the final primary pose from the task system
        inline Pose const* GetPrimaryPose() { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetPrimaryPose(); }

        // Get the final primary pose from the task system
        inline Pose const* GetPrimaryPoseForPreviousUpdate() { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetPrimaryPoseForPreviousUpdate(); }

        // Set the list of secondary skeletons we should try to animate
        void SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons );

        // Do we have any secondary poses
        inline bool HasSecondaryPoses() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetNumSecondarySkeletons() > 0; }

        // Get the number of secondary poses
        inline int32_t GetNumSecondaryPoses() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetNumSecondarySkeletons(); }

        // Do we have secondary pose for a given skeleton
        inline bool HasSecondaryPose( Skeleton const* pSkeleton ) const { return GetSecondaryPose( pSkeleton ) != nullptr; }

        // Do we have secondary pose for a given skeleton
        inline bool HasSecondaryPoseForPreviousUpdate( Skeleton const* pSkeleton ) const { return GetSecondaryPoseForPreviousUpdate( pSkeleton ) != nullptr; }

        // Get a secondary pose for a given skeleton
        Pose const *GetSecondaryPose( Skeleton const* pSkeleton ) const;

        // Get a secondary pose for a given skeleton
        Pose const *GetSecondaryPoseForPreviousUpdate( Skeleton const* pSkeleton ) const;

        // Get all sampled secondary poses
        inline TInlineVector<Pose const*, 1> GetSecondaryPoses() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetSecondaryPoses(); }

        // Get all sampled secondary poses for the previous update
        inline TInlineVector<Pose const*, 1> GetSecondaryPosesForPreviousUpdate() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetSecondaryPosesForPreviousUpdate(); }

        // Set the skeleton LOD for the result pose
        inline void SetSkeletonLOD( Skeleton::LOD lod ) { EE_ASSERT( m_isStandaloneGraphInstance ); m_graphContext.m_pTaskSystem->SetSkeletonLOD( lod ); }

        // Get the current skeleton LOD we are using
        inline Skeleton::LOD GetSkeletonLOD() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->GetSkeletonLOD(); }

        // Set whether we are allowed to sample float channel data
        EE_FORCE_INLINE void SetAllowFloatChannelSampling( bool allowFloatChannelSampling ) { EE_ASSERT( m_isStandaloneGraphInstance ); m_graphContext.m_pTaskSystem->SetAllowFloatChannelSampling( allowFloatChannelSampling ); }

        // Are we allowed to sample float channel data?
        EE_FORCE_INLINE bool IsFloatChannelSamplingAllowed() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pTaskSystem->IsFloatChannelSamplingAllowed(); }

        // Task System
        //-------------------------------------------------------------------------

        // Does the task system have any pending pose tasks
        bool DoesTaskSystemNeedUpdate() const;

        // Serialize the currently registered pose tasks. Note: This can only be done after the task system has executed!
        void SerializeTaskList( Blob& outTopologyData, Blob& outTaskData );

        // Get generated resource mappings
        TaskSerializationContext const &GetTaskSerializationContext() { return UpdateTaskSerializationContext(); }

        // Graph State
        //-------------------------------------------------------------------------

        // Is this a valid instance (i.e. has a valid root node)
        bool IsValid() const { return m_pRootNode != nullptr && m_pRootNode->IsValid(); }

        // Is this a valid instance that has been correctly initialized
        bool IsInitialized() const { return m_pRootNode != nullptr && m_pRootNode->IsValid() && m_pRootNode->IsInitialized(); }

        // Reset/Initialize the graph state with an initial time
        void ResetGraphState( SyncTrackTime initTime = SyncTrackTime() );

        // Reset/Initialize the graph state with an initial time
        void ResetGraphState( GraphTimeInfo const &graphTimeInfo );

        // Reset/Initialize the graph state with an initial time
        void ResetReferencedGraphState( GraphContext const &parentContext, SyncTrackTime initTime = SyncTrackTime() );

        // Reset/Initialize the graph state with an initial time
        void ResetReferencedGraphState( GraphContext const &parentContext, GraphTimeInfo const &graphTimeInfo );

        // Get the current graph updateID
        inline uint32_t GetUpdateID() const { return m_graphContext.m_updateID; }

        // Get the root motion delta for the last update
        inline Transform GetRootMotionDeltaForLastUpdate() const { return m_rootMotionDelta; }

        // Run the graph logic
        // If the sync track update range is set, this will perform a synchronized update
        // If the sync track update range is not set, it will run unsynchronized and use the frame delta time instead
        GraphPoseNodeResult EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, SyncTrackTimeRange const* pUpdateRange );

        // Run the graph logic - as a referenced graph
        // If the sync track update range is set, this will perform a synchronized update
        // If the sync track update range is not set, it will run unsynchronized and use the frame delta time instead
        GraphPoseNodeResult EvaluateReferencedGraph( GraphContext const& parentContext, SyncTrackTimeRange const* pUpdateRange );

        // Execute any pre-physics pose tasks (assumes the character is at its final position for this frame)
        void ExecutePrePhysicsPoseTasks( Transform const& endWorldTransform );

        // Execute any post-physics pose tasks
        void ExecutePostPhysicsPoseTasks();

        #if EE_DEVELOPMENT_TOOLS
        // Get the time it took for the last graph update to execute
        Microseconds GetGraphExecutionTime() const { return m_executionTime; }

        // Get the time it took for the last task system update to execute
        Microseconds GetTasksExecutionTime() const { return m_graphContext.m_pTaskSystem->GetExecutionTime(); }
        #endif

        // Get the sampled events for the last update
        SampledEventsBuffer const& GetSampledEvents() const { EE_ASSERT( m_isStandaloneGraphInstance ); return *m_graphContext.m_pSampledEventsBuffer; }

        // Get the list of active node indices for this instance
        inline TVector<int16_t> const& GetActiveNodes() const { return m_graphContext.m_activeNodes; }

        // Get the current sync time for the base and any active layers
        void GetCurrentGraphTimingInfo( GraphTimeInfo &outGraphTimeInfo ) const;

        // General Node Info
        //-------------------------------------------------------------------------

        inline PoseNode const* GetRootNode() const { return m_pRootNode; }

        // Was this node active in the last update
        inline bool IsNodeActive( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            return IsControlParameter( nodeIdx ) || m_nodes[nodeIdx]->IsNodeActive( m_graphContext.m_updateID );
        }

        inline bool IsValidNodeIndex( int16_t nodeIdx ) const { return nodeIdx >= 0 && nodeIdx < m_pGraphDefinition->m_nodeDefinitions.size(); }

        // Control Parameters
        //-------------------------------------------------------------------------

        inline int32_t GetNumControlParameters() const { return (int32_t) m_pGraphDefinition->m_controlParameterIDs.size(); }

        inline int16_t GetControlParameterIndex( StringID parameterID ) const
        {
            auto const& parameterLookupMap = m_pGraphDefinition->m_parameterLookupMap;
            auto const foundIter = parameterLookupMap.find( parameterID );
            if ( foundIter != parameterLookupMap.end() )
            {
                return foundIter->second;
            }

            return InvalidIndex;
        }

        inline StringID GetControlParameterID( int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return m_pGraphDefinition->m_controlParameterIDs[parameterNodeIdx];
        }

        inline GraphValueType GetControlParameterType( int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return static_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->GetValueType();
        }

        template<typename T>
        inline T GetControlParameterValue( int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return static_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->GetValue<T>( const_cast<GraphContext&>( m_graphContext ) );
        }

        void SetControlParameterValue( int16_t parameterNodeIdx, bool const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            static_cast<ControlParameterBoolNode*>( m_nodes[parameterNodeIdx] )->DirectlySetValue( value );
        }

        void SetControlParameterValue( int16_t parameterNodeIdx, StringID const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            static_cast<ControlParameterIDNode*>( m_nodes[parameterNodeIdx] )->DirectlySetValue( value );
        }

        void SetControlParameterValue( int16_t parameterNodeIdx, float const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            EE_ASSERT( Math::IsFinite( value ) );
            EE_ASSERT( Math::IsInRangeInclusive( value, float( -INT32_MAX ), float( INT32_MAX ) ) );
            static_cast<ControlParameterFloatNode*>( m_nodes[parameterNodeIdx] )->DirectlySetValue( value );
        }

        void SetControlParameterValue( int16_t parameterNodeIdx, Float3 const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            EE_ASSERT( value.IsFinite() );
            static_cast<ControlParameterVectorNode*>( m_nodes[parameterNodeIdx] )->DirectlySetValue( value );
        }

        void SetControlParameterValue( int16_t parameterNodeIdx, Vector const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            EE_ASSERT( value.IsFinite() );
            static_cast<ControlParameterVectorNode*>( m_nodes[parameterNodeIdx] )->DirectlySetValue( value.ToFloat3() );
        }

        void SetControlParameterValue( int16_t parameterNodeIdx, Target const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            static_cast<ControlParameterTargetNode*>( m_nodes[parameterNodeIdx] )->DirectlySetValue( value );
        }

        // External Graphs
        //-------------------------------------------------------------------------

        // Get the number of external graph slots available
        int32_t GetNumExternalGraphSlots() const;

        // Get the slot ID for a given external graph slot index - Note: this is only valid to call for a valid slot index
        StringID GetExternalGraphSlotID( int32_t slotIdx ) const;

        // Check if a given slot ID is valid
        inline bool IsValidExternalGraphSlotID( StringID slotID ) const { return GetExternalGraphNodeIndex( slotID ) != InvalidIndex; }

        // Is the specified external graph slot node active
        inline bool IsExternalGraphSlotNodeActive( StringID slotID ) const { return IsNodeActive( GetExternalGraphNodeIndex( slotID ) ); }

        // Is the specified external graph slot node filled
        bool IsExternalGraphSlotFilled( StringID slotID ) const;

        // Connects a supplied external graph to the specified slot. Note, it is the callers responsibility to ensure that the slot ID is valid!
        bool ConnectExternalGraph( StringID slotID, GraphInstance* pGraphInstance );

        // Disconnects an external graph
        void DisconnectExternalGraph( StringID slotID );

        // Disconnects all external graphs currently connected
        void DisconnectAllExternalGraphs();

        // External Poses
        //-------------------------------------------------------------------------

        int32_t GetNumExternalPoseSlots() const;

        // Get the slot ID for a given external pose slot index - Note: this is only valid to call for a valid slot index
        StringID GetExternalPoseSlotID( int32_t nSlotIdx ) const;

        // Check if a given slot ID is valid
        inline bool IsValidExternalPoseSlotID( StringID slotID ) const { return GetExternalPoseNodeIndex( slotID ) != InvalidIndex; }

        // Is the specified external pose slot node active
        inline bool IsExternalPoseSlotNodeActive( StringID slotID ) const { return IsNodeActive( GetExternalPoseNodeIndex( slotID ) ); }

        // Is the specified external pose slot node filled
        bool IsExternalPoseSet( StringID slotID ) const;

        // Gets the current state of an external pose node, returns false if the slot is not filled. Note, it is the callers responsibility to ensure that the slot ID is valid!
        bool GetExternalPoseState( StringID slotID, ExternalPoseData& outState );

        // Connects a supplied external pose to the specified slot. Note, it is the callers responsibility to ensure that the slot ID is valid!
        bool SetExternalPose( StringID slotID, ExternalPoseData const& poseData );

        // Disconnects an external pose, will destroy the created instance
        void ClearExternalPose( StringID slotID );

        // Disconnects all external poses
        void ClearAllExternalPoses();

        // Recording
        //-------------------------------------------------------------------------

        // Are we currently recording this graph
        inline bool IsRecording() const { return m_pRecording != nullptr; }

        // Start a graph recording
        void StartRecording( GraphRecording* pRecording );

        // Stop a graph recording
        void StopRecording();

        // Set to a previously recorded state
        bool SetToRecordedState( RecordedGraphUpdateData const& recordedUpdateData, TVector<RecordedGraphState*> const &recordedGraphStates );

        // Record the current state of this referenced graph
        void RecordReferencedGraphState( RecordedGraphState& recordedState );

        // Set to a previously recorded referenced graph state
        bool SetToRecordedReferencedGraphState( RecordedGraphState const& recordedState );

        // Debug Information
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Get the graph debug mode
        inline GraphDebugMode GetGraphDebugMode() const { return m_debugMode; }

        // Set the graph debug mode
        inline void SetGraphDebugMode( GraphDebugMode mode ) { m_debugMode = mode; }

        // Get the root motion debug mode
        inline RootMotionDebugMode GetRootMotionDebugMode() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pRootMotionDebugger->GetDebugMode(); }

        // Set the root motion debug mode
        inline void SetRootMotionDebugMode( RootMotionDebugMode mode ) { EE_ASSERT( m_isStandaloneGraphInstance ); m_graphContext.m_pRootMotionDebugger->SetDebugMode( mode ); }

        // Get the root motion debugger for this instance
        inline RootMotionDebugger const* GetRootMotionDebugger() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pRootMotionDebugger; }

        // Manually end the root motion debugger update (call this if you explicitly skip executing the pose tasks)
        inline void EndRootMotionDebuggerUpdate( Transform const& endWorldTransform ) { EE_ASSERT( m_isStandaloneGraphInstance ); m_graphContext.m_pRootMotionDebugger->EndCharacterUpdate( endWorldTransform ); }

        // Get the task system debug mode
        inline TaskSystemDebugMode GetTaskSystemDebugMode() const;

        // Set the task system debug mode
        inline void SetTaskSystemDebugMode( TaskSystemDebugMode mode );

        // Get the debug world transform that the task system used to execute
        Transform GetTaskSystemDebugWorldTransform();

        // Get the runtime time info for a specified pose node
        inline PoseNodeDebugInfo GetPoseNodeDebugInfo( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            EE_ASSERT( m_nodes[nodeIdx]->GetValueType() == GraphValueType::Pose );
            auto pNode = static_cast<PoseNode const*>( m_nodes[nodeIdx] );
            return pNode->GetDebugInfo();
        }

        // Get all debuggable graphs - this will return all referenced and external graph instances (recursively)
        void GetDebuggableGraphInstances( TVector<DebuggableGraphInstance>& outReferencedGraphInstances ) const;

        // Get the connected debuggable graph instance
        GraphInstance const* GetDebuggableGraphInstance( int16_t nodeIdx ) const;

        // Get the connected debuggable graph instance
        GraphInstance const* GetDebuggableGraphInstance( PointerID referencedGraphInstanceID ) const;

        // Get the connected debuggable graph instance for an external graph node
        GraphInstance const* GetDebuggableGraphInstance( StringID externalSlotID ) const;

        // Get the connected external graph instance
        SourcePath GetSourcePathForDebuggableGraphInstance( PointerID instanceID ) const;

        // Resolve a source path into human readable string
        InlineString ResolveSourcePath( SourcePath const& sourcePath ) const;

        // Get the value of a specified value node
        template<typename T>
        inline T GetRuntimeNodeDebugValue( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            auto pValueNode = static_cast<ValueNode*>( const_cast<GraphNode*>( m_nodes[nodeIdx] ) );
            return pValueNode->GetValue<T>( const_cast<GraphContext&>( m_graphContext ) );
        }

        // Get a node instance for debugging
        inline GraphNode const* GetNodeDebugInstance( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            return m_nodes[nodeIdx];
        }

        // Get the runtime log for this graph instance
        TVector<GraphLogEntry> const* GetLog() const { EE_ASSERT( m_isStandaloneGraphInstance ); return m_graphContext.m_pLog; }

        // Log any errors/warnings occurring from the graph update!
        void OutputLog();

        // Log any errors/warnings occurring from the task system update
        void OutputTaskSystemLog();

        // Draw full graph debug visualizations
        void DrawDebug( DebugDrawContext& drawContext, TVector<SourcePath> const *pNodesToDebug = nullptr );
        #endif

    private:

        GraphInstance( GraphInstance const& ) = delete;
        GraphInstance( GraphInstance&& ) = delete;
        GraphInstance& operator=( GraphInstance const& ) = delete;
        GraphInstance& operator=( GraphInstance&& ) = delete;

        // Full initialization of persistent nodes
        void Initialize();

        // Full shutdown of persistent and root nodes
        void Shutdown();

        // Reset (shutdown and init) of the root node
        void ResetGraphState( GraphTimeInfo const *pGraphTimeInfo, SyncTrackTime initTime );

        GraphPoseNodeResult EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, BranchState const* pBranchState, SyncTrackTimeRange const* pUpdateRange );

        EE_FORCE_INLINE bool IsControlParameter( int16_t nodeIdx ) const { return nodeIdx < GetNumControlParameters(); }

        // External Graphs
        //-------------------------------------------------------------------------

        int32_t GetExternalGraphSlotIndex( StringID slotID ) const;
        int16_t GetExternalGraphNodeIndex( StringID slotID ) const;
        ReferencedGraphNode* GetExternalGraphNode( StringID slotID ) const;

        void InitializeReferencedGraphContexts( GraphContext *pParentContext, bool initializeGraphInstances );
        void ShutdownReferencedGraphContexts( bool shutdownGraphInstances );

        // External Poses
        //-------------------------------------------------------------------------

        int32_t GetExternalPoseSlotIndex( StringID slotID ) const;
        int16_t GetExternalPoseNodeIndex( StringID slotID ) const;
        ExternalPoseNode *GetExternalPoseNode( StringID slotID ) const;

        // Task Serialization
        //-------------------------------------------------------------------------

        // Returns the list of all resource LUTs used by this instance: the graph def + all connected external graphs
        void GetResourceLookupTables( TInlineVector<ResourceLUT const*, 10>& outLUTs ) const;

        // Generate a resource mapping for all resources used by this graph instance
        TaskSerializationContext& UpdateTaskSerializationContext();

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void GetReferencedGraphsForDebug( TVector<DebuggableGraphInstance>& outReferencedGraphInstances, SourcePath const& parentPath ) const;
        #endif

        // Recording
        //-------------------------------------------------------------------------

        // Get the current active layer states
        void GetUpdateStateForActiveLayers( TVector<GraphLayerUpdateState>& outRanges ) const;

        void RecordPreGraphEvaluateState( Seconds const deltaTime, Transform const& startWorldTransform );

        void RecordPostGraphEvaluateState( SyncTrackTimeRange const* pRange );

        void RecordGraphState( RecordedUpdateType updateType, GraphTimeInfo const& timingInfo, Transform const& worldTransform );

        void RecordGraphNodeState( RecordedGraphState &recordedState );

        bool RestoreRecordedGraphState( RecordedGraphState const &recordedState );

        void RestoreRecordedGraphParameters( RecordedGraphUpdateData const &updateData );

        // Record all the registered tasks for this update
        void RecordTasks();

    private:

        GraphDefinition const* const            m_pGraphDefinition = nullptr;
        TVector<GraphNode*>                     m_nodes;
        uint8_t*                                m_pAllocatedInstanceMemory = nullptr;
        PoseNode*                               m_pRootNode = nullptr;
        bool                                    m_isExternalGraph = false;
        bool                                    m_isStandaloneGraphInstance = true;

        TaskSerializationContext                m_taskSerializationContext;
        GraphContext                            m_graphContext;
        SecondarySkeletonList                   m_secondarySkeletons;
        bool                                    m_taskSerializationRequiresUpdate = false;
        bool                                    m_externalGraphStateChanged = false;

        Transform                               m_rootMotionDelta = Transform::Identity;
        GraphRecording*                         m_pRecording = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        GraphDebugMode                          m_debugMode = GraphDebugMode::Off;
        Microseconds                            m_executionTime;
        #endif
    };
}
