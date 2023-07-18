#include "Animation_TaskSerializer.h"
#include "Animation_Task.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Encoding/Quantization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TaskSerializer::TaskSerializer( Skeleton const* pSkeleton, TInlineVector<ResourceLUT const*, 10> const& LUTs, uint8_t numTasksToSerialize )
        : BitArchive<1280>()
        , m_LUTs( LUTs )
        , m_numSerializedTasks( numTasksToSerialize )
    {
        m_maxBitsForDependencies = Math::GetMostSignificantBit( m_numSerializedTasks ) + 1;
        EE_ASSERT( m_maxBitsForDependencies <= 8 );

        m_maxBitsForBoneMask = Math::GetMostSignificantBit( pSkeleton->GetNumBoneMasks() ) + 1;
        EE_ASSERT( m_maxBitsForBoneMask <= 8 );

        // Serialize number of tasks
        WriteUInt( m_maxBitsForDependencies, 4 ); // We only allow a maximum of 255 tasks
        WriteUInt( numTasksToSerialize, m_maxBitsForDependencies );
    }

    TaskSerializer::TaskSerializer( Skeleton const* pSkeleton, TInlineVector<ResourceLUT const*, 10> const& LUTs, Blob const& inData )
        : BitArchive<1280>( inData )
        , m_LUTs( LUTs )
    {
        m_maxBitsForDependencies = (uint8_t) ReadUInt( 4 );
        EE_ASSERT( m_maxBitsForDependencies <= 8 );

        m_maxBitsForBoneMask = Math::GetMostSignificantBit( pSkeleton->GetNumBoneMasks() ) + 1;
        EE_ASSERT( m_maxBitsForBoneMask <= 8 );

        m_numSerializedTasks = (uint8_t) ReadUInt( m_maxBitsForDependencies );
    }

    //-------------------------------------------------------------------------

    void TaskSerializer::WriteDependencyIndex( int8_t index )
    {
        EE_ASSERT( IsWriting() );
        EE_ASSERT( index >= 0 ); // Dont encode invalid indices!
        WriteUInt( index, m_maxBitsForDependencies );
    }

    //-------------------------------------------------------------------------

    int8_t TaskSerializer::ReadDependencyIndex()
    {
        EE_ASSERT( IsReading() );
        return (int8_t) ReadUInt( m_maxBitsForDependencies );
    }
}