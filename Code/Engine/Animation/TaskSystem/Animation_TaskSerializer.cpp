#include "Animation_TaskSerializer.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "System/Encoding/Quantization.h"

//-------------------------------------------------------------------------

#include "Tasks/Animation_Task_Sample.h"
#include "Tasks/Animation_Task_Blend.h"
#include "Tasks/Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    uint8_t TaskSerializer::GetNumTaskTypes()
    {
        return 3;
    }

    uint8_t TaskSerializer::GetSerializedTaskTypeID( Task const* pTask )
    {
        switch ( pTask->GetTaskTypeID() )
        {
            case Tasks::DefaultPoseTask::s_taskTypeID:
            return 0;
            break;

            case Tasks::SampleTask::s_taskTypeID:
            return 1;
            break;

            case Tasks::BlendTask::s_taskTypeID:
            return 2;
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        return 0xFF;
    }

    Task* TaskSerializer::CreateTask( uint8_t serializedTaskTypeID )
    {
        switch ( serializedTaskTypeID )
        {
            // DefaultPoseTask
            case 0:
            {
                Task* pTask = (Task*) EE::Alloc( sizeof( Tasks::DefaultPoseTask ) );
                new ( pTask ) Tasks::DefaultPoseTask();
                return pTask;
            }
            break;

            // SampleTask
            case 1:
            {
                Task* pTask = (Task*) EE::Alloc( sizeof( Tasks::SampleTask ) );
                new ( pTask ) Tasks::SampleTask();
                return pTask;
            }
            break;

            // BlendTask
            case 2:
            {
                Task* pTask = (Task*) EE::Alloc( sizeof( Tasks::BlendTask ) );
                new ( pTask ) Tasks::BlendTask();
                return pTask;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    TaskSerializer::TaskSerializer( Skeleton const* pSkeleton, TInlineVector<ResourceLUT const*, 10> const& LUTs, uint8_t numTasksToSerialize )
        : BitArchive<1280>()
        , m_LUTs( LUTs )
        , m_numSerializedTasks( numTasksToSerialize )
        , m_maxBitsForTaskTypeID( Math::GetMostSignificantBit( GetNumTaskTypes() ) + 1 )
    {
        EE_ASSERT( m_maxBitsForTaskTypeID <= 8 );

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
        , m_maxBitsForTaskTypeID( Math::GetMostSignificantBit( GetNumTaskTypes() ) + 1 )
    {
        EE_ASSERT( m_maxBitsForTaskTypeID <= 8 );

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

    void TaskSerializer::WriteTaskTypeID( uint8_t ID )
    {
        EE_ASSERT( IsWriting() );
        WriteUInt( ID, m_maxBitsForTaskTypeID );
    }

    //-------------------------------------------------------------------------

    int8_t TaskSerializer::ReadDependencyIndex()
    {
        EE_ASSERT( IsReading() );
        return (int8_t) ReadUInt( m_maxBitsForDependencies );
    }

    uint8_t TaskSerializer::ReadTaskTypeID()
    {
        EE_ASSERT( IsReading() );
        return (uint8_t) ReadUInt( m_maxBitsForTaskTypeID );
    }
}