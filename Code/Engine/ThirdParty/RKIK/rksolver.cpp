//--------------------------------------------------------------------------------------------------
// solver.cpp
//
// Copyright(C) by D. Gregorius. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include "rksolver.h"


//--------------------------------------------------------------------------------------------------
// RkIkSolver
//--------------------------------------------------------------------------------------------------
float rkTwistAngle(const RkQuaternion& RelQ)
{
    // Note: We assume the relative quaternion is build from two quaternions in the same hemisphere!
    float Twist = 2.0f * rkATan2(RelQ.Z, RelQ.W);
    RK_ASSERT(-RK_PI <= Twist && Twist <= RK_PI);

    return Twist;
}


//--------------------------------------------------------------------------------------------------
float rkSwingAngle(const RkQuaternion& RelQ)
{
    // Note: We assume the relative quaternion is build from two quaternions in the same hemisphere!
    float Swing = 2.0f * rkATan2(rkSqrt(RelQ.X * RelQ.X + RelQ.Y * RelQ.Y), rkSqrt(RelQ.Z * RelQ.Z + RelQ.W * RelQ.W));
    RK_ASSERT(0.0f <= Swing && Swing <= RK_PI);

    return Swing;
}


//--------------------------------------------------------------------------------------------------
void rkApplyLinearPushAt(const RkVector3& Push, const RkVector3& Offset, const RkIkBody& Body, RkTransform& Transform)
{
    // Update orientation
    RkMatrix3 InvI = Body.GetInertiaInv(Transform);
    RkVector3 Omega = InvI * rkCross(Offset, Push);
    RkQuaternion Orientation = Transform.Rotation;
    Orientation = rkNormalize(Orientation + 0.5f * RkQuaternion(Omega.X, Omega.Y, Omega.Z, 0.0f) * Orientation);

    // Update position
    float InvM = Body.GetMassInv();
    RkVector3 Center = Body.GetCenter(Transform);
    Center = Center + InvM * Push;
    RkVector3 Position = Center - Orientation * Body.CenterOfMass;

    // Save new transform
    Transform = { Position, Orientation };
}


//--------------------------------------------------------------------------------------------------
void rkApplyAngularPush(const RkVector3& Push, const RkIkBody& Body, RkTransform& Transform)
{
    // Update orientation
    RkMatrix3 InvI = Body.GetInertiaInv(Transform);
    RkVector3 Omega = InvI * Push;
    RkQuaternion Orientation = Transform.Rotation;
    Orientation = rkNormalize(Orientation + 0.5f * RkQuaternion(Omega.X, Omega.Y, Omega.Z, 0.0f) * Orientation);

    // Update position
    RkVector3 Center = Body.GetCenter(Transform);
    RkVector3 Position = Center - Orientation * Body.CenterOfMass;

    // Save new transform
    Transform = { Position, Orientation };
}


//--------------------------------------------------------------------------------------------------
void rkSolveTwist(const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    // Body1
    int BodyIndex1 = Joint.ParentBodyIndex;
    const RkIkBody& Body1 = Bodies[BodyIndex1];
    RkTransform Transform1 = BodyTransforms[BodyIndex1];
    RkMatrix3 InvI1 = Body1.GetInertiaInv(Transform1);
    RkQuaternion Basis1 = Joint.GetBasis1(Transform1);

    // Body2
    int BodyIndex2 = Joint.BodyIndex;
    const RkIkBody& Body2 = Bodies[BodyIndex2];
    RkTransform Transform2 = BodyTransforms[BodyIndex2];
    RkMatrix3 InvI2 = Body2.GetInertiaInv(Transform2);
    RkQuaternion Basis2 = Joint.GetBasis2(Transform2);

    // Relative quaternion
    if (rkDot(Basis1, Basis2) < 0.0f)
    {
        Basis2 = -Basis2;
    }
    RkQuaternion RelQ = rkCMul(Basis1, Basis2);

    float Twist = rkTwistAngle(RelQ);
    float MinTwistLimit = Joint.MinTwistLimit;
    float MaxTwistLimit = Joint.MaxTwistLimit;
    if (MinTwistLimit <= Twist && Twist <= MaxTwistLimit)
    {
        return;
    }

    RkVector3 AxisZ1 = Basis1 * RK_VEC3_AXIS_Z;
    RkVector3 AxisZ2 = Basis2 * RK_VEC3_AXIS_Z;
    float TanThetaOver2 = rkSqrt((RelQ.X * RelQ.X + RelQ.Y * RelQ.Y) / (RelQ.Z * RelQ.Z + RelQ.W * RelQ.W));
    RkVector3 Axis = AxisZ1 + TanThetaOver2 * rkCross(rkNormalize(rkCross(AxisZ1, AxisZ2)), AxisZ1);
    float EffectiveMassInv = rkDot(Axis, (InvI1 + InvI2) * Axis);
    float EffectiveMass = EffectiveMassInv * EffectiveMassInv > 1000.0f * RK_F32_MIN ? 1.0f / EffectiveMassInv : 0.0f;

    float C = Twist < MinTwistLimit ? Twist - MinTwistLimit : Twist - MaxTwistLimit;
    float DeltaLambda = Joint.Weight * EffectiveMass * -C;

    RkVector3 Push = Axis * DeltaLambda;
    rkApplyAngularPush(-Push, Body1, Transform1);
    BodyTransforms[BodyIndex1] = Transform1;
    rkApplyAngularPush(Push, Body2, Transform2);
    BodyTransforms[BodyIndex2] = Transform2;
}


//--------------------------------------------------------------------------------------------------
void rkSolveSwing(const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    // Body1
    int BodyIndex1 = Joint.ParentBodyIndex;
    const RkIkBody& Body1 = Bodies[BodyIndex1];
    RkTransform Transform1 = BodyTransforms[BodyIndex1];
    RkMatrix3 InvI1 = Body1.GetInertiaInv(Transform1);
    RkQuaternion Basis1 = Joint.GetBasis1(Transform1);

    // Body2
    int BodyIndex2 = Joint.BodyIndex;
    const RkIkBody& Body2 = Bodies[BodyIndex2];
    RkTransform Transform2 = BodyTransforms[BodyIndex2];
    RkMatrix3 InvI2 = Body2.GetInertiaInv(Transform2);
    RkQuaternion Basis2 = Joint.GetBasis2(Transform2);

    // Relative quaternion
    if (rkDot(Basis1, Basis2) < 0.0f)
    {
        Basis2 = -Basis2;
    }
    RkQuaternion RelQ = rkCMul(Basis1, Basis2);

    // Solve
    float Swing = rkSwingAngle(RelQ);
    float SwingLimit = Joint.SwingLimit;
    if (Swing < SwingLimit)
    {
        return;
    }

    RkVector3 AxisZ1 = Basis1 * RK_VEC3_AXIS_Z;
    RkVector3 AxisZ2 = Basis2 * RK_VEC3_AXIS_Z;
    RkVector3 Axis = rkNormalize(rkCross(AxisZ1, AxisZ2));
    float EffectiveMassInv = rkDot(Axis, (InvI1 + InvI2) * Axis);
    float EffectiveMass = EffectiveMassInv * EffectiveMassInv > 1000.0f * RK_F32_MIN ? 1.0f / EffectiveMassInv : 0.0f;

    float C = Swing - SwingLimit;
    float DeltaLambda = Joint.Weight * EffectiveMass * -C;

    RkVector3 Push = Axis * DeltaLambda;
    rkApplyAngularPush(-Push, Body1, Transform1);
    BodyTransforms[BodyIndex1] = Transform1;
    rkApplyAngularPush(Push, Body2, Transform2);
    BodyTransforms[BodyIndex2] = Transform2;
}


//--------------------------------------------------------------------------------------------------
void rkSolveAngular(const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    // Body1
    int BodyIndex1 = Joint.ParentBodyIndex;
    const RkIkBody& Body1 = Bodies[BodyIndex1];
    RkTransform Transform1 = BodyTransforms[BodyIndex1];
    RkMatrix3 InvI1 = Body1.GetInertiaInv(Transform1);
    RkQuaternion Basis1 = Joint.GetBasis1(Transform1);

    // Body2
    int BodyIndex2 = Joint.BodyIndex;
    const RkIkBody& Body2 = Bodies[BodyIndex2];
    RkTransform Transform2 = BodyTransforms[BodyIndex2];
    RkMatrix3 InvI2 = Body2.GetInertiaInv(Transform2);
    RkQuaternion Basis2 = Joint.GetBasis2(Transform2);

    // Relative quaternion
    if (rkDot(Basis1, Basis2) < 0.0f)
    {
        Basis2 = -Basis2;
    }
    RkQuaternion RelQ = rkCMul(Basis1, Basis2);

    // Solve
    RkMatrix3 InvI = InvI1 + InvI2;
    RkVector3 AxisX = 0.5f * (Basis1 * (RelQ.S * RK_VEC3_AXIS_X + rkCross(RelQ.V, RK_VEC3_AXIS_X)));
    RkVector3 AxisY = 0.5f * (Basis1 * (RelQ.S * RK_VEC3_AXIS_Y + rkCross(RelQ.V, RK_VEC3_AXIS_Y)));

    float A11 = rkDot(AxisX, InvI * AxisX);
    float A21 = rkDot(AxisY, InvI * AxisX);
    float A22 = rkDot(AxisY, InvI * AxisY);
    RkMatrix2 EffectiveMassInv(A11, A21, A21, A22);
    RkMatrix2 EffectiveMass = rkInvert(EffectiveMassInv);

    RkVector2 C(RelQ.X, RelQ.Y);
    RkVector2 DeltaLambda = Joint.Weight * EffectiveMass * -C;

    RkVector3 Push = DeltaLambda.X * AxisX + DeltaLambda.Y * AxisY;
    rkApplyAngularPush(-Push, Body1, Transform1);
    BodyTransforms[BodyIndex1] = Transform1;
    rkApplyAngularPush(Push, Body2, Transform2);
    BodyTransforms[BodyIndex2] = Transform2;
}


//--------------------------------------------------------------------------------------------------
void rkSolveLinear(const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    // Body1
    int BodyIndex1 = Joint.ParentBodyIndex;
    const RkIkBody& Body1 = Bodies[BodyIndex1];
    RkTransform Transform1 = BodyTransforms[BodyIndex1];
    float InvM1 = Body1.GetMassInv();
    RkMatrix3 InvI1 = Body1.GetInertiaInv(Transform1);
    RkVector3 Center1 = Body1.GetCenter(Transform1);
    RkVector3 Origin1 = Joint.GetOrigin1(Transform1);
    RkVector3 Offset1 = Origin1 - Center1;
    RkMatrix3 Skew1 = rkSkew( Offset1 );

    // Body2 
    int BodyIndex2 = Joint.BodyIndex;
    const RkIkBody& Body2 = Bodies[BodyIndex2];
    RkTransform Transform2 = BodyTransforms[BodyIndex2];
    float InvM2 = Body2.GetMassInv();
    RkMatrix3 InvI2 = Body2.GetInertiaInv(Transform2);
    RkVector3 Center2 = Body2.GetCenter(Transform2);
    RkVector3 Origin2 = Joint.GetOrigin2(Transform2);
    RkVector3 Offset2 = Origin2 - Center2;
    RkMatrix3 Skew2 = rkSkew( Offset2 );

    // Solve
    float InvM = InvM1 + InvM2;
    RkMatrix3 EffectiveMassInv = RkMatrix3(InvM, InvM, InvM) - Skew1 * InvI1 * Skew1 - Skew2 * InvI2 * Skew2;
    RkMatrix3 EffectiveMass = rkInvertT(EffectiveMassInv);

    RkVector3 C = Origin2 - Origin1;
    RkVector3 DeltaLambda = Joint.Weight * EffectiveMass * -C;

    RkVector3 Push = DeltaLambda;
    rkApplyLinearPushAt(-Push, Offset1, Body1, Transform1);
    BodyTransforms[BodyIndex1] = Transform1;
    rkApplyLinearPushAt(Push, Offset2, Body2, Transform2);
    BodyTransforms[BodyIndex2] = Transform2;
}


//--------------------------------------------------------------------------------------------------
void rkSolveAngular(const RkIkEffector& Effector, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    RK_ASSERT(Effector.Enabled);

    // Body
    int BodyIndex = Effector.BodyIndex;
    const RkIkBody& Body = Bodies[BodyIndex];
    RkTransform Transform = BodyTransforms[BodyIndex];
    RkMatrix3 InvI = Body.GetInertiaInv(Transform);
    RkQuaternion Basis = Transform.Rotation;

    // Relative quaternion
    RkQuaternion RelQ = rkCMul(Basis, Effector.TargetOrientation);
    if (rkDot(RelQ, RK_QUAT_IDENTITY) < 0.0f)
    {
        RelQ = -RelQ;
    }
    RkQuaternion Omega = 2.0f * (RK_QUAT_IDENTITY - RelQ) * rkConjugate(RelQ);

    // Solve
    RkMatrix3 EffectiveMassInv = InvI;
    RkMatrix3 EffectiveMass = rkInvert(EffectiveMassInv);

    RkVector3 C = Basis * -Omega.V;
    RkVector3 DeltaLambda = Effector.Weight * EffectiveMass * -C;

    RkVector3 Push = DeltaLambda;
    rkApplyAngularPush(-Push, Body, Transform);
    BodyTransforms[BodyIndex] = Transform;
}

//--------------------------------------------------------------------------------------------------
void rkSolveAngular( const RkIkBody& Body, const RkTransform& Transform0, RkTransform& Transform )
{
    RK_ASSERT( Body.Resistance > 0.0f );

    // Body
    RkMatrix3 InvI = Body.GetInertiaInv( Transform );
    RkQuaternion Basis = Transform.Rotation;

    // Relative quaternion
    RkQuaternion RelQ = rkCMul( Basis, Transform0.Rotation );
    if ( rkDot( RelQ, RK_QUAT_IDENTITY ) < 0.0f )
    {
        RelQ = -RelQ;
    }
    RkQuaternion Omega = 2.0f * ( RK_QUAT_IDENTITY - RelQ ) * rkConjugate( RelQ );

    // Solve
    RkMatrix3 EffectiveMassInv = InvI;
    RkMatrix3 EffectiveMass = rkInvert( EffectiveMassInv );

    RkVector3 C = Basis * -Omega.V;
    RkVector3 DeltaLambda = Body.Resistance * EffectiveMass * -C;

    RkVector3 Push = DeltaLambda;
    rkApplyAngularPush( -Push, Body, Transform );
}


//--------------------------------------------------------------------------------------------------
void rkSolveLinear( const RkIkBody& Body, const RkTransform& Transform0, RkTransform& Transform )
{
    RK_ASSERT( Body.Resistance > 0.0f );

    // Body
    float InvM = Body.GetMassInv();
    RkMatrix3 InvI = Body.GetInertiaInv(Transform);
    RkVector3 Center = Body.GetCenter(Transform);
    RkVector3 Origin = Transform.Translation;
    RkVector3 Offset = Origin - Center;

    // Solve
    RkMatrix3 Skew = rkSkew(Offset);
    RkMatrix3 EffectiveMassInv = RkMatrix3(InvM, InvM, InvM) - Skew * InvI * Skew;
    RkMatrix3 EffectiveMass = rkInvertT(EffectiveMassInv);

    RkVector3 C = Transform0.Translation - Origin;
    RkVector3 DeltaLambda = Body.Resistance * EffectiveMass * -C;

    RkVector3 Push = DeltaLambda;
    rkApplyLinearPushAt(-Push, Offset, Body, Transform);
}


//--------------------------------------------------------------------------------------------------
void rkSolveLinear( const RkIkEffector& Effector, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms )
{
    RK_ASSERT( Effector.Enabled );

    // Body
    int BodyIndex = Effector.BodyIndex;
    const RkIkBody& Body = Bodies[BodyIndex];
    RkTransform Transform = BodyTransforms[BodyIndex];
    float InvM = Body.GetMassInv();
    RkMatrix3 InvI = Body.GetInertiaInv( Transform );
    RkVector3 Center = Body.GetCenter( Transform );
    RkVector3 Origin = Transform.Translation;
    RkVector3 Offset = Origin - Center;

    // Solve
    RkMatrix3 Skew = rkSkew( Offset );
    RkMatrix3 EffectiveMassInv = RkMatrix3( InvM, InvM, InvM ) - Skew * InvI * Skew;
    RkMatrix3 EffectiveMass = rkInvertT( EffectiveMassInv );

    RkVector3 C = Effector.TargetPosition - Origin;
    RkVector3 DeltaLambda = Effector.Weight * EffectiveMass * -C;

    RkVector3 Push = DeltaLambda;
    rkApplyLinearPushAt( -Push, Offset, Body, Transform );
    BodyTransforms[BodyIndex] = Transform;
}


//--------------------------------------------------------------------------------------------------
void rkSolveJoint(const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    // Twist
    rkSolveTwist(Joint, Bodies, BodyTransforms);

    if (Joint.SwingLimit > 0.0f)
    {
        // Swing 
        rkSolveSwing(Joint, Bodies, BodyTransforms);
    }
    else
    {
        // Hinge
        rkSolveAngular(Joint, Bodies, BodyTransforms);
    }

    // Point2Point
    rkSolveLinear(Joint, Bodies, BodyTransforms);
}


//--------------------------------------------------------------------------------------------------
void rkSolveEffector(const RkIkEffector& Effector, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms)
{
    rkSolveAngular(Effector, Bodies, BodyTransforms);
    rkSolveLinear(Effector, Bodies, BodyTransforms);
}


//--------------------------------------------------------------------------------------------------
void rkSolveBody( const RkIkBody& Body, const RkTransform& BodyTransform0, RkTransform& BodyTransform )
{
    rkSolveAngular( Body, BodyTransform0, BodyTransform );
    rkSolveLinear( Body, BodyTransform0, BodyTransform );
}


//--------------------------------------------------------------------------------------------------
RkSolver::RkSolver(RkArray< RkIkBody > Bodies, RkArray< RkIkJoint > Joints )
    : mBodies( Bodies)
    , mJoints( Joints )
{
    int BodyCount = mBodies.Size();
    for (int BodyIndex = 0; BodyIndex < BodyCount; ++BodyIndex)
    {
        RK_ASSERT( mBodies[BodyIndex].Mass > 0 );
        RK_ASSERT(mBodies[BodyIndex].Radius.X > 0);
        RK_ASSERT(mBodies[BodyIndex].Radius.Y > 0);
        RK_ASSERT(mBodies[BodyIndex].Radius.Z > 0);
    }

    int JointCount = mJoints.Size();
    for (int JointIndex = 0; JointIndex < JointCount; ++JointIndex)
    {
        RK_ASSERT(mJoints[JointIndex].ParentBodyIndex >= 0 && mJoints[JointIndex].ParentBodyIndex < BodyCount);
        RK_ASSERT(mJoints[JointIndex].BodyIndex >= 0 && mJoints[JointIndex].BodyIndex < BodyCount);

        // TODO: additional validation for limits
    }
}

//--------------------------------------------------------------------------------------------------
void RkSolver::Solve( const RkArray< RkIkEffector >& Effectors, RkArray< RkTransform >& BodyTransforms, int Iterations)
{
    if (mBodies.Empty())
    {
        return;
    }
    RK_ASSERT(BodyTransforms.Size() == mBodies.Size());
    RkArray< RkTransform > BodyTransforms0 = BodyTransforms;

    for (int Iteration = 0; Iteration < Iterations; ++Iteration)
    {
        // Relax effectors
        int EffectorCount = Effectors.Size();
        for (int EffectorIndex = 0; EffectorIndex < EffectorCount; ++EffectorIndex)
        {
            // Get effector target (skip if disabled)
            const RkIkEffector& Effector = Effectors[EffectorIndex];
            RK_ASSERT( 0 <= Effector.BodyIndex && Effector.BodyIndex < mBodies.Size() );

            if (!Effector.Enabled)
            {
                continue;
            }

            rkSolveEffector(Effector, mBodies, BodyTransforms);
        }

        // Relax joints
        int JointCount = mJoints.Size();
        for ( int JointIndex = 0; JointIndex < JointCount; ++JointIndex )
        {
            const RkIkJoint& Joint = mJoints[JointIndex];
            RK_ASSERT( 0 <= Joint.BodyIndex && Joint.BodyIndex < mBodies.Size() );

            rkSolveJoint( Joint, mBodies, BodyTransforms );
        }

        // Relax bodies
        int BodyCount = mBodies.Size();
        for (int BodyIndex = 0; BodyIndex < BodyCount; ++BodyIndex)
        {
            const RkIkBody& Body = mBodies[BodyIndex];
            if ( Body.Resistance == 0.0f )
            {
                continue;
            }

            rkSolveBody( Body, BodyTransforms0[BodyIndex], BodyTransforms[BodyIndex] );
        }
    }
}