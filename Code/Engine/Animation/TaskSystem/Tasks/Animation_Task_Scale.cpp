#include "Animation_Task_Scale.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Engine/Animation/TaskSystem/Animation_BoneMaskPool.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct TransformAccessor
    {
        static void ApplyScale( Pose *pPose, BoneMask const& mask )
        {
            if ( mask.IsZeroWeightMask() )
            {
                pPose->m_parentSpaceTransforms[0].SetScale( 0.0f );
            }
            else if ( mask.IsFullWeightMask() )
            {
                return;
            }
            else // Apply scale
            {
                int32_t const numBones = pPose->GetNumBones();
                for ( int32_t i = 0; i < numBones; i++ )
                {
                    float const scale = pPose->m_parentSpaceTransforms[i].GetScale() * mask.GetWeight( i );
                    pPose->m_parentSpaceTransforms[i].SetScale( scale );
                }
            }
        }
    };
}

namespace EE::Animation
{
    ScaleTask::ScaleTask( int8_t sourceTaskIdx, BoneMaskTaskList const& boneMaskTaskList )
        : PoseTask( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_boneMaskTaskList( boneMaskTaskList )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_boneMaskTaskList.HasTasks() );
    }

    void ScaleTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );

        //-------------------------------------------------------------------------

        auto pPrimaryPose = pSourceBuffer->GetPrimaryPose();
        int8_t const boneMaskBufferIdx = m_boneMaskTaskList.GenerateBoneMask( pPrimaryPose->GetSkeleton(), context.m_boneMaskPool );
        if ( boneMaskBufferIdx != InvalidIndex )
        {
            BoneMaskBuffer const* pBoneMaskBuffer = context.m_boneMaskPool.GetBuffer( boneMaskBufferIdx );

            for ( Pose& pose : pSourceBuffer->m_poses )
            {
                BoneMask const* pMask = pBoneMaskBuffer->TryGetBoneMask( pose.GetSkeleton() );
                if ( pMask != nullptr )
                {
                    TransformAccessor::ApplyScale( &pose, *pMask );
                }
            }

            context.m_boneMaskPool.ReleaseMaskBuffer( boneMaskBufferIdx );
        }

        //-------------------------------------------------------------------------

        MarkTaskComplete( context );
    }

    void ScaleTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteBoneMaskTaskList( m_boneMaskTaskList );
    }

    void ScaleTask::Deserialize( TaskSerializer& serializer )
    {
        serializer.ReadBoneMaskTaskList( m_boneMaskTaskList );
    }
}