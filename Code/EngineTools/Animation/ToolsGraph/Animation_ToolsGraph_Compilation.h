#pragma once
#include "EngineTools/NodeGraph/NodeGraph_BaseGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class VariationHierarchy;

    //-------------------------------------------------------------------------

    enum NodeCompilationState
    {
        NeedCompilation,
        AlreadyCompiled,
    };

    //-------------------------------------------------------------------------

    struct NodeCompilationLogEntry
    {
        NodeCompilationLogEntry( Severity severity, UUID const& nodeID, String const& message )
            : m_message( message )
            , m_nodeID( nodeID )
            , m_severity( severity )
        {}

        String              m_message;
        UUID                m_nodeID;
        Severity            m_severity;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GraphCompilationContext
    {
        friend class GraphDefinitionCompiler;

    public:

        GraphCompilationContext() = default;
        GraphCompilationContext( GraphCompilationContext const& ) = default;
        ~GraphCompilationContext();

        GraphCompilationContext& operator=( GraphCompilationContext const& rhs ) = default;

        void Reset();

        // Variation Data
        //-------------------------------------------------------------------------

        void SetVariationData( VariationHierarchy const* pVariationHierarchy, StringID ID );
        
        VariationHierarchy const& GetVariationHierarchy() const { EE_ASSERT( m_pVariationHierarchy != nullptr ); return *m_pVariationHierarchy; }
        StringID GetVariationID() const { return m_variationID; }


        // Logging
        //-------------------------------------------------------------------------

        void LogMessage( NodeGraph::BaseNode const* pNode, char const* pFormat, ... )
        { 
            va_list args;
            va_start( args, pFormat );
            AddToLog( Severity::Info, pNode->GetID(), String( String::CtorSprintf(), pFormat, args ) );
            va_end( args );
        }

        void LogWarning( NodeGraph::BaseNode const* pNode, char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            AddToLog( Severity::Warning, pNode->GetID(), String( String::CtorSprintf(), pFormat, args ) );
            va_end( args );
        }

        void LogError( NodeGraph::BaseNode const* pNode, char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            AddToLog( Severity::Error, pNode->GetID(), String( String::CtorSprintf(), pFormat, args ) );
            va_end( args );
        }

        void LogMessage( char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            AddToLog( Severity::Info, UUID(), String( String::CtorSprintf(), pFormat, args ) );
            va_end( args );
        }

        void LogWarning( char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            AddToLog( Severity::Warning, UUID(), String( String::CtorSprintf(), pFormat, args ) );
            va_end( args );
        }

        void LogError( char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            AddToLog( Severity::Error, UUID(), String(String::CtorSprintf(), pFormat, args));
            va_end( args );
        }

        // General Compilation
        //-------------------------------------------------------------------------

        // Get the UUID to runtime index mapping
        inline THashMap<UUID, int16_t> const& GetUUIDToRuntimeIndexMap() const { return m_nodeIDToIndexMap; }

        // Get the runtime index to UUID mapping
        inline THashMap<int16_t, UUID> const& GetRuntimeIndexToUUIDMap() const { return m_nodeIndexToIDMap; }

        // Try to get the runtime settings for a node in the graph, will return whether this node was already compiled or still needs compilation
        template<typename T>
        NodeCompilationState GetDefinition( NodeGraph::BaseNode const* pNode, typename T::Definition*& pOutDefinition )
        {
            auto foundIter = m_nodeIDToIndexMap.find( pNode->GetID() );
            if ( foundIter != m_nodeIDToIndexMap.end() )
            {
                pOutDefinition = (typename T::Definition*) m_nodeDefinitions[foundIter->second];
                return NodeCompilationState::AlreadyCompiled;
            }

            //-------------------------------------------------------------------------

            EE_ASSERT( m_nodeDefinitions.size() < 0xFFFF );
            pOutDefinition = EE::New<typename T::Definition>();
            m_nodeDefinitions.emplace_back( pOutDefinition );
            m_compiledNodePaths.emplace_back( pNode->GetStringPathFromRoot() );
            pOutDefinition->m_nodeIdx = int16_t( m_nodeDefinitions.size() ) - 1;

            // Add to map
            m_nodeIDToIndexMap.insert( TPair<UUID, int16_t>( pNode->GetID(), pOutDefinition->m_nodeIdx ) );
            m_nodeIndexToIDMap.insert( TPair<int16_t, UUID >( pOutDefinition->m_nodeIdx, pNode->GetID() ) );

            // Add to persistent nodes list
            TryAddPersistentNode( pNode, pOutDefinition );

            // Update instance requirements
            m_graphInstanceRequiredAlignment = Math::Max( m_graphInstanceRequiredAlignment, (uint32_t) alignof( T ) );
            uint32_t const requiredNodePadding = (uint32_t) Memory::CalculatePaddingForAlignment( m_currentNodeMemoryOffset, alignof( T ) );

            // Set current node offset
            m_nodeMemoryOffsets.emplace_back( m_currentNodeMemoryOffset + requiredNodePadding );
            
            // Shift memory offset to take into account the current node size
            m_currentNodeMemoryOffset += uint32_t( sizeof( T ) + requiredNodePadding );

            return NodeCompilationState::NeedCompilation;
        }

        // This will return an index that can be used to look up the resource at runtime
        inline int16_t RegisterResource( ResourceID const& resourceID )
        {
            if ( !resourceID.IsValid() )
            {
                return InvalidIndex;
            }

            int32_t resourceIdx = VectorFindIndex( m_registeredResources, resourceID );
            if ( resourceIdx == InvalidIndex )
            {
                resourceIdx = (int32_t) m_registeredResources.size();
                m_registeredResources.emplace_back( resourceID );
            }

            return (int16_t) resourceIdx;
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

        // Record all referenced graph node indices
        inline int16_t RegisterReferencedGraphNode( int16_t nodeIdx, UUID const& nodeID, ResourceID const& graphDefinitionResourceID )
        {
            int16_t const resourceSlotIdx = RegisterResource( graphDefinitionResourceID );

            GraphDefinition::ReferencedGraphSlot const newSlot( nodeIdx, resourceSlotIdx );
            m_registeredReferencedGraphSlots.emplace_back( newSlot );
            return (int16_t) m_registeredReferencedGraphSlots.size() - 1;
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

        // Start compilation of transition conditions
        inline void BeginTransitionConditionsCompilation( Seconds transitionDuration, int16_t transitionDurationOverrideIdx )
        {
            m_transitionDuration = transitionDuration;
            m_transitionDurationOverrideIdx = transitionDurationOverrideIdx;
        }

        // End compilation of transition conditions
        inline void EndTransitionConditionsCompilation()
        {
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

        void TryAddPersistentNode( NodeGraph::BaseNode const* pNode, GraphNode::Definition* pDefinition );

        inline void AddToLog( Severity severity, UUID nodeID, String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( severity, nodeID, message ) ); }

    private:

        StringID                                        m_variationID;
        VariationHierarchy const*                       m_pVariationHierarchy = nullptr;

        TVector<NodeCompilationLogEntry>                m_log;

        THashMap<UUID, int16_t>                         m_nodeIDToIndexMap;
        THashMap<int16_t, UUID>                         m_nodeIndexToIDMap;
        TVector<int16_t>                                m_persistentNodeIndices;
        TVector<String>                                 m_compiledNodePaths;
        TVector<GraphNode::Definition*>                 m_nodeDefinitions;
        TVector<uint32_t>                               m_nodeMemoryOffsets;
        uint32_t                                        m_currentNodeMemoryOffset = 0;
        uint32_t                                        m_graphInstanceRequiredAlignment = alignof( bool );

        TVector<GraphDefinition::ReferencedGraphSlot>   m_registeredReferencedGraphSlots;
        TVector<GraphDefinition::ExternalGraphSlot>     m_registeredExternalGraphSlots;
        int16_t                                         m_conduitSourceStateCompiledNodeIdx = InvalidIndex;
        Seconds                                         m_transitionDuration = 0;
        int16_t                                         m_transitionDurationOverrideIdx = InvalidIndex;

        TVector<ResourceID>                             m_registeredResources;
    };
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ToolsGraphDefinition;
    class GraphDefinition;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GraphDefinitionCompiler
    {

    public:

        bool CompileGraph( ToolsGraphDefinition const& editorGraph, StringID variationID );

        inline GraphDefinition const* GetCompiledGraph() const { return &m_runtimeGraph; }
        inline TVector<NodeCompilationLogEntry> const& GetLog() const { return m_context.m_log; }
        inline THashMap<UUID, int16_t> const& GetUUIDToRuntimeIndexMap() const { return m_context.m_nodeIDToIndexMap; }
        inline THashMap<int16_t, UUID> const& GetRuntimeIndexToUUIDMap() const { return m_context.m_nodeIndexToIDMap; }

    private:

        GraphDefinition             m_runtimeGraph;
        GraphCompilationContext     m_context;
    };
}