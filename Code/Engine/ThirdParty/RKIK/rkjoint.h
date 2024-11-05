//--------------------------------------------------------------------------------------------------
/*
	@file		joint.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once

#include "rkmath.h"


//--------------------------------------------------------------------------------------------------
// RkIkJoint
//--------------------------------------------------------------------------------------------------
struct RkIkJoint
	{
	int ParentBodyIndex;
	RkTransform LocalFrame;
	int BodyIndex;
	
	float SwingLimit;
	float MinTwistLimit;
	float MaxTwistLimit;

    float Weight = 1.0f;

	inline RkVector3 GetOrigin1( const RkTransform& Transform1 ) const
		{
		return Transform1 * LocalFrame.Translation;
		}

	inline RkQuaternion GetBasis1( const RkTransform& Transform1 ) const
		{
		return Transform1.Rotation * LocalFrame.Rotation;
		}

	inline RkVector3 GetOrigin2( const RkTransform& Transform2 ) const
		{
		return Transform2.Translation;
		}

	inline RkQuaternion GetBasis2( const RkTransform& Transform2 ) const
		{
		return Transform2.Rotation;
		}
	};