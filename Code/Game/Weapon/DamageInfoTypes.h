#pragma once
#include "Base/Math/Vector.h"
//-------------------------------------------------------------------------

namespace EE::Weapon
{
	enum class DamageType : uint8_t
	{
		Normal,
		Silver
	};

	struct DamageDealtInfo
	{
		Vector m_hitLocation	= Vector::Zero;
		float m_damageValue		= 1.f;
		DamageType m_damageType = DamageType::Normal;
	};

	struct RangedWeaponInfo
	{
		float m_baseAccuracy	= 1.f;
		float m_damageValue		= 1.f;
	};

}