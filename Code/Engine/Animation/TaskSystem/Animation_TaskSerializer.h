#pragma once
#include "Engine/_Module/API.h"
#include "System/Resource/ResourcePtr.h"
#include "System/Serialization/BitSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Task;
    class Skeleton;

    //-------------------------------------------------------------------------

    using ResourceLUT = THashMap<uint32_t, Resource::ResourcePtr>;

    // Single use serializer!
    //-------------------------------------------------------------------------

    class EE_ENGINE_API TaskSerializer : public Serialization::BitArchive<1280>
    {

    public:

        TaskSerializer( Skeleton const* pSkeleton, TInlineVector<ResourceLUT const*, 10> const& LUTs, uint8_t numTasksToSerialize );
        TaskSerializer( Skeleton const* pSkeleton, TInlineVector<ResourceLUT const*, 10> const& LUTs, Blob const& inData );

        // Get the number of serialized task in the provided blob
        uint8_t GetNumSerializedTasks() const { EE_ASSERT( IsReading() ); return m_numSerializedTasks; }

        // Get the number of bits to use for bone mask indices
        uint32_t GetMaxBitsForBoneMaskIndex() const { return m_maxBitsForBoneMask; }

        // Serialization
        //-------------------------------------------------------------------------

        // Writes out a resource ptr
        template<typename T>
        void WriteResourcePtr( T const* pResource )
        {
            WriteUInt( pResource->GetResourceID().GetPathID(), 32 );
        }

        // Write out a task dependency index, the max number of bits was set at serializer construction time
        void WriteDependencyIndex( int8_t index );

        // Deserialization
        //-------------------------------------------------------------------------

        // Read back a resource ptr
        template<typename T>
        T const* ReadResourcePtr()
        {
            EE_ASSERT( IsReading() );
            uint32_t const pathID = ReadUInt( 32 );

            Resource::ResourcePtr ptr;
            for ( auto const& LUT : m_LUTs )
            {
                auto iter = LUT->find( pathID );
                if ( iter != LUT->end() )
                {
                    ptr = iter->second;
                    break;
                }
            }

            EE_ASSERT( ptr != nullptr );
            return ptr.GetPtr<T>();
        }

        // Reads back a task dependency index, the max number of bits was read from the serialized stream
        int8_t ReadDependencyIndex();

    private:

        TInlineVector<ResourceLUT const*, 10> const&                m_LUTs;
        uint8_t                                                     m_numSerializedTasks = 0;
        uint32_t                                                    m_maxBitsForDependencies = 8;
        uint32_t                                                    m_maxBitsForBoneMask;
    };
}