#include "Animation_TaskSerializer.h"
#include "Animation_Task.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Encoding/Quantization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ResourceMappings::Generate( TInlineVector<ResourceLUT const*, 10> const& LUTs )
    {
        m_resourceIDToIdxMap.clear();
        m_uniqueResourceIDs.clear();
        m_LUTs = LUTs;

        //-------------------------------------------------------------------------

        uint32_t resourceIdx = 0;
        for ( auto const& pLUT : m_LUTs )
        {
            for ( auto const& pair : *pLUT )
            {
                // Do we already have a record for this resource? We only create records for unique resources!
                auto iter = m_resourceIDToIdxMap.find( pair.first );
                if ( iter == m_resourceIDToIdxMap.end() )
                {
                    m_resourceIDToIdxMap.insert( TPair<uint32_t, uint32_t>( pair.first, resourceIdx ) );
                    m_uniqueResourceIDs.emplace_back( pair.first );
                    resourceIdx++;
                }
            }
        }
        EE_ASSERT( resourceIdx == m_uniqueResourceIDs.size() );
    }

    //-------------------------------------------------------------------------

    TaskSerializer::TaskSerializer( Skeleton const* pSkeleton, ResourceMappings const& resourceMappings, uint8_t numTasksToSerialize )
        : BitArchive<1280>()
        , m_pSkeleton( pSkeleton )
        , m_resourceMappings( resourceMappings )
        , m_numSerializedTasks( numTasksToSerialize )
    {
        m_maxBitsForDependencies = Math::GetMaxNumberOfBitsForValue( m_numSerializedTasks );
        EE_ASSERT( m_maxBitsForDependencies <= 8 );

        m_maxBitsForBoneMask = Math::GetMaxNumberOfBitsForValue( pSkeleton->GetNumBoneMasks() );
        EE_ASSERT( m_maxBitsForBoneMask <= 8 );

        m_maxBitsForResourceIDs = Math::GetMaxNumberOfBitsForValue( m_resourceMappings.GetNumMappings() );
        EE_ASSERT( m_maxBitsForResourceIDs > 0 && m_maxBitsForResourceIDs <= 16 );

        m_maxBitsForBoneIndices = Math::GetMaxNumberOfBitsForValue( pSkeleton->GetNumBones() );
        EE_ASSERT( m_maxBitsForBoneIndices > 0 && m_maxBitsForBoneIndices <= 16 );

        // Serialize number of tasks
        WriteUInt( m_maxBitsForDependencies, 4 ); // We only allow a maximum of 255 tasks
        WriteUInt( numTasksToSerialize, m_maxBitsForDependencies );
    }

    TaskSerializer::TaskSerializer( Skeleton const* pSkeleton, ResourceMappings const& resourceMappings, Blob const& inData )
        : BitArchive<1280>( inData )
        , m_pSkeleton( pSkeleton )
        , m_resourceMappings( resourceMappings )
    {
        m_maxBitsForDependencies = (uint8_t) ReadUInt( 4 );
        EE_ASSERT( m_maxBitsForDependencies <= 8 );

        m_maxBitsForBoneMask = Math::GetMaxNumberOfBitsForValue( pSkeleton->GetNumBoneMasks() );
        EE_ASSERT( m_maxBitsForBoneMask <= 8 );

        m_maxBitsForResourceIDs = Math::GetMaxNumberOfBitsForValue( m_resourceMappings.GetNumMappings() );
        EE_ASSERT( m_maxBitsForResourceIDs > 0 && m_maxBitsForResourceIDs <= 16 );

        m_maxBitsForBoneIndices = Math::GetMaxNumberOfBitsForValue( pSkeleton->GetNumBones() );
        EE_ASSERT( m_maxBitsForBoneIndices > 0 && m_maxBitsForBoneIndices <= 16 );

        m_numSerializedTasks = (uint8_t) ReadUInt( m_maxBitsForDependencies );
    }

    //-------------------------------------------------------------------------

    void TaskSerializer::WriteDependencyIndex( int8_t index )
    {
        EE_ASSERT( IsWriting() );
        EE_ASSERT( index >= 0 ); // Dont encode invalid indices!
        WriteUInt( index, m_maxBitsForDependencies );
    }

    void TaskSerializer::WriteBoneIndex( int32_t boneIdx )
    {
        EE_ASSERT( boneIdx >= 0 ); // Dont encode invalid indices!
        WriteUInt( (uint32_t) boneIdx, m_maxBitsForBoneIndices );
    }

    void TaskSerializer::WriteRotation( Quaternion const& rotation )
    {
        EE_ASSERT( rotation.IsNormalized() );

        Quantization::EncodedQuaternion q( rotation );
        WriteUInt( q.m_data0, 16 );
        WriteUInt( q.m_data1, 16 );
        WriteUInt( q.m_data2, 16 );
    }

    void TaskSerializer::WriteTranslation( Float3 const& translation )
    {
        WriteFloat( translation.m_x );
        WriteFloat( translation.m_y );
        WriteFloat( translation.m_z );
    }

    //-------------------------------------------------------------------------

    int8_t TaskSerializer::ReadDependencyIndex()
    {
        EE_ASSERT( IsReading() );
        return (int8_t) ReadUInt( m_maxBitsForDependencies );
    }

    int32_t TaskSerializer::ReadBoneIndex()
    {
        return (int32_t) ReadUInt( m_maxBitsForBoneIndices );
    }

    Quaternion TaskSerializer::ReadRotation()
    {
        Quantization::EncodedQuaternion q;
        q.m_data0 = (uint16_t) ReadUInt( 16 );
        q.m_data1 = (uint16_t) ReadUInt( 16 );
        q.m_data2 = (uint16_t) ReadUInt( 16 );
        return q.ToQuaternion();
    }

    Float3 TaskSerializer::ReadTranslation()
    {
        Float3 t;
        t.m_x = ReadFloat();
        t.m_y = ReadFloat();
        t.m_z = ReadFloat();
        return t;
    }
}