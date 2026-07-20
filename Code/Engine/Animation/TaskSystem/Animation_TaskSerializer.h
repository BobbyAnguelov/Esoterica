#pragma once
#include "Engine/_Module/API.h"
#include "Animation_BoneMaskTask.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationTarget.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Serialization/BitSerialization.h"
#include "Base/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class PoseTask;
    class Skeleton;
    class AnimationClip;
    class GraphDefinition;

    //-------------------------------------------------------------------------

    using ResourceLUT = THashMap<uint64_t, Resource::ResourcePtr>;

    // Mapping table for all unique resources used by the task system
    //-------------------------------------------------------------------------

    class TaskSerializationContext
    {
    public:

        TaskSerializationContext() = default;

        // Ctor that generates the list of data from the provided resource handles - NOTE: order of secondary skeletons and external graphs is critical!
        TaskSerializationContext
        (
            TVector<TypeSystem::TypeInfo const*> const& taskTypes,
            GraphDefinition const* pGraphDef,
            TInlineVector<GraphDefinition const*, 3> const& externalGraphDefs,
            SecondarySkeletonList const& secondarySkeletons
        );

        // Ctor that populates the list of data from assets list - primarily for demo playback
        TaskSerializationContext
        (
            TVector<TypeSystem::TypeInfo const*> const& taskTypes,
            Skeleton const* pSkeleton,
            SecondarySkeletonList const& secondarySkeletons,
            TVector<StringID> const& uniqueBoneNames,
            TVector<StringID> const& uniqueBoneMaskNames,
            TVector<Resource::IResource const*> const& uniqueResources
        );

        void Clear();
        bool IsValid() const;

        // Create a hash of the state of the task serialization context - this is useful for validating that task lists can be deserialized correctly
        uint64_t CalculateHash() const;

        // Skeletons & Graph Defs
        //-------------------------------------------------------------------------

        Skeleton const* GetSkeleton() const { return m_pSkeleton; }
        SecondarySkeletonList const &GetSecondarySkeletons() const { return m_secondarySkeletons; }

        GraphDefinition const* GetMainGraphDef() const { return m_pGraphDef; }
        TInlineVector<GraphDefinition const*, 3> const &GetExternalGraphDefs() const { return m_externalGraphDefs; }

        // Tasks
        //-------------------------------------------------------------------------
        // List of all the task type names

        uint8_t GetNumTaskTypes() const;
        uint8_t GetTaskTypeIndex( PoseTask const* pTask ) const;
        PoseTask *CreateTaskFromTaskTypeIndex( uint8_t taskIdx ) const;
        inline uint32_t GetMaxNumBitsForTaskTypes() const { return m_maxBitsForTaskTypes; }

        // Bone Names
        //-------------------------------------------------------------------------
        // List of all unique bone names across all skeletons

        inline uint32_t GetNumBoneNames() const { return uint32_t( m_uniqueBoneNames.size() ); }
        int32_t GetBoneNameIndex( StringID boneNameID ) const;
        StringID GetBoneName( int32_t idx ) const { return m_uniqueBoneNames[idx]; }
        inline uint32_t GetMaxNumBitsForBoneNames() const { return m_maxBitsForBoneNames; }

        // Bone Masks
        //-------------------------------------------------------------------------
        // List of all unique bone mask IDs for the primary skeleton

        inline uint32_t GetNumBoneMasks() const { return uint32_t( m_uniqueBoneMaskNames.size() ); }
        int32_t GetBoneMaskIndex( StringID boneMaskID ) const;
        StringID GetBoneMaskName( int32_t idx ) const { return m_uniqueBoneMaskNames[idx]; }
        inline uint32_t GetMaxNumBitsForBoneMaskNames() const { return m_maxBitsForBoneMaskNames; }

        // Resources
        //-------------------------------------------------------------------------
        // List of all resources references by the graph and any externally injected graph

        inline uint32_t GetNumResources() const { return uint32_t( m_uniqueResources.size() ); }
        int32_t GetResourceIndex( ResourceID const& resourceID ) const;
        inline uint32_t GetMaxNumBitsForResources() const { return m_maxBitsForResources; }

        template<typename T>
        inline T const* GetResourcePtr( uint32_t resourceIdx ) const
        {
            // Convert to path ID
            EE_ASSERT( resourceIdx < m_uniqueResources.size() );
            Resource::IResource const* pResource = m_uniqueResources[resourceIdx];

            EE_ASSERT( pResource->GetResourceTypeID() == T::GetStaticResourceTypeID() );
            return (T const*) pResource;
        }

    private:

        void GenerateBoneNameList();
        void PopulateBoneNameList( TVector<StringID> const &uniqueBoneNames );

        void GenerateBoneMaskList();
        void PopulateBoneMaskList( TVector<StringID> const &uniqueBoneMaskNames );

        void GenerateResourceList();
        void PopulateResourceList( TVector<Resource::IResource const*> const &uniqueResources );

        void TryAddResourceToUniqueResources( Resource::IResource const* pResource )
        {
            if ( m_resourceIDToIdxMap.find( pResource->GetResourceID() ) == m_resourceIDToIdxMap.end() )
            {
                int32_t const uniqueResourceIdx = (int32_t) m_uniqueResources.size();
                m_uniqueResources.emplace_back( pResource );
                m_resourceIDToIdxMap.insert( TPair<ResourceID, int32_t>( pResource->GetResourceID(), uniqueResourceIdx ) );
            }
        }

    private:

        Skeleton const*                                         m_pSkeleton = nullptr;
        SecondarySkeletonList                                   m_secondarySkeletons;
        GraphDefinition const*                                  m_pGraphDef = nullptr;
        TInlineVector<GraphDefinition const*, 3>                m_externalGraphDefs;

        TVector<TypeSystem::TypeInfo const*>                    m_taskTypes;
        uint32_t                                                m_maxBitsForTaskTypes = 0;

        THashMap<StringID, int32_t>                             m_boneNameToIdxMap;
        TVector<StringID>                                       m_uniqueBoneNames;
        uint32_t                                                m_maxBitsForBoneNames = 0;

        THashMap<StringID, int32_t>                             m_boneMaskNameToIdxMap;
        TVector<StringID>                                       m_uniqueBoneMaskNames;
        uint32_t                                                m_maxBitsForBoneMaskNames = 0;

        THashMap<ResourceID, int32_t>                           m_resourceIDToIdxMap;
        TVector<Resource::IResource const*>                     m_uniqueResources;
        uint32_t                                                m_maxBitsForResources = 0;
    };

    // Task Serializer!
    //-------------------------------------------------------------------------

    class EE_ENGINE_API TaskSerializer : public Serialization::BitArchive
    {
        friend class TaskSystem;

    public:

        TaskSerializer( TaskSerializationContext const& taskSerializationContext );
        TaskSerializer( TaskSerializationContext const& taskSerializationContext, Blob const& inTopologyData, Blob const& inTaskData );

        // Get the number of serialized task in the provided blob
        uint8_t GetNumSerializedTasks() const { EE_ASSERT( IsReading() ); return m_numSerializedTasks; }

        // Serialization
        //-------------------------------------------------------------------------

        // Writes out a resource ptr
        // NOTE: This is not a robust solution as it does not take into account for external graphs being injected at runtime
        // We should replicate all LUT changes and update the serializer tables
        template<typename T>
        void WriteResourcePtr( T const* pResource )
        {
            uint32_t const resourceIdx = m_context.GetResourceIndex( pResource->GetResourceID() );
            WriteUInt( resourceIdx, m_context.GetMaxNumBitsForResources() );
        }

        // Write out a bone name
        void WriteBoneName( StringID boneName )
        {
            int32_t const boneNameIdx = m_context.GetBoneNameIndex( boneName );
            EE_ASSERT( boneNameIdx >= 0 ); // Dont encode invalid indices!
            WriteUInt( (uint32_t) boneNameIdx, m_context.GetMaxNumBitsForBoneNames() );
        }

        // Write out a bone index
        void WriteBoneIndex( int32_t boneIdx )
        {
            EE_ASSERT( boneIdx >= 0 );
            StringID const boneNameID = m_context.GetSkeleton()->GetBoneID( boneIdx );
            WriteBoneName( boneNameID );
        }

        // Write out a bone index
        void WriteBoneIndex( Skeleton const* pSkeleton, int32_t boneIdx )
        {
            EE_ASSERT( pSkeleton != nullptr );
            EE_ASSERT( boneIdx >= 0 );
            StringID const boneNameID = pSkeleton->GetBoneID( boneIdx );
            WriteBoneName( boneNameID );
        }

        // Write out a bone mask
        void WriteBoneMaskName( StringID boneMaskName )
        {
            int32_t const boneMaskNameIdx = m_context.GetBoneMaskIndex( boneMaskName );
            EE_ASSERT( boneMaskNameIdx >= 0 ); // Dont encode invalid indices!
            WriteUInt( (uint32_t) boneMaskNameIdx, m_context.GetMaxNumBitsForBoneMaskNames() );
        }

        // Write out a bone mask task list
        void WriteBoneMaskTaskList( BoneMaskTaskList const& taskList )
        {
            taskList.Serialize( *this, m_context.GetMaxNumBitsForBoneMaskNames() );
        }

        // Write out a rotation
        void WriteRotation( Quaternion const& rotation );

        // Write out a translation
        void WriteTranslation( Float3 const& translation )
        {
            WriteFloat( translation.m_x );
            WriteFloat( translation.m_y );
            WriteFloat( translation.m_z );
        }

        EE_FORCE_INLINE void WriteTranslation( Vector const& translation ) { WriteTranslation( translation.ToFloat3() ); }

        // Write out a quantized translation
        void WriteQuantizedTranslationForIK( Float3 const& translation, float symmetricMaxComponentValue )
        {
            EE_ASSERT( !IsReading() );

            WriteQuantizedFloat( translation.m_x, -symmetricMaxComponentValue, symmetricMaxComponentValue );
            WriteQuantizedFloat( translation.m_y, -symmetricMaxComponentValue, symmetricMaxComponentValue );
            WriteQuantizedFloat( translation.m_z, -symmetricMaxComponentValue, symmetricMaxComponentValue );
        }

        EE_FORCE_INLINE void WriteQuantizedTranslationForIK( Vector const& translation, float symmetricMaxComponentValue ) { WriteQuantizedTranslationForIK( translation.ToFloat3(), symmetricMaxComponentValue ); }

        // Deserialization
        //-------------------------------------------------------------------------

        // Read back a resource ptr
        // NOTE: This is not a robust solution as it does not take into account for external graphs being injected at runtime
        // We should replicate all LUT changes and update the serializer tables
        template<typename T>
        T const* ReadResourcePtr()
        {
            EE_ASSERT( IsReading() );

            // Read the index
            uint32_t const resourceIdx = (uint32_t) ReadUInt( m_context.GetMaxNumBitsForResources() );
            return m_context.GetResourcePtr<T>( resourceIdx );
        }

        // Reads back a bone name
        StringID ReadBoneName()
        {
            int32_t const boneNameIdx = (int32_t) ReadUInt( m_context.GetMaxNumBitsForBoneNames() );
            return m_context.GetBoneName( boneNameIdx );
        }

        // Reads back a bone name
        int32_t ReadBoneIndex( bool bRestrictToPrimarySkeleton = true, Skeleton const **ppOptionalOutSkeletonForIndex = nullptr );

        // Reads back a bone mask name
        StringID ReadBoneMaskName()
        {
            int32_t const boneMaskNameIdx = (int32_t) ReadUInt( m_context.GetMaxNumBitsForBoneMaskNames() );
            return m_context.GetBoneMaskName( boneMaskNameIdx );
        }

        // Reads back a bone mask task list
        void ReadBoneMaskTaskList( BoneMaskTaskList& outTaskList )
        {
            outTaskList.Reset();
            outTaskList.Deserialize( *this, m_context.GetMaxNumBitsForBoneMaskNames() );
        }

        // Reads back an animation target
        Quaternion ReadRotation();

        // Reads back an animation target
        Float3 ReadTranslation()
        {
            Float3 t;
            t.m_x = ReadFloat();
            t.m_y = ReadFloat();
            t.m_z = ReadFloat();
            return t;
        }

        // Write out a quantized translation
        Float3 ReadQuantizedTranslationForIK( float symmetricMaxComponentValue )
        {
            EE_ASSERT( IsReading() );

            Float3 t;
            t.m_x = ReadQuantizedFloat( -symmetricMaxComponentValue, symmetricMaxComponentValue );
            t.m_y = ReadQuantizedFloat( -symmetricMaxComponentValue, symmetricMaxComponentValue );
            t.m_z = ReadQuantizedFloat( -symmetricMaxComponentValue, symmetricMaxComponentValue );
            return t;
        }

    private:

        // Write
        //-------------------------------------------------------------------------

        // Write the number of tasks to serialize and calculate the number of bits need for dependencies
        void WriteNumTasks( uint8_t numTasksToSerialize )
        {
            EE_ASSERT( m_topologyData.IsWriting() );
            EE_ASSERT( numTasksToSerialize > 0 );
            EE_ASSERT( m_maxBitsForDependencies == 0 );

            m_numSerializedTasks = numTasksToSerialize;
            m_maxBitsForDependencies = Math::GetMaxNumberOfBitsForValue( m_numSerializedTasks );
            EE_ASSERT( m_maxBitsForDependencies <= 8 );

            m_topologyData.WriteUInt( m_maxBitsForDependencies, 4 ); // We only allow a maximum of 255 tasks
            m_topologyData.WriteUInt( numTasksToSerialize, m_maxBitsForDependencies );
        }

        // Write out a given task type as an index
        void WriteTaskType( PoseTask const* pTask )
        {
            EE_ASSERT( m_topologyData.IsWriting() );
            uint8_t const nTypeIdx = m_context.GetTaskTypeIndex( pTask );
            EE_ASSERT( nTypeIdx != 0xFF );
            m_topologyData.WriteUInt( nTypeIdx, m_context.GetMaxNumBitsForTaskTypes() );
        }

        // Write out a task dependency index, the max number of bits was set at serializer construction time
        void WriteDependencyIndex( int8_t index )
        {
            EE_ASSERT( IsWriting() );
            EE_ASSERT( index >= 0 ); // Dont encode invalid indices!
            m_topologyData.WriteUInt( index, m_maxBitsForDependencies );
        }

        // Read
        //-------------------------------------------------------------------------

        uint8_t ReadNumTasks()
        {
            EE_ASSERT( m_topologyData.IsReading() );
            EE_ASSERT( m_numSerializedTasks == 0 );
            EE_ASSERT( m_maxBitsForDependencies == 0 );

            m_maxBitsForDependencies = (uint8_t) m_topologyData.ReadUInt( 4 );
            EE_ASSERT( m_maxBitsForDependencies <= 8 );

            //-------------------------------------------------------------------------

            m_numSerializedTasks = (uint8_t) m_topologyData.ReadUInt( m_maxBitsForDependencies );
            EE_ASSERT( m_numSerializedTasks > 0 );
            return m_numSerializedTasks;
        }

        PoseTask *ReadTaskTypeAndCreateTask()
        {
            EE_ASSERT( m_topologyData.IsReading() );

            uint8_t const taskTypeID = (uint8_t) m_topologyData.ReadUInt( m_context.GetMaxNumBitsForTaskTypes() );
            PoseTask *pTask = m_context.CreateTaskFromTaskTypeIndex( taskTypeID );
            EE_ASSERT( pTask != nullptr );
            return pTask;
        }

        // Reads back a task dependency index, the max number of bits was read from the serialized stream
        int8_t ReadDependencyIndex()
        {
            EE_ASSERT( m_topologyData.IsReading() );
            return (int8_t) m_topologyData.ReadUInt( m_maxBitsForDependencies );
        }

    private:

        TaskSerializationContext const&                             m_context;
        uint8_t                                                     m_numSerializedTasks = 0;
        uint32_t                                                    m_maxBitsForDependencies = 0;
        Serialization::BitArchive                                   m_topologyData;
    };
}