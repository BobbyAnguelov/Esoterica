#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationTarget.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Serialization/BitSerialization.h"
#include "Base/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Task;
    class Skeleton;

    //-------------------------------------------------------------------------

    using ResourceLUT = THashMap<uint32_t, Resource::ResourcePtr>;

    // Mapping table for all unique resources used by the task system
    //-------------------------------------------------------------------------

    class ResourceMappings
    {
    public:

        ResourceMappings() = default;
        ResourceMappings( TInlineVector<ResourceLUT const*, 10> const& LUTs ) { Generate( LUTs ); }

        void Generate( TInlineVector<ResourceLUT const*, 10> const& LUTs );

        void Clear()
        {
            m_LUTs.clear();
            m_resourceIDToIdxMap.clear();
            m_uniqueResourceIDs.clear();
        }

        inline uint32_t GetResourceIndex( ResourceID const& ID ) const
        {
            return m_resourceIDToIdxMap.at( ID.GetPathID() );
        }

        template<typename T>
        inline T const* GetResourcePtr( uint32_t resourceIdx ) const
        {
            // Convert to path ID
            EE_ASSERT( resourceIdx < m_uniqueResourceIDs.size() );
            uint32_t const pathID = m_uniqueResourceIDs[resourceIdx];

            Resource::ResourcePtr ptr;
            for ( ResourceLUT const* pLUT : m_LUTs )
            {
                auto iter = pLUT->find( pathID );
                if ( iter != pLUT->end() )
                {
                    ptr = iter->second;
                    break;
                }
            }

            EE_ASSERT( ptr != nullptr );
            return ptr.GetPtr<T>();
        }

        inline uint32_t GetNumMappings() const { return uint32_t( m_uniqueResourceIDs.size() ); }

    private:

        TInlineVector<ResourceLUT const*, 10>                       m_LUTs;
        THashMap<uint32_t, uint32_t>                                m_resourceIDToIdxMap;
        TVector<uint32_t>                                           m_uniqueResourceIDs;
    };

    // Task Serializer!
    //-------------------------------------------------------------------------

    class EE_ENGINE_API TaskSerializer : public Serialization::BitArchive<1280>
    {

    public:

        TaskSerializer( Skeleton const* pSkeleton, ResourceMappings const& resourceMappings, uint8_t numTasksToSerialize );
        TaskSerializer( Skeleton const* pSkeleton, ResourceMappings const& resourceMappings, Blob const& inData );

        // Get the number of serialized task in the provided blob
        uint8_t GetNumSerializedTasks() const { EE_ASSERT( IsReading() ); return m_numSerializedTasks; }

        // Get the number of bits to use for bone mask indices
        uint32_t GetMaxBitsForBoneMaskIndex() const { return m_maxBitsForBoneMask; }

        // Serialization
        //-------------------------------------------------------------------------

        // Writes out a resource ptr
        // NOTE: This is not a robust solution as it does not take into account for external graphs being injected at runtime
        // We should replicate all LUT changes and update the serializer tables
        template<typename T>
        void WriteResourcePtr( T const* pResource )
        {
            uint32_t const resourceIdx = m_resourceMappings.GetResourceIndex( pResource->GetResourceID() );
            WriteUInt( resourceIdx, m_maxBitsForResourceIDs );
        }

        // Write out a task dependency index, the max number of bits was set at serializer construction time
        void WriteDependencyIndex( int8_t index );

        // Write out a bone index for the skeleton we are using
        void WriteBoneIndex( int32_t boneIdx );

        // Write out a rotation
        void WriteRotation( Quaternion const& rotation );

        // Write out a translation
        void WriteTranslation( Float3 const& translation );
        inline void WriteTranslation( Vector const& translation ) { WriteTranslation( translation.ToFloat3() ); }

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
            uint32_t const resourceIdx = (uint32_t) ReadUInt( m_maxBitsForResourceIDs );
            return m_resourceMappings.GetResourcePtr<T>( resourceIdx );
        }

        // Reads back a task dependency index, the max number of bits was read from the serialized stream
        int8_t ReadDependencyIndex();

        // Reads back a bone index for the skeleton we are using
        int32_t ReadBoneIndex();

        // Reads back an animation target
        Quaternion ReadRotation();

        // Reads back an animation target
        Float3 ReadTranslation();

    private:

        Skeleton const*                                             m_pSkeleton = nullptr;
        ResourceMappings const&                                     m_resourceMappings;
        uint8_t                                                     m_numSerializedTasks = 0;
        uint32_t                                                    m_maxBitsForDependencies = 8;
        uint32_t                                                    m_maxBitsForBoneMask = 0;
        uint32_t                                                    m_maxBitsForResourceIDs = 0;
        uint32_t                                                    m_maxBitsForBoneIndices = 0;
    };
}