//--------------------------------------------------------------------------------------------------
/*
	@file		solver.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once

#include "rkbody.h"
#include "rkeffector.h"
#include "rkjoint.h"
#include "rkarray.h"
#include "rkmath.h"


//--------------------------------------------------------------------------------------------------
// RkIkSolver
//--------------------------------------------------------------------------------------------------
float rkTwistAngle( const RkQuaternion& RelQ );
float rkSwingAngle( const RkQuaternion& RelQ );

void rkApplyLinearPushAt( const RkVector3& Push, const RkVector3& Offset, const RkIkBody& Body, RkTransform& Transform );
void rkApplyAngularPush( const RkVector3& Push, const RkIkBody& Body, RkTransform& Transform );

void rkSolveTwist( const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveSwing( const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveAngular( const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveLinear( const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveAngular( const RkIkEffector& Effector, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveLinear( const RkIkEffector& Effector, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveAngular( const RkIkBody& Body, const RkTransform& Transform0, RkTransform& Transform );
void rkSolveLinear( const RkIkBody& Body, const RkTransform& Transform0, RkTransform& Transform );
void rkSolveJoint( const RkIkJoint& Joint, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveEffector( const RkIkEffector& Effector, const RkArray< RkIkBody >& Bodies, RkArray< RkTransform >& BodyTransforms );
void rkSolveBody( const RkIkBody& Body, const RkTransform& BodyTransform0, RkTransform& BodyTransform );


class RkSolver
{
public:
	RkSolver( RkArray< RkIkBody > Bodies, RkArray< RkIkJoint > Joints);

	int32 GetNumBodies() const { return mBodies.Size(); }
    RkArray< RkIkBody > const& GetBodies() const { return mBodies; }
    RkArray< RkIkJoint > const& GetJoints() const { return mJoints; }

	void Solve( const RkArray< RkIkEffector >& Effectors, RkArray< RkTransform >& BodyTransforms, int Iterations = 4 );

private:

	RkArray< RkIkBody > mBodies;
	RkArray< RkIkJoint > mJoints;
};