#include "Animation_Task_FootIK.h"
#include "Engine/Animation/IK/TwoBoneIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    FootIKTask::FootIKTask( int8_t sourceTaskIdx, int32_t leftFootBoneIdx, int32_t rightFootBoneIdx, bool isTargetInWorldSpace, Target const& leftTarget, Target const& rightTarget, IKBlendMode blendMode, float blendWeight )
        : PoseTask( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_leftFootBoneIdx( leftFootBoneIdx )
        , m_rightFootBoneIdx( rightFootBoneIdx )
        , m_leftTarget( leftTarget )
        , m_rightTarget( rightTarget )
        , m_blendMode( blendMode )
        , m_blendWeight( Math::Clamp( blendWeight, 0.0f, 1.0f ) )
        , m_isTargetInWorldSpace( isTargetInWorldSpace )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_leftFootBoneIdx != InvalidIndex );
        EE_ASSERT( m_rightFootBoneIdx != InvalidIndex );
        EE_ASSERT( m_leftTarget.IsTargetSet() );
        EE_ASSERT( m_rightTarget.IsTargetSet() );
        EE_ASSERT( blendWeight > 0.0f && blendWeight <= 1.0f ); // Dont register an IK task with zero weight!
    }

    void FootIKTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pPose = pSourceBuffer->GetPrimaryPose();
        auto pSkeleton = pPose->GetSkeleton();

        int32_t const numBones = pSkeleton->GetNumBones();
        EE_ASSERT( m_leftFootBoneIdx < numBones );
        EE_ASSERT( m_rightFootBoneIdx < numBones );

        // Set up target transform
        //-------------------------------------------------------------------------

        if ( !m_isRunningFromDeserializedData )
        {
            bool const bLeftResult = m_leftTarget.TryGetTransform( pPose, m_leftTargetTransform );
            EE_ASSERT( bLeftResult );

            // Get the effector index
            if ( m_leftTarget.IsBoneTarget() )
            {
                m_leftTargetBoneIdx = pSkeleton->GetBoneIndex( m_leftTarget.GetBoneID() );
            }

            // Convert to character space
            if ( !m_leftTarget.IsBoneTarget() && m_isTargetInWorldSpace )
            {
                m_leftTargetTransform = context.m_worldTransformInverse * m_leftTargetTransform;
            }

            bool const bRightResult = m_rightTarget.TryGetTransform( pPose, m_rightTargetTransform );
            EE_ASSERT( bRightResult );

            if ( m_rightTarget.IsBoneTarget() )
            {
                m_rightTargetBoneIdx = pSkeleton->GetBoneIndex( m_rightTarget.GetBoneID() );
            }

            if ( !m_rightTarget.IsBoneTarget() && m_isTargetInWorldSpace )
            {
                m_rightTargetTransform = context.m_worldTransformInverse * m_rightTargetTransform;
            }
        }
        else // Running with deserialized data
        {
            // Optimization for bone-targets with no offsets
            if ( m_leftTargetBoneIdx != InvalidIndex )
            {
                m_leftTargetTransform = pPose->GetModelSpaceTransform( m_leftTargetBoneIdx );
            }

            // Optimization for bone-targets with no offsets
            if ( m_rightTargetBoneIdx != InvalidIndex )
            {
                m_rightTargetTransform = pPose->GetModelSpaceTransform( m_rightTargetBoneIdx );
            }
        }

        // Solve
        //-------------------------------------------------------------------------

        TwoBoneSolver::Solve( *pPose, m_leftFootBoneIdx, m_leftTargetTransform, 0.0f, m_blendMode, m_blendWeight );
        TwoBoneSolver::Solve( *pPose, m_rightFootBoneIdx, m_rightTargetTransform, 0.0f, m_blendMode, m_blendWeight );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_debugLeftFootBoneID = pSkeleton->GetBoneID( m_leftFootBoneIdx );
        m_debugRightFootBoneID = pSkeleton->GetBoneID( m_rightFootBoneIdx );
        #endif

        MarkTaskComplete( context );
    }

    void FootIKTask::Serialize( TaskSerializer& serializer ) const
    {
        EE_ASSERT( !m_isRunningFromDeserializedData );

        serializer.WriteBoneIndex( m_leftFootBoneIdx );
        serializer.WriteBoneIndex( m_rightFootBoneIdx );

        // Left Target
        //-------------------------------------------------------------------------

        // Optimization for pure bone targets
        if ( m_leftTarget.IsBoneTarget() && !m_leftTarget.HasOffsets() && m_leftTargetBoneIdx != InvalidIndex )
        {
            serializer.WriteBool( true );
            serializer.WriteBoneIndex( m_leftTargetBoneIdx );
        }
        else
        {
            serializer.WriteBool( false );
            serializer.WriteRotation( m_leftTargetTransform.GetRotation() );
            serializer.WriteQuantizedTranslationForIK( m_leftTargetTransform.GetTranslation(), s_quantizationRangeForTranslation );
        }

        // Right Target
        //-------------------------------------------------------------------------

        // Optimization for pure bone targets
        if ( m_rightTarget.IsBoneTarget() && !m_rightTarget.HasOffsets() && m_rightTargetBoneIdx != InvalidIndex )
        {
            serializer.WriteBool( true );
            serializer.WriteBoneIndex( m_rightTargetBoneIdx );
        }
        else
        {
            serializer.WriteBool( false );
            serializer.WriteRotation( m_rightTargetTransform.GetRotation() );
            serializer.WriteQuantizedTranslationForIK( m_rightTargetTransform.GetTranslation(), s_quantizationRangeForTranslation );
        }

        //-------------------------------------------------------------------------

        bool const hasBlendWeight = m_blendWeight != 1.0f;
        serializer.WriteBool( hasBlendWeight );
        if ( hasBlendWeight )
        {
            serializer.WriteBool( m_blendMode == IKBlendMode::Effector );
            serializer.WriteNormalizedFloat8Bit( m_blendWeight != 1.0f );
        }
    }

    void FootIKTask::Deserialize( TaskSerializer& serializer )
    {
        m_leftTargetBoneIdx = m_rightTargetBoneIdx = InvalidIndex;

        m_leftFootBoneIdx = serializer.ReadBoneIndex();
        m_rightFootBoneIdx = serializer.ReadBoneIndex();

        // Left Target
        //-------------------------------------------------------------------------

        bool bIsBoneTarget = serializer.ReadBool();
        if ( bIsBoneTarget )
        {
            m_leftTargetBoneIdx = serializer.ReadBoneIndex();
        }
        else
        {
            Quaternion const q = serializer.ReadRotation();
            Vector const t = serializer.ReadQuantizedTranslationForIK( s_quantizationRangeForTranslation );
            m_leftTargetTransform = Transform( q, t );
        }

        // Right Target
        //-------------------------------------------------------------------------

        bIsBoneTarget = serializer.ReadBool();
        if ( bIsBoneTarget )
        {
            m_rightTargetBoneIdx = serializer.ReadBoneIndex();
        }
        else
        {
            Quaternion const q = serializer.ReadRotation();
            Vector const t = serializer.ReadQuantizedTranslationForIK( s_quantizationRangeForTranslation );
            m_rightTargetTransform = Transform( q, t );
        }

        //-------------------------------------------------------------------------

        bool const bHasBlendWeight = serializer.ReadBool();
        if ( bHasBlendWeight )
        {
            bool const bIsEffectorBlend = serializer.ReadBool();
            m_blendMode = bIsEffectorBlend ? IKBlendMode::Effector : IKBlendMode::Pose;
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
    InlineString FootIKTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        return InlineString( InlineString::CtorSprintf(), "Effectors: L=%s, R=%s", m_debugLeftFootBoneID.c_str(), m_debugRightFootBoneID.c_str() );
    }

    void FootIKTask::DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        PoseTask::DrawDebug( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );

        if ( isDetailedViewEnabled && !m_isRunningFromDeserializedData )
        {
            Transform tmp = m_leftTargetTransform * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 4.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Left Target", Colors::LimeGreen );

            tmp = m_rightTargetTransform * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 4.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Right Target", Colors::Yellow );
        }
    }
    #endif
}
