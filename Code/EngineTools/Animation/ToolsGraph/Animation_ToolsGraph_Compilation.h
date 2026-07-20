#pragma once
#include "EngineTools/NodeGraph/NodeGraph_BaseGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Base/TypeSystem/TypeRegistry.h"

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

        NodeCompilationLogEntry( Severity severity, UUID const& nodeID, InlineString const& message )
            : m_message( message.c_str() )
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
        ResourceID GetVariationSkeletonID() const;
        SkeletonResourceDescriptor const* GetVariationSkeletonDesc() const { return &m_variationSkeletonDescriptor; }

        // Logging
        //-------------------------------------------------------------------------

        void LogMessage( NodeGraph::BaseNode const* pNode, char const* pFormat, ... )
        { 
            InlineString str;

            va_list args;
            va_start( args, pFormat );
            str.sprintf_va_list( pFormat, args );
            va_end( args );

            AddToLog( Severity::Info, pNode->GetID(), str );
        }

        void LogWarning( NodeGraph::BaseNode const* pNode, char const* pFormat, ... )
        {
            InlineString str;

            va_list args;
            va_start( args, pFormat );
            str.sprintf_va_list( pFormat, args );
            va_end( args );

            AddToLog( Severity::Warning, pNode->GetID(), str );
        }

        void LogError( NodeGraph::BaseNode const* pNode, char const* pFormat, ... )
        {
            InlineString str;

            va_list args;
            va_start( args, pFormat );
            str.sprintf_va_list( pFormat, args );
            va_end( args );

            AddToLog( Severity::Error, pNode->GetID(), str );
        }

        void LogMessage( char const* pFormat, ... )
        {
            InlineString str;

            va_list args;
            va_start( args, pFormat );
            str.sprintf_va_list( pFormat, args );
            va_end( args );

            AddToLog( Severity::Info, UUID(), str );
        }

        void LogWarning( char const* pFormat, ... )
        {
            InlineString str;

            va_list args;
            va_start( args, pFormat );
            str.sprintf_va_list( pFormat, args );
            va_end( args );

            AddToLog( Severity::Warning, UUID(), str );
        }

        void LogError( char const* pFormat, ... )
        {
            InlineString str;

            va_list args;
            va_start( args, pFormat );
            str.sprintf_va_list( pFormat, args );
            va_end( args );

            AddToLog( Severity::Error, UUID(), str );
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

            // Update instance requirements
            m_graphInstanceRequiredAlignment = Math::Max( m_graphInstanceRequiredAlignment, (uint32_t) alignof( T ) );
            uint32_t const requiredNodePadding = (uint32_t) Memory::CalculatePaddingForAlignment( m_currentNodeMemoryOffset, alignof( T ) );

            // Set current node offset
            m_nodeMemoryOffsets.emplace_back( m_currentNodeMemoryOffset + requiredNodePadding );
            
            // Shift memory offset to take into account the current node size
            m_currentNodeMemoryOffset += uint32_t( sizeof( T ) + requiredNodePadding );

            return NodeCompilationState::NeedCompilation;
        }

        // Register a node as being persistent
        void RegisterPersistentNode( int16_t compiledNodeIdx )
        {
            if ( compiledNodeIdx != InvalidIndex && !VectorContains( m_persistentNodeIndices, compiledNodeIdx ) )
            {
                m_persistentNodeIndices.emplace_back( compiledNodeIdx );
            }
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

        // Record all compiled external pose nodes
        inline int16_t RegisterExternalPoseSlotNode( int16_t nodeIdx, StringID slotID )
        {
            GraphDefinition::ExternalPoseSlot const newSlot( nodeIdx, slotID );

            EE_ASSERT( nodeIdx != InvalidIndex && slotID.IsValid() );

            for ( auto const &existingSlot : m_registeredExternalPoseSlots )
            {
                EE_ASSERT( existingSlot.m_nodeIdx != nodeIdx && existingSlot.m_slotID != slotID );
            }

            // Add slot
            int16_t slotIdx = (int16_t) m_registeredExternalPoseSlots.size();
            m_registeredExternalPoseSlots.emplace_back( newSlot );
            return slotIdx;
        }

        // Record all referenced graph node indices
        inline int16_t RegisterReferencedGraphNode( int16_t nodeIdx, UUID const& nodeID, ResourceID const& graphDefinitionResourceID )
        {
            int16_t const resourceSlotIdx = RegisterResource( graphDefinitionResourceID );

            GraphDefinition::InternalGraphSlot const newSlot( nodeIdx, resourceSlotIdx );
            m_registeredInternalGraphSlots.emplace_back( newSlot );
            return (int16_t) m_registeredInternalGraphSlots.size() - 1;
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

        inline void AddToLog( Severity severity, UUID nodeID, String const& message ) { m_log.emplace_back( NodeCompilationLogEntry( severity, nodeID, message ) ); }
        inline void AddToLog( Severity severity, UUID nodeID, InlineString const& message ) { m_log.emplace_back( NodeCompilationLogEntry( severity, nodeID, message ) ); }

    private:

        StringID                                        m_variationID;
        VariationHierarchy const*                       m_pVariationHierarchy = nullptr;
        SkeletonResourceDescriptor                      m_variationSkeletonDescriptor;

        TVector<NodeCompilationLogEntry>                m_log;

        THashMap<UUID, int16_t>                         m_nodeIDToIndexMap;
        THashMap<int16_t, UUID>                         m_nodeIndexToIDMap;
        TVector<int16_t>                                m_persistentNodeIndices;
        TVector<String>                                 m_compiledNodePaths;
        TVector<GraphNode::Definition*>                 m_nodeDefinitions;
        TVector<uint32_t>                               m_nodeMemoryOffsets;
        uint32_t                                        m_currentNodeMemoryOffset = 0;
        uint32_t                                        m_graphInstanceRequiredAlignment = alignof( bool );

        TVector<GraphDefinition::InternalGraphSlot>     m_registeredInternalGraphSlots;
        TVector<GraphDefinition::ExternalGraphSlot>     m_registeredExternalGraphSlots;
        TVector<GraphDefinition::ExternalPoseSlot>      m_registeredExternalPoseSlots;
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

        GraphDefinitionCompiler( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDataPath );

        bool CompileGraph( ToolsGraphDefinition const& editorGraph, StringID variationID );

        inline GraphDefinition const* GetCompiledGraph() const { return &m_runtimeGraph; }
        inline TVector<NodeCompilationLogEntry> const& GetLog() const { return m_context.m_log; }
        inline THashMap<UUID, int16_t> const& GetUUIDToRuntimeIndexMap() const { return m_context.m_nodeIDToIndexMap; }
        inline THashMap<int16_t, UUID> const& GetRuntimeIndexToUUIDMap() const { return m_context.m_nodeIndexToIDMap; }

    private:

        TypeSystem::TypeRegistry const&     m_typeRegistry;
        FileSystem::Path const&             m_sourceDataPath;
        GraphDefinition                     m_runtimeGraph;
        GraphCompilationContext             m_context;
    };
}