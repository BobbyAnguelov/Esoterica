#include "TwoBoneIK.h"
#include "Base/Math/MathUtils.h"
#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void TwoBoneSolver::Solve( Pose &pose, int32_t effectorBoneIdx, Transform targetTransform, float chainRotationWeight, IKBlendMode blendMode, float blendWeight )
    {
        EE_ASSERT( pose.IsPoseSet() );
        Skeleton const *pSkeleton = pose.GetSkeleton();

        // Set up bone indices
        //-------------------------------------------------------------------------

        TArray<int32_t, 3> boneIndices;
        boneIndices[2] = effectorBoneIdx;
        for ( int32_t i = 1; i >= 0; --i )
        {
            boneIndices[i] = pSkeleton->GetParentBoneIndex( boneIndices[i + 1] );
            EE_ASSERT( boneIndices[i] != InvalidIndex );
        }

        int32_t const numBonesInChain = (int32_t) boneIndices.size();
        int32_t const startBoneIdx = boneIndices[0];

        // Get transforms
        //-------------------------------------------------------------------------

        TArray<Transform, 3> modelSpaceTransforms;

        // Get the parent of the start of the chain, this is needed to calculate the local transforms
        int32_t const baseParentIndex = pSkeleton->GetParentBoneIndex( startBoneIdx );
        Transform const baseParentTransform = ( baseParentIndex != InvalidIndex ) ? pose.GetModelSpaceTransform( baseParentIndex ) : Transform::Identity;

        Transform prevTransform = baseParentTransform;
        for ( int32_t i = 0; i < numBonesInChain; i++ )
        {
            Transform const &boneTransform = pose.GetTransform( boneIndices[i] );
            modelSpaceTransforms[i] = boneTransform * prevTransform;
            prevTransform = modelSpaceTransforms[i];
        }

        // Blend effector target
        //-------------------------------------------------------------------------

        bool const hasBlendWeight = blendWeight != 1.0f;
        if ( hasBlendWeight && blendMode == IKBlendMode::Effector )
        {
            targetTransform = Transform::SLerp( modelSpaceTransforms[2], targetTransform, blendWeight );
        }

        // Get reference transforms
        //-------------------------------------------------------------------------

        TArray<Transform, 3> modelSpaceChainReferenceTransforms;
        for ( int32_t i = 0; i < modelSpaceChainReferenceTransforms.size(); ++i )
        {
            modelSpaceChainReferenceTransforms[i] = pSkeleton->GetModelSpaceReferencePose()[boneIndices[i]];
        }

        // Solve
        //-------------------------------------------------------------------------

        Solve( modelSpaceTransforms, modelSpaceChainReferenceTransforms, targetTransform, chainRotationWeight );

        // Blend results back into pose
        //-------------------------------------------------------------------------

        if ( hasBlendWeight && blendMode == IKBlendMode::Pose )
        {
            // Compute relative transforms in reverse order
            for ( int32_t i = numBonesInChain - 1; i > 0; i-- )
            {
                Transform const localTransformPostIK = modelSpaceTransforms[i].GetDeltaFromOther( modelSpaceTransforms[i - 1] );
                pose.SetTransform( boneIndices[i], Transform::SLerp( pose.GetTransform( boneIndices[i] ), localTransformPostIK, blendWeight ) );
            }

            Transform const localTransformPostIK = modelSpaceTransforms[0].GetDeltaFromOther( baseParentTransform );
            pose.SetTransform( startBoneIdx, Transform::SLerp( pose.GetTransform( boneIndices[0] ), localTransformPostIK, blendWeight ) );
        }
        else
        {
            // Compute relative transforms in reverse order
            for ( int32_t i = numBonesInChain - 1; i > 0; i-- )
            {
                pose.SetTransform( boneIndices[i], modelSpaceTransforms[i].GetDeltaFromOther( modelSpaceTransforms[i - 1] ) );
            }

            pose.SetTransform( startBoneIdx, modelSpaceTransforms[0].GetDeltaFromOther( baseParentTransform ) );
        }

        pose.ClearModelSpaceTransforms();
    }

    bool TwoBoneSolver::Solve( TArray<Transform, 3> &modelSpaceBoneTransforms, TArray<Transform, 3> const& modelSpaceReferenceTransforms, Transform const &targetTransform, float chainRotationWeight )
    {
        Vector const v1 = modelSpaceBoneTransforms[1].GetTranslation() - modelSpaceBoneTransforms[0].GetTranslation();
        Vector const v2 = modelSpaceBoneTransforms[2].GetTranslation() - modelSpaceBoneTransforms[1].GetTranslation();
        Vector const vH = modelSpaceBoneTransforms[2].GetTranslation() - modelSpaceBoneTransforms[0].GetTranslation();
        Vector const vHDesired = targetTransform.GetTranslation() - modelSpaceBoneTransforms[0].GetTranslation();

        float const length1 = v1.GetLength3();
        float const length2 = v2.GetLength3();
        float const lenHDesired = vHDesired.GetLength3();

        if ( Math::IsNearZero( length1 ) || Math::IsNearZero( length2, 0.0f ) )
        {
            return false;
        }

        Vector const v1Norm = v1 / length1;
        Vector const v2Norm = v2 / length2;

        // Calculate the current bend in the mid joint
        const float cosTheta = Math::Clamp( -v1Norm.GetDot3( v2Norm ), -1.0f, 1.0f );
        const float theta = Math::ACos( cosTheta );

        // Calculate the desired bend in he mid joint that will lead to the required distance between the end effector and the root joint of the chain
        const float cosThetaDesired = Math::Clamp( ( Math::Sqr( length1 ) + Math::Sqr( length2 ) - Math::Sqr( lenHDesired ) ) / ( 2.0f * length1 * length2 ), -1.0f, 1.0f );
        const float thetaDesired = Math::ACos( cosThetaDesired );

        // How much we need to rotate in the mid joint
        Radians const thetaDelta = thetaDesired - theta;

        Vector vHingeAxisLS = Vector::Zero;
        Vector vHingeAxisMS = Vector::Zero;
        if ( Math::IsNearEqual( cosTheta, -1.0f, 1e-5f ) )
        {
            // The points of the chain are collinear. We need to use the reference pose to calculate the hinge axis to rotate around in the mid joint
            // This obviously requires the ref pose to have a slight bend
            Vector const v1Ref = modelSpaceReferenceTransforms[1].GetTranslation() - modelSpaceReferenceTransforms[0].GetTranslation();
            Vector const v2Ref = modelSpaceReferenceTransforms[2].GetTranslation() - modelSpaceReferenceTransforms[1].GetTranslation();
            Vector const vCross = v2Ref.Cross3( v1Ref );

            if ( vCross.GetLengthSquared3() > 1e-6f )
            {
                vHingeAxisLS = modelSpaceReferenceTransforms[1].InverseRotateVector( vCross ).GetNormalized3();
            }
            else // Even the ref pose is collinear, so let's just use a specific hinge axis. Might make this configurable
            {
                vHingeAxisLS = Vector::WorldLeft;
            }

            vHingeAxisMS = modelSpaceBoneTransforms[1].RotateVector( vHingeAxisLS );
        }
        else
        {
            vHingeAxisMS = v2Norm.Cross3( v1Norm ).GetNormalized3();
            vHingeAxisLS = modelSpaceBoneTransforms[1].InverseRotateVector( vHingeAxisMS );
        }

        Transform parentSpaceboneTransforms[3];
        parentSpaceboneTransforms[2] = modelSpaceBoneTransforms[2] * modelSpaceBoneTransforms[1].GetInverse();

        // Rotate the mid joint such that the distance between the end effector and the root is the required one to solve the chain
        modelSpaceBoneTransforms[1].SetRotation( modelSpaceBoneTransforms[1].GetRotation() * Quaternion( vHingeAxisMS, thetaDelta ) );
        modelSpaceBoneTransforms[2] = parentSpaceboneTransforms[2] * modelSpaceBoneTransforms[1];

        parentSpaceboneTransforms[1] = modelSpaceBoneTransforms[1] * modelSpaceBoneTransforms[0].GetInverse();

        // Rotate the root so that the end effector aligns with the target transform as much as possible
        modelSpaceBoneTransforms[0].SetRotation( modelSpaceBoneTransforms[0].GetRotation() * Quaternion::FromRotationBetweenVectors( modelSpaceBoneTransforms[2].GetTranslation() - modelSpaceBoneTransforms[0].GetTranslation(), vHDesired ) );
        modelSpaceBoneTransforms[1] = parentSpaceboneTransforms[1] * modelSpaceBoneTransforms[0];
        modelSpaceBoneTransforms[2] = parentSpaceboneTransforms[2] * modelSpaceBoneTransforms[1];
        modelSpaceBoneTransforms[2].SetRotation( targetTransform.GetRotation() );

        // If we don't want to blend in the twist from the reference pose we are done
        if ( chainRotationWeight <= 0.0f )
        {
            return true;
        }

        // Recalculate the model space hinge axis now that we have solved the chain
        vHingeAxisMS = modelSpaceBoneTransforms[1].RotateVector( vHingeAxisLS );

        // Calculate the difference between the current hinge axis ( bend plane ) and the one that we would need to have to match the ref pose as much as possible
        Vector const vRefHingeAxisMS = modelSpaceBoneTransforms[2].RotateVector( modelSpaceReferenceTransforms[2].InverseRotateVector( modelSpaceReferenceTransforms[1].RotateVector( vHingeAxisLS ) ) ).GetNormalized3();

        // We can rotate the root joint around this axis without changing the end effector's position
        Vector const vTwistAxis = ( modelSpaceBoneTransforms[0].GetTranslation() - modelSpaceBoneTransforms[2].GetTranslation() ).GetNormalized3();
        Radians const twistDelta = Math::CalculateAngleBetweenVectorsAroundAnAxis( vRefHingeAxisMS, vHingeAxisMS, vTwistAxis );

        modelSpaceBoneTransforms[0].SetRotation( modelSpaceBoneTransforms[0].GetRotation() * Quaternion::SLerp( Quaternion::Identity, Quaternion( vTwistAxis, -twistDelta ), chainRotationWeight ) );
        modelSpaceBoneTransforms[1] = parentSpaceboneTransforms[1] * modelSpaceBoneTransforms[0];
        modelSpaceBoneTransforms[2] = parentSpaceboneTransforms[2] * modelSpaceBoneTransforms[1];
        modelSpaceBoneTransforms[2].SetRotation( targetTransform.GetRotation() );

        return true;
    }
}