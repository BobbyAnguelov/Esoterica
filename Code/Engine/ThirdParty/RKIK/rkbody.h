//--------------------------------------------------------------------------------------------------
/*
	@file		body.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once

#include "rkmath.h"


//--------------------------------------------------------------------------------------------------
// RkIkBody
//--------------------------------------------------------------------------------------------------
struct RkIkBody
	{
	float Mass;
	RkVector3 CenterOfMass;
	RkVector3 Radius;

    float Resistance = 0.0f;

	inline float GetMassInv() const
		{
		return Mass > 0.0f ? 1.0f / Mass : 0.0f;
		}

	inline RkMatrix3 GetInertiaInv( const RkTransform& Transform ) const
		{
		if ( Mass > 0.0f )
			{
			float Ixx = 0.2f * Mass * ( Radius.Y * Radius.Y + Radius.Z * Radius.Z );
			float Iyy = 0.2f * Mass * ( Radius.X * Radius.X + Radius.Z * Radius.Z );
			float Izz = 0.2f * Mass * ( Radius.X * Radius.X + Radius.Y * Radius.Y );
			
			RkMatrix3 R( Transform.Rotation );
			return R * RkMatrix3( 1.0f / Ixx, 1.0f / Iyy, 1.0f / Izz ) * rkTranspose( R );
			}
		else
			{
			return RK_MAT3_ZERO;
			}
		}

	inline RkVector3 GetCenter( const RkTransform& Transform ) const
		{
		return Transform * CenterOfMass;
		}
	};