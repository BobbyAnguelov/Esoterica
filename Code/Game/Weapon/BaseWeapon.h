#pragma once
#include "DamageInfoTypes.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::Weapon
{
    class Ammo;
    struct AmmoContext;
    class BaseWeapon
    {
    public:

        virtual ~BaseWeapon() = default;

        // recharge ammo?
        // change ammo?
        virtual bool Shoot(AmmoContext const& ctx, Vector origin, Vector target);

    public:

        Ammo* m_pCurrentAmmo = nullptr;
        RangedWeaponInfo m_weaponDamage;

        float m_baseAccuracy    = 1.f;
        float m_firingRate        = 1.f;
        int32_t m_currentAmmoNum    = 1;
        int32_t m_maxAmmoCapacity = 1;
        //reload time
        // ammo capacity
    };
}