#pragma once
#include "Base/Math/Transform.h"
#include "Base/Types/Arrays.h"
#include "IK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Pose;

    //-------------------------------------------------------------------------

    struct TwoBoneSolver
    {
        // Note: ChainRotationWeight - this controls how we solve for effector rotations, 0.0f will try to fully rotate the effector, 1.0f will try to solve the rotation by rotating the IK chain
        static void Solve( Pose &pose, int32_t effectorBoneIdx, Transform targetTransform, float chainRotationWeight = 0.0f, IKBlendMode blendMode = IKBlendMode::Effector, float blendWeight = 1.0f );

        // Note: ChainRotationWeight - this controls how we solve for effector rotations, 0.0f will try to fully rotate the effector, 1.0f will try to solve the rotation by rotating the IK chain
        static bool Solve( TArray<Transform, 3> &modelSpaceBoneTransforms, TArray<Transform, 3> const& modelSpaceReferenceTransforms, Transform const &targetTransform, float chainRotationWeight );
    };
}
