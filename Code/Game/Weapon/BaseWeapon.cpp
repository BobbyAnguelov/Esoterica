#include "BaseWeapon.h"
//-------------------------------------------------------------------------
#include "Ammo.h"
//-------------------------------------------------------------------------

using namespace EE::Weapon;
// add an assign ammo function?

bool BaseWeapon::Shoot(AmmoContext const& ctx, Vector origin, Vector target)
{
	// check num, decrement stuff
	// calculate damage
	if (m_pCurrentAmmo == nullptr) // change for a check
	{
		return false;
	}

	if (m_currentAmmoNum < 1) // maybe a out reason why it failed?
	{
		return false;
	}

	m_currentAmmoNum--;
	m_pCurrentAmmo->Shoot(ctx, m_weaponDamage, origin, target);
	return true;
}
