#include "Animation_Task_AimIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    AimIKTask::AimIKTask( int8_t sourceTaskIdx, Vector const& worldSpaceTarget )
        : Task( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_worldSpaceTarget( worldSpaceTarget )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
    }

    void AimIKTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pPose = pSourceBuffer->GetPrimaryPose();

        // Generate Effector Data
        //-------------------------------------------------------------------------

        if ( m_generateEffectorData )
        {
            m_modelSpaceTarget = context.m_worldTransformInverse.TransformVector( m_worldSpaceTarget );
        }

        // Do IK
        //-------------------------------------------------------------------------

        int32_t tipIdx = pPose->GetSkeleton()->GetBoneIndex( StringID( "weaponTip" ) );
        int32_t buttIdx = pPose->GetSkeleton()->GetBoneIndex( StringID( "weaponEnd" ) );

        if ( tipIdx != InvalidIndex && buttIdx != InvalidIndex )
        {
            int32_t weaponIdx = pPose->GetSkeleton()->GetParentBoneIndex( buttIdx );
            int32_t weaponParentIdx = pPose->GetSkeleton()->GetParentBoneIndex( weaponIdx );

            Transform weaponParentTransform = pPose->GetModelSpaceTransform( weaponParentIdx );
            Transform weaponTransform = pPose->GetTransform( weaponIdx ) * weaponParentTransform;
            Transform buttTransform = pPose->GetTransform( buttIdx ) * weaponTransform;
            Transform tipTransform = pPose->GetTransform( tipIdx ) * weaponTransform;

            Transform inverseButtTransform = buttTransform.GetInverse();
            Transform tipRelativeToButt = tipTransform * inverseButtTransform;
            Transform weaponRelativeToButt = weaponTransform * inverseButtTransform;

            Vector const weaponDir = tipTransform.GetTranslation() - buttTransform.GetTranslation();
            Vector const newWeaponDir = m_modelSpaceTarget - buttTransform.GetTranslation();

            Quaternion deltaRotation = Quaternion::FromRotationBetweenVectors( weaponDir, newWeaponDir );
            
            Transform newButtTransform = buttTransform;
            newButtTransform.AddRotation( deltaRotation );

            Transform newTipTransform = tipRelativeToButt * newButtTransform;
            Transform newWeaponTransform = weaponRelativeToButt * newButtTransform;
            Transform inverseNewWeaponTransform = newWeaponTransform.GetInverse();

            pPose->SetTransform( weaponIdx, newWeaponTransform * weaponParentTransform.GetInverse() );
            pPose->SetTransform( buttIdx, newButtTransform * inverseNewWeaponTransform );
            pPose->SetTransform( tipIdx, newTipTransform * inverseNewWeaponTransform );
        }

        //-------------------------------------------------------------------------

        MarkTaskComplete( context );
    }

    void AimIKTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteFloat( m_modelSpaceTarget.GetX() );
        serializer.WriteFloat( m_modelSpaceTarget.GetY() );
        serializer.WriteFloat( m_modelSpaceTarget.GetZ() );
    }

    void AimIKTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 1 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_generateEffectorData = false;
    }

    #if EE_DEVELOPMENT_TOOLS
    void AimIKTask::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        Task::DrawDebug( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );
    }
    #endif
}