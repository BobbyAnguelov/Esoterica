#pragma once
#include "Animation_RuntimeGraph_Definition.h"
#include "Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Animation_RuntimeGraph_Contexts.h"
#include "Base/Types/PointerID.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsWorld;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphContext;
    class TaskSystem;
    class GraphNode;
    class PoseNode;
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

    public:

        struct ChildGraph
        {
            int16_t             m_nodeIdx = InvalidIndex;
            GraphInstance*      m_pInstance = nullptr;
        };

        struct ExternalGraph
        {
            StringID            m_slotID;
            int16_t             m_nodeIdx = InvalidIndex;
            GraphInstance*      m_pInstance = nullptr;
        };

        #if EE_DEVELOPMENT_TOOLS
        struct DebuggableChildGraph
        {
            PointerID GetID() const { return PointerID( m_pInstance ); }

            String              m_pathToInstance;
            GraphInstance*      m_pInstance = nullptr;
        };
        #endif

    public:

        // Main instance
        inline GraphInstance( GraphVariation const* pGraphVariation, uint64_t ownerID ) : GraphInstance( pGraphVariation, ownerID, nullptr ) {}
        ~GraphInstance();

        // Info
        //-------------------------------------------------------------------------

        inline GraphVariation const* GetGraphVariation() const { return m_pGraphVariation; }
        inline StringID const& GetVariationID() const { return m_pGraphVariation->m_dataSet.m_variationID; }
        inline ResourceID const& GetResourceID() const { return m_pGraphVariation->GetResourceID(); }
        inline ResourceID const& GetDefinitionResourceID() const { return m_pGraphVariation->m_pGraphDefinition->GetResourceID(); }

        // Returns the list of all resource LUTs used by this instance: the graph def + all connected external graphs
        void GetResourceLookupTables( TInlineVector<ResourceLUT const*, 10>& outLUTs ) const;

        // Pose
        //-------------------------------------------------------------------------

        // Get the final pose from the task system
        Pose const* GetPose();

        // Set the skeleton LOD for the result pose
        void SetSkeletonLOD( Skeleton::LOD lod );

        // Get the current skeleton LOD we are using
        Skeleton::LOD GetSkeletonLOD() const;

        // Task System
        //-------------------------------------------------------------------------

        // Enable task serialization
        void EnableTaskSystemSerialization( TypeSystem::TypeRegistry const& typeRegistry );

        // Disable task serialization
        void DisableTaskSystemSerialization();

        // Does the task system have any pending pose tasks
        bool DoesTaskSystemNeedUpdate() const;

        // Serialize the currently registered pose tasks. Note: This can only be done after the task system has executed!
        void SerializeTaskList( Blob& outBlob ) const;

        // Graph State
        //-------------------------------------------------------------------------

        // Is this a valid instance (i.e. has a valid root node)
        bool IsValid() const { return m_pRootNode != nullptr && m_pRootNode->IsValid(); }

        // Is this a valid instance that has been correctly initialized
        bool IsInitialized() const { return m_pRootNode != nullptr && m_pRootNode->IsValid() && m_pRootNode->IsInitialized(); }

        // Reset/Initialize the graph state with an initial time
        void ResetGraphState( SyncTrackTime initTime = SyncTrackTime(), TVector<GraphLayerUpdateState> const* pLayerInitInfo = nullptr );

        // Get the current graph updateID
        inline uint32_t GetUpdateID() const { return m_graphContext.m_updateID; }

        // Get the current active layer states
        void GetUpdateStateForActiveLayers( TVector<GraphLayerUpdateState>& outRanges );

        // Run the graph logic
        // If the sync track update range is set, this will perform a synchronized update
        // If the sync track update range is not set, it will run unsynchronized and use the frame delta time instead
        GraphPoseNodeResult EvaluateGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, SyncTrackTimeRange const* pUpdateRange, bool resetGraphState = false );

        // Run the graph logic - as a child graph
        // If the sync track update range is set, this will perform a synchronized update
        // If the sync track update range is not set, it will run unsynchronized and use the frame delta time instead
        GraphPoseNodeResult EvaluateChildGraph( Seconds const deltaTime, Transform const& startWorldTransform, Physics::PhysicsWorld* pPhysicsWorld, SyncTrackTimeRange const* pUpdateRange, GraphLayerContext* pLayerContext );

        // Execute any pre-physics pose tasks (assumes the character is at its final position for this frame)
        void ExecutePrePhysicsPoseTasks( Transform const& endWorldTransform );

        // Execute any post-physics pose tasks
        void ExecutePostPhysicsPoseTasks();

        // Get the sampled events for the last update
        SampledEventsBuffer const& GetSampledEvents() const { return m_graphContext.m_sampledEventsBuffer; }

        // General Node Info
        //-------------------------------------------------------------------------

        inline PoseNode const* GetRootNode() const { return m_pRootNode; }

        // Was this node active in the last update
        inline bool IsNodeActive( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            return m_nodes[nodeIdx]->IsNodeActive( const_cast<GraphContext&>( m_graphContext ) );
        }

        inline bool IsValidNodeIndex( int16_t nodeIdx ) const 
        {
            return nodeIdx < m_pGraphVariation->m_pGraphDefinition->m_nodeSettings.size();
        }

        // Control Parameters
        //-------------------------------------------------------------------------

        inline int32_t GetNumControlParameters() const { return (int32_t) m_pGraphVariation->m_pGraphDefinition->m_controlParameterIDs.size(); }

        inline int16_t GetControlParameterIndex( StringID parameterID ) const
        {
            int32_t const numParams = GetNumControlParameters();
            for ( int16_t i = 0; i < numParams; i++ )
            {
                if ( m_pGraphVariation->m_pGraphDefinition->m_controlParameterIDs[i] == parameterID )
                {
                    return i;
                }
            }

            return InvalidIndex;
        }

        inline StringID GetControlParameterID( int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return m_pGraphVariation->m_pGraphDefinition->m_controlParameterIDs[parameterNodeIdx];
        }

        inline GraphValueType GetControlParameterType( int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return reinterpret_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->GetValueType();
        }

        template<typename T>
        inline void SetControlParameterValue( int16_t parameterNodeIdx, T const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            reinterpret_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->SetValue<T>( value );
        }

        template<typename T>
        inline T GetControlParameterValue( int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return reinterpret_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->GetValue<T>( const_cast<GraphContext&>( m_graphContext ) );
        }

        // External Graphs
        //-------------------------------------------------------------------------

        // Check if a given slot ID is valid
        inline bool IsValidExternalGraphSlotID( StringID slotID ) const { return GetExternalGraphNodeIndex( slotID ) != InvalidIndex; }

        // Is the specified external graph slot node active
        inline bool IsExternalGraphSlotNodeActive( StringID slotID ) const { return IsNodeActive( GetExternalGraphNodeIndex( slotID ) ); }

        // Is the specified external graph slot node filled
        inline bool IsExternalGraphSlotFilled( StringID slotID ) const { return GetConnectedExternalGraphIndex( slotID ) != InvalidIndex; }

        // Connects a supplied external graph to the specified slot. Note, it is the callers responsibility to ensure that the slot ID is valid!
        GraphInstance* ConnectExternalGraph( StringID slotID, GraphVariation const* pGraphVariation );

        // Disconnects an external graph, will destroy the created instance
        void DisconnectExternalGraph( StringID slotID );

        // Debug Information
        //-------------------------------------------------------------------------
        
        #if EE_DEVELOPMENT_TOOLS
        // Get the list of active node indices for this instance
        inline TVector<int16_t> const& GetActiveNodes() const { return m_activeNodes; }

        // Get the graph debug mode
        inline GraphDebugMode GetGraphDebugMode() const { return m_debugMode; }

        // Set the graph debug mode
        inline void SetGraphDebugMode( GraphDebugMode mode ) { m_debugMode = mode; }

        // Get the root motion debug mode
        inline RootMotionDebugMode GetRootMotionDebugMode() const { return m_rootMotionDebugger.GetDebugMode(); }

        // Set the root motion debug mode
        inline void SetRootMotionDebugMode( RootMotionDebugMode mode ) { m_rootMotionDebugger.SetDebugMode( mode ); }

        // Get the root motion debugger for this instance
        inline RootMotionDebugger const* GetRootMotionDebugger() const { return &m_rootMotionDebugger; }

        // Manually end the root motion debugger update (call this if you explicitly skip executing the pose tasks)
        inline void EndRootMotionDebuggerUpdate( Transform const& endWorldTransform ) { m_rootMotionDebugger.EndCharacterUpdate( endWorldTransform ); }

        // Get the task system debug mode
        inline TaskSystemDebugMode GetTaskSystemDebugMode() const;

        // Set the task system debug mode
        inline void SetTaskSystemDebugMode( TaskSystemDebugMode mode );

        // Get the debug world transform that the task system used to execute
        Transform GetTaskSystemDebugWorldTransform();

        // Set the list of the debugs that we wish to explicitly debug. Set an empty list to debug everything!
        inline void SetNodeDebugFilterList( TVector<int16_t> const& filterList ) { m_debugFilterNodes = filterList; }

        // Get the runtime time info for a specified pose node
        inline PoseNodeDebugInfo GetPoseNodeDebugInfo( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            EE_ASSERT( m_nodes[nodeIdx]->GetValueType() == GraphValueType::Pose );
            auto pNode = reinterpret_cast<PoseNode const*>( m_nodes[nodeIdx] );
            return pNode->GetDebugInfo();
        }

        // Get the connected external graph instance
        GraphInstance const* GetChildGraphDebugInstance( int16_t nodeIdx ) const;

        // Get the connected external graph instance
        GraphInstance const* GetChildGraphDebugInstance( PointerID childGraphInstanceID ) const;

        // Get all child graphs - this will return all child graph instance (recursively)
        void GetChildGraphsForDebug( TVector<DebuggableChildGraph>& outChildGraphInstances, String const& pathPrefix = String() ) const;

        // Get the connected external graph instance
        GraphInstance const* GetExternalGraphDebugInstance( StringID slotID ) const;

        // Get all connected external graphs
        inline TVector<ExternalGraph> const& GetExternalGraphsForDebug() const { return m_externalGraphs; }

        // Get the value of a specified value node
        template<typename T>
        inline T GetRuntimeNodeDebugValue( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            auto pValueNode = reinterpret_cast<ValueNode*>( const_cast<GraphNode*>( m_nodes[nodeIdx] ) );
            return pValueNode->GetValue<T>( const_cast<GraphContext&>( m_graphContext ) );
        }

        // Get a node instance for debugging
        inline GraphNode const* GetNodeDebugInstance( int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            return m_nodes[nodeIdx];
        }

        // Get the runtime log for this graph instance
        TVector<GraphLogEntry> const& GetLog() const { return m_log; }

        // Log any errors/warnings occurring from the graph update!
        void OutputLog();

        // Draw graph debug visualizations
        void DrawDebug( Drawing::DrawContext& drawContext );
        #endif

        // Recording
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Are we currently recording this graph
        inline bool IsRecording() const { return m_pRecorder != nullptr; }

        // Start a graph recording
        void StartRecording( GraphRecorder* pRecorder );

        // Stop a graph recording
        void StopRecording();

        // Set the "per-frame update" data needed to evaluate a recorded graph execution
        void SetRecordedFrameUpdateData( RecordedGraphFrameData const& recordedUpdateData );

        // Record the current state of this graph
        void RecordGraphState( RecordedGraphState& recordedState );

        // Set to a previously recorded state
        void SetToRecordedState( RecordedGraphState const& recordedState );
        #endif

    private:

        explicit GraphInstance( GraphVariation const* pGraphVariation, uint64_t ownerID, TaskSystem* pTaskSystem );

        EE_FORCE_INLINE bool IsControlParameter( int16_t nodeIdx ) const { return nodeIdx < GetNumControlParameters(); }
        int32_t GetExternalGraphSlotIndex( StringID slotID ) const;
        int16_t GetExternalGraphNodeIndex( StringID slotID ) const;
        int32_t GetConnectedExternalGraphIndex( StringID slotID ) const;

        GraphInstance( GraphInstance const& ) = delete;
        GraphInstance( GraphInstance&& ) = delete;
        GraphInstance& operator=( GraphInstance const& ) = delete;
        GraphInstance& operator=( GraphInstance&& ) = delete;

        // Recording
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void RecordPreGraphEvaluateState( Seconds const deltaTime, Transform const& startWorldTransform );

        void RecordPostGraphEvaluateState( SyncTrackTimeRange const* pRange );

        // Record all the registered tasks for this update
        void RecordTasks();
        #endif

    private:

        GraphVariation const* const             m_pGraphVariation = nullptr;
        TVector<GraphNode*>                     m_nodes;
        uint8_t*                                m_pAllocatedInstanceMemory = nullptr;
        PoseNode*                               m_pRootNode = nullptr;
        uint64_t                                m_userID = 0; // An idea identifying the owner of this instance (usually the entity ID)

        TaskSystem*                             m_pTaskSystem = nullptr;
        GraphContext                            m_graphContext;
        TVector<ChildGraph>                     m_childGraphs;
        TVector<ExternalGraph>                  m_externalGraphs;

        #if EE_DEVELOPMENT_TOOLS
        TVector<int16_t>                        m_activeNodes;
        GraphDebugMode                          m_debugMode = GraphDebugMode::Off;
        RootMotionDebugger                      m_rootMotionDebugger; // Allows nodes to record root motion operations
        TVector<int16_t>                        m_debugFilterNodes; // The list of nodes that are allowed to debug draw (if this is empty all nodes will draw)
        TVector<GraphLogEntry>                  m_log;
        int32_t                                 m_lastOutputtedLogItemIdx = 0;
        GraphRecorder*                          m_pRecorder = nullptr;
        #endif
    };
}
