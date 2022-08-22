#pragma once
#include "Animation_EditorGraph_FlowGraph.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum NodeCompilationState
    {
        NeedCompilation,
        AlreadyCompiled,
    };

    //-------------------------------------------------------------------------

    struct NodeCompilationLogEntry
    {
        NodeCompilationLogEntry( Log::Severity severity, UUID const& nodeID, String const& message )
            : m_message( message )
            , m_nodeID( nodeID )
            , m_severity( severity )
        {}

        String              m_message;
        UUID                m_nodeID;
        Log::Severity       m_severity;
    };

    //-------------------------------------------------------------------------

    class GraphCompilationContext
    {
        friend class GraphDefinitionCompiler;

    public:

        GraphCompilationContext();
        ~GraphCompilationContext();

        void Reset();

        // Logging
        //-------------------------------------------------------------------------

        void LogMessage( VisualGraph::BaseNode const* pNode, String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( Log::Severity::Message, pNode->GetID(), message ) ); }
        void LogWarning( VisualGraph::BaseNode const* pNode, String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( Log::Severity::Warning, pNode->GetID(), message ) ); }
        void LogError( VisualGraph::BaseNode const* pNode, String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( Log::Severity::Error, pNode->GetID(), message ) ); }

        void LogMessage( String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( Log::Severity::Message, UUID(), message ) ); }
        void LogWarning( String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( Log::Severity::Warning, UUID(), message ) ); }
        void LogError( String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( Log::Severity::Error, UUID(), message ) ); }

        // General Compilation
        //-------------------------------------------------------------------------

        inline THashMap<UUID, int16_t> GetIDToIndexMap() const { return m_nodeIDToIndexMap; }

        // Try to get the runtime settings for a node in the graph, will return whether this node was already compiled or still needs compilation
        template<typename T>
        NodeCompilationState GetSettings( VisualGraph::BaseNode const* pNode, typename T::Settings*& pOutSettings )
        {
            auto foundIter = m_nodeIDToIndexMap.find( pNode->GetID() );
            if ( foundIter != m_nodeIDToIndexMap.end() )
            {
                pOutSettings = (T::Settings*) m_nodeSettings[foundIter->second];
                return NodeCompilationState::AlreadyCompiled;
            }

            //-------------------------------------------------------------------------

            EE_ASSERT( m_nodeSettings.size() < 0xFFFF );
            pOutSettings = EE::New<T::Settings>();
            m_nodeSettings.emplace_back( pOutSettings );
            m_compiledNodePaths.emplace_back( pNode->GetPathFromRoot() );
            pOutSettings->m_nodeIdx = int16_t( m_nodeSettings.size() ) - 1;

            // Add to map
            m_nodeIDToIndexMap.insert( TPair<UUID, int16_t>( pNode->GetID(), pOutSettings->m_nodeIdx ) );

            // Add to persistent nodes list
            auto pFlowNode = TryCast<GraphNodes::EditorGraphNode>( pNode );
            if ( pFlowNode != nullptr && pFlowNode->IsPersistentNode() )
            {
                m_persistentNodeIndices.emplace_back( pOutSettings->m_nodeIdx );
            }

            // Update instance requirements
            m_graphInstanceRequiredAlignment = Math::Max( m_graphInstanceRequiredAlignment, (uint32_t) alignof( T ) );
            uint32_t const requiredNodePadding = (uint32_t) Memory::CalculatePaddingForAlignment( m_currentNodeMemoryOffset, alignof( T ) );

            // Set current node offset
            m_nodeMemoryOffsets.emplace_back( m_currentNodeMemoryOffset + requiredNodePadding );
            
            // Shift memory offset to take into account the current node size
            m_currentNodeMemoryOffset += uint32_t( sizeof( T ) + requiredNodePadding );

            return NodeCompilationState::NeedCompilation;
        }

        // This will return an index that can be used to look up the data resource at runtime
        inline int16_t RegisterDataSlotNode( UUID const& nodeID )
        {
            EE_ASSERT( !VectorContains( m_registeredDataSlots, nodeID ) );

            int16_t slotIdx = (int16_t) m_registeredDataSlots.size();
            m_registeredDataSlots.emplace_back( nodeID );
            return slotIdx;
        }

        // Record all compiled external graph nodes
        inline int16_t RegisterExternalGraphSlotNode( int16_t nodeIdx, StringID slotID )
        {
            GraphDefinition::ExternalGraphSlot const newSlot( nodeIdx, slotID );

            EE_ASSERT( nodeIdx != InvalidIndex && slotID.IsValid() );

            for ( auto const& existingSlot : m_registeredExternalGraphSlots )
            {
                EE_ASSERT( existingSlot.m_nodeIdx != nodeIdx && existingSlot.m_slotID != slotID );
            }

            // Add slot
            int16_t slotIdx = (int16_t) m_registeredExternalGraphSlots.size();
            m_registeredExternalGraphSlots.emplace_back( newSlot );
            return slotIdx;
        }

        // Record all child graph node indices
        inline int16_t RegisterChildGraphNode( int16_t nodeIdx, UUID const& nodeID )
        {
            int16_t const dataSlotIdx = RegisterDataSlotNode( nodeID );

            GraphDefinition::ChildGraphSlot const newSlot( nodeIdx, dataSlotIdx );
            m_registeredChildGraphSlots.emplace_back( newSlot );
            return (int16_t) m_registeredChildGraphSlots.size() - 1;
        }

        // State Machine Compilation
        //-------------------------------------------------------------------------

        // Start compilation of a transition conduit
        inline void BeginConduitCompilation( int16_t sourceStateNodeIdx )
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx == InvalidIndex );
            EE_ASSERT( sourceStateNodeIdx != InvalidIndex );
            m_conduitSourceStateCompiledNodeIdx = sourceStateNodeIdx;
        }

        // End compilation of a transition conduit
        inline void EndConduitCompilation()
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx != InvalidIndex );
            m_conduitSourceStateCompiledNodeIdx = InvalidIndex;
        }

        // Some nodes optionally need the conduit index so we need to have a way to know what mode we are in
        inline bool IsCompilingConduit() const
        {
            return m_conduitSourceStateCompiledNodeIdx != InvalidIndex;
        }

        inline int16_t GetConduitSourceStateIndex() const
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx != InvalidIndex );
            return m_conduitSourceStateCompiledNodeIdx;
        }

        // Start compilation of a transition conduit
        inline void BeginTransitionConditionsCompilation( Seconds transitionDuration, int16_t transitionDurationOverrideIdx )
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx != InvalidIndex );
            m_transitionDuration = transitionDuration;
            m_transitionDurationOverrideIdx = transitionDurationOverrideIdx;
        }

        // End compilation of a transition conduit
        inline void EndTransitionConditionsCompilation()
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx != InvalidIndex );
            m_transitionDuration = 0;
            m_transitionDurationOverrideIdx = InvalidIndex;
        }

        // Get the node index for a transition duration value node chain if set
        inline int16_t GetCompiledTransitionDurationOverrideIdx() const
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx != InvalidIndex );
            return m_transitionDurationOverrideIdx;
        }

        // Get the duration of a transition (if not using an override)
        inline Seconds GetCompiledTransitionDuration() const
        {
            EE_ASSERT( m_conduitSourceStateCompiledNodeIdx != InvalidIndex );
            return m_transitionDuration;
        }

    private:

        TVector<NodeCompilationLogEntry>                m_log;
        THashMap<UUID, int16_t>                         m_nodeIDToIndexMap;
        TVector<int16_t>                                m_persistentNodeIndices;
        TVector<String>                                 m_compiledNodePaths;
        TVector<GraphNode::Settings*>                   m_nodeSettings;
        TVector<uint32_t>                               m_nodeMemoryOffsets;
        uint32_t                                        m_currentNodeMemoryOffset = 0;
        uint32_t                                        m_graphInstanceRequiredAlignment = alignof( bool );

        TVector<UUID>                                   m_registeredDataSlots;
        TVector<GraphDefinition::ChildGraphSlot>        m_registeredChildGraphSlots;
        TVector<GraphDefinition::ExternalGraphSlot>     m_registeredExternalGraphSlots;
        int16_t                                         m_conduitSourceStateCompiledNodeIdx = InvalidIndex;
        Seconds                                         m_transitionDuration = 0;
        int16_t                                         m_transitionDurationOverrideIdx = InvalidIndex;
    };
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EditorGraphDefinition;
    class GraphDefinition;

    //-------------------------------------------------------------------------

    class GraphDefinitionCompiler
    {

    public:

        bool CompileGraph( EditorGraphDefinition const& editorGraph );

        inline GraphDefinition const* GetCompiledGraph() const { return &m_runtimeGraph; }
        inline TVector<NodeCompilationLogEntry> const& GetLog() const { return m_context.m_log; }
        inline TVector<UUID> const& GetRegisteredDataSlots() const { return m_context.m_registeredDataSlots; }
        inline THashMap<UUID, int16_t> const& GetIDToIndexMap() const { return m_context.m_nodeIDToIndexMap; }

    private:

        GraphDefinition             m_runtimeGraph;
        GraphCompilationContext     m_context;
    };
}