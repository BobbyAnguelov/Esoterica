//--------------------------------------------------------------------------------------------------
/*
	@file		effector.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once


//--------------------------------------------------------------------------------------------------
// RkIkEffector
//--------------------------------------------------------------------------------------------------
struct RkIkEffector
{
	bool Enabled;
	int BodyIndex;
	RkVector3 TargetPosition;
	RkQuaternion TargetOrientation;
    
    float Weight = 1.0f;
};