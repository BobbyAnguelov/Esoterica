#include "Animation_Task_TwoBoneIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/Plane.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    TwoBoneIKTask::TwoBoneIKTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, float allowedStretchPercentage, bool isTargetInWorldSpace, Target const& effectorTarget )
        : Task( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_effectorBoneIdx( effectorBoneIdx )
        , m_allowedStretchPercentage( allowedStretchPercentage )
        , m_effectorTarget( effectorTarget )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_effectorBoneIdx != InvalidIndex );
        EE_ASSERT( m_allowedStretchPercentage >= 0.0f && m_allowedStretchPercentage <= 0.1f );
        EE_ASSERT( m_effectorTarget.IsTargetSet() );
    }

    void TwoBoneIKTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pPose = pSourceBuffer->GetPrimaryPose();
        auto pSkeleton = pPose->GetSkeleton();
        EE_ASSERT( m_effectorBoneIdx < pSkeleton->GetNumBones() );

        #if EE_DEVELOPMENT_TOOLS
        m_debugEffectorBoneID = pSkeleton->GetBoneID( m_effectorBoneIdx );
        EE_ASSERT( m_debugEffectorBoneID.IsValid() );
        #endif

        // Get bone indices
        //-------------------------------------------------------------------------

        int32_t const chainEndBoneIdx = m_effectorBoneIdx;
        int32_t const chainMidBoneIdx = pSkeleton->GetParentBoneIndex( chainEndBoneIdx );
        int32_t const chainStartBoneIdx = pSkeleton->GetParentBoneIndex( chainMidBoneIdx );

        float const lengthStartToMid = pPose->GetTransform( chainMidBoneIdx ).GetTranslation().GetLength3();
        float const lengthMidToEnd = pPose->GetTransform( chainEndBoneIdx ).GetTranslation().GetLength3();
        float totalChainLength = lengthMidToEnd + lengthStartToMid;
        float actualStretchPercentageNeeded = 0.0f;

        // Calculate actual chain target
        //-------------------------------------------------------------------------

        pPose->CalculateModelSpaceTransforms();

        if ( !m_isRunningFromDeserializedData )
        {
            bool const result = m_effectorTarget.TryGetTransform( pPose, m_effectorTargetTransformCS );
            EE_ASSERT( result );

            // Convert to character space
            if ( !m_effectorTarget.IsBoneTarget() && m_isTargetInWorldSpace )
            {
                m_effectorTargetTransformCS = m_effectorTargetTransformCS * context.m_worldTransformInverse;
            }

            // Calculate final allowed chain length
            if ( m_allowedStretchPercentage > 0.0f )
            {
                totalChainLength += totalChainLength * m_allowedStretchPercentage;
            }

            Transform const chainStartTransform = pPose->GetModelSpaceTransform( chainStartBoneIdx );

            #if EE_DEVELOPMENT_TOOLS
            m_debugRequestedTargetTransformCS = m_effectorTargetTransformCS;
            m_debugChainStartTransformCS = chainStartTransform;
            m_debugTotalChainLength = totalChainLength;
            #endif

            Vector dir;
            float length;
            ( m_effectorTargetTransformCS.GetTranslation() - chainStartTransform.GetTranslation() ).ToDirectionAndLength3( dir, length );
            if ( length > totalChainLength )
            {
                Vector const clampedEffectorTargetPos = chainStartTransform.GetTranslation() + ( dir * totalChainLength );
                m_effectorTargetTransformCS.SetTranslation( clampedEffectorTargetPos );
                actualStretchPercentageNeeded = m_allowedStretchPercentage;
            }
            else
            {
                actualStretchPercentageNeeded = Math::Max( 0.0f, ( length / ( lengthMidToEnd + lengthStartToMid ) ) - 1.0f );
            }
        }
        else
        {
            float const length = ( m_effectorTargetTransformCS.GetTranslation() - pPose->GetModelSpaceTransform( chainStartBoneIdx ).GetTranslation() ).GetLength3();
            actualStretchPercentageNeeded = Math::Max( 0.0f, ( length / ( lengthMidToEnd + lengthStartToMid ) ) - 1.0f );
        }

        // Perform IK
        //-------------------------------------------------------------------------

        Transform startCS = pPose->GetModelSpaceTransform( chainStartBoneIdx );
        Transform endCS = pPose->GetModelSpaceTransform( chainEndBoneIdx );

        // Rotate original transforms to align the original direction vector (start->end) to the desired direction (start->target)
        Vector startToEndDir = ( endCS.GetTranslation() - startCS.GetTranslation() ).GetNormalized3();
        Vector startToTargetDir;
        float startToTargetLength;
        ( m_effectorTargetTransformCS.GetTranslation() - startCS.GetTranslation() ).ToDirectionAndLength3( startToTargetDir, startToTargetLength );
        Quaternion deltaOrientation = Quaternion::FromRotationBetweenVectors( startToEndDir, startToTargetDir );

        startCS.SetRotation( startCS.GetRotation() * deltaOrientation );
        Transform midCS = pPose->GetTransform( chainMidBoneIdx ) * startCS;
        endCS = pPose->GetTransform( chainEndBoneIdx ) * midCS;

        // Calculate a plane in which to perform the analytical solve
        Vector midToEndDir = ( endCS.GetTranslation() - midCS.GetTranslation() ).GetNormalized3();
        Vector startToMidDir = ( midCS.GetTranslation() - startCS.GetTranslation() ).GetNormalized3();
        Vector planeNormal = startToMidDir.Cross3( midToEndDir ).GetNormalized3();

        // Calculate required angle using cosine rule
        float const a = lengthMidToEnd + ( lengthMidToEnd * actualStretchPercentageNeeded );
        float const b = startToTargetLength;
        float const c = lengthStartToMid + ( lengthStartToMid * actualStretchPercentageNeeded );

        Radians const requiredAngle = Math::ACos( ( Math::Sqr( a ) + Math::Sqr( b ) - Math::Sqr( c ) ) / ( 2 * a * b ) );

        Quaternion qRot( AxisAngle( planeNormal, requiredAngle ) );
        Vector offset = qRot.RotateVector( startToTargetDir.GetNegated() ) * a;
        //midCS.SetTranslation( offset + m_effectorTargetTransformCS.GetTranslation() );

        // Convert back into parent space
        //-------------------------------------------------------------------------

        pPose->SetTransform( chainEndBoneIdx, endCS * midCS.GetInverse() );
        pPose->SetTransform( chainMidBoneIdx, midCS * startCS.GetInverse() );

        int32_t const chainStartParentBoneIdx = pSkeleton->GetParentBoneIndex( chainStartBoneIdx );
        if ( chainStartParentBoneIdx != InvalidIndex )
        {
            pPose->SetTransform( chainStartBoneIdx, startCS * pPose->GetModelSpaceTransform( chainStartParentBoneIdx ).GetInverse() );
        }
        else
        {
            pPose->SetRotation( chainStartBoneIdx, pPose->GetTransform( chainStartBoneIdx ).GetRotation() * deltaOrientation );
        }

        //-------------------------------------------------------------------------

        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString TwoBoneIKTask::GetDebugTextInfo() const
    {
        return m_debugEffectorBoneID.c_str();
    }

    void TwoBoneIKTask::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        Task::DrawDebug( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );

        if ( isDetailedViewEnabled && !m_isRunningFromDeserializedData )
        {
            Transform tmp = m_effectorTargetTransformCS * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 4.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Actual Target", Colors::LimeGreen );

            tmp = m_debugRequestedTargetTransformCS * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 3.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Requested Target", Colors::Yellow );

            tmp = m_debugChainStartTransformCS * worldTransform;
            drawingContext.DrawSphere( tmp, m_debugTotalChainLength, Colors::Green );
;       }
    }
    #endif

    void TwoBoneIKTask::Serialize( TaskSerializer& serializer ) const
    {
        EE_ASSERT( m_isRunningFromDeserializedData );

        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteBoneIndex( m_effectorBoneIdx );
        serializer.WriteRotation( m_effectorTargetTransformCS.GetRotation() );
        serializer.WriteTranslation( m_effectorTargetTransformCS.GetTranslation() );
    }

    void TwoBoneIKTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 1 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_effectorBoneIdx = serializer.ReadBoneIndex();

        Quaternion const q = serializer.ReadRotation();
        Float3 const t = serializer.ReadTranslation();
        m_effectorTargetTransformCS = Transform( q, t );

        m_isRunningFromDeserializedData = true;
    }
}