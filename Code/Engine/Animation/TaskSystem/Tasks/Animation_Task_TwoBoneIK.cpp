#include "Animation_Task_TwoBoneIK.h"
#include "Engine/Animation/IK/TwoBoneIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TwoBoneIKTask::TwoBoneIKTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, bool isTargetInWorldSpace, Target const& effectorTarget, IKBlendMode blendMode, float blendWeight, float referencePoseTwistWeight )
        : PoseTask( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_effectorBoneIdx( effectorBoneIdx )
        , m_referencePoseTwistWeight( referencePoseTwistWeight )
        , m_target( effectorTarget )
        , m_blendMode( blendMode )
        , m_blendWeight( Math::Clamp( blendWeight, 0.0f, 1.0f ) )
        , m_isTargetInWorldSpace( isTargetInWorldSpace )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_effectorBoneIdx != InvalidIndex );
        EE_ASSERT( m_target.IsTargetSet() );
        EE_ASSERT( blendWeight > 0.0f && blendWeight <= 1.0f ); // Dont register an IK task with zero weight!
        EE_ASSERT( m_referencePoseTwistWeight >= 0.0f && m_referencePoseTwistWeight <= 1.0f );
    }

    void TwoBoneIKTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pPose = pSourceBuffer->GetPrimaryPose();
        auto pSkeleton = pPose->GetSkeleton();
        int32_t const numBones = pSkeleton->GetNumBones();
        EE_ASSERT( m_effectorBoneIdx < numBones );

        // Set up target transform
        //-------------------------------------------------------------------------

        if ( !m_isRunningFromDeserializedData )
        {
            bool const bResult = m_target.TryGetTransform( pPose, m_targetTransform );
            EE_ASSERT( bResult );

            // Get the effector index
            if ( m_target.IsBoneTarget() )
            {
                m_targetBoneIdx = pSkeleton->GetBoneIndex( m_target.GetBoneID() );
            }

            // Convert to character space
            if ( !m_target.IsBoneTarget() && m_isTargetInWorldSpace )
            {
                m_targetTransform = m_targetTransform * context.m_worldTransformInverse;
            }
        }
        else // Running with deserialized data
        {
            // Optimization for bone-targets with no offsets
            if ( m_targetBoneIdx != InvalidIndex )
            {
                m_targetTransform = pPose->GetModelSpaceTransform( m_targetBoneIdx );
            }
        }

        // Perform solve
        //-------------------------------------------------------------------------

        TwoBoneSolver::Solve( *pPose, m_effectorBoneIdx, m_targetTransform, m_referencePoseTwistWeight, m_blendMode, m_blendWeight );

        MarkTaskComplete( context );
    }

    void TwoBoneIKTask::Serialize( TaskSerializer& serializer ) const
    {
        EE_ASSERT( !m_isRunningFromDeserializedData );

        serializer.WriteBoneIndex( m_effectorBoneIdx );

        //-------------------------------------------------------------------------

        // Optimization for pure bone targets
        if ( m_target.IsBoneTarget() && !m_target.HasOffsets() && m_targetBoneIdx != InvalidIndex )
        {
            serializer.WriteBool( true );
            serializer.WriteBoneIndex( m_targetBoneIdx );
        }
        else
        {
            serializer.WriteBool( false );
            serializer.WriteRotation( m_targetTransform.GetRotation() );
            serializer.WriteQuantizedTranslationForIK( m_targetTransform.GetTranslation(), s_quantizationRangeForTranslation );
        }

        //-------------------------------------------------------------------------

        serializer.WriteNormalizedFloat8Bit( m_referencePoseTwistWeight );

        bool const hasBlendWeight = m_blendWeight != 1.0f;
        serializer.WriteBool( hasBlendWeight );
        if ( hasBlendWeight )
        {
            serializer.WriteBool( m_blendMode == IKBlendMode::Effector );
            serializer.WriteNormalizedFloat8Bit( m_blendWeight != 1.0f );
        }
    }

    void TwoBoneIKTask::Deserialize( TaskSerializer& serializer )
    {
        m_effectorBoneIdx = serializer.ReadBoneIndex();

        //-------------------------------------------------------------------------

        bool bIsBoneTarget = serializer.ReadBool();
        if ( bIsBoneTarget )
        {
            m_targetBoneIdx = serializer.ReadBoneIndex();
        }
        else
        {
            Quaternion const q = serializer.ReadRotation();
            Vector const t = serializer.ReadQuantizedTranslationForIK( s_quantizationRangeForTranslation );
            m_targetTransform = Transform( q, t );
        }

        //-------------------------------------------------------------------------

        m_referencePoseTwistWeight = serializer.ReadNormalizedFloat8Bit();

        bool const hasBlendWeight = serializer.ReadBool();
        if ( hasBlendWeight )
        {
            bool const isEffectorBlend = serializer.ReadBool();
            m_blendMode = isEffectorBlend ? IKBlendMode::Effector : IKBlendMode::Pose;
            m_blendWeight = serializer.ReadNormalizedFloat8Bit();
        }
        else
        {
            m_blendWeight = 1.0f;
        }

        m_isRunningFromDeserializedData = true;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    InlineString TwoBoneIKTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        return InlineString( InlineString::CtorSprintf(), "Two Bone IK: %s", m_debugEffectorBoneID.c_str() );
    }

    void TwoBoneIKTask::DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        PoseTask::DrawDebug( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );

        if ( isDetailedViewEnabled && !m_isRunningFromDeserializedData )
        {
            Transform tmp = m_targetTransform * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 4.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Actual Target", Colors::LimeGreen );
        }
    }
    #endif
}
