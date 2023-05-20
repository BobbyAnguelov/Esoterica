#pragma once
#include "DamageInfoTypes.h"
#include "System/Math/Vector.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EntityWorldUpdateContext;
    namespace Physics { class PhysicsWorld; }
}

namespace EE::Weapon
{
    // Provides the common set of systems and components needed for ammo
    //-------------------------------------------------------------------------

    struct AmmoContext
    {
        bool IsValid() const {};

        // Forwarding helper functions
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Seconds GetDeltaTime() const { return m_pEntityWorldUpdateContext->GetDeltaTime(); }
        template<typename T> inline T* GetWorldSystem() const { return m_pEntityWorldUpdateContext->GetWorldSystem<T>(); }
        template<typename T> inline T* GetSystem() const { return m_pEntityWorldUpdateContext->GetSystem<T>(); }
        template<typename T> inline T* GetAnimSubGraphController() const { return m_pAnimationController->GetSubGraphController<T>(); }

    public:

        EntityWorldUpdateContext const* m_pEntityWorldUpdateContext = nullptr;
        Physics::PhysicsWorld* m_pPhysicsWorld = nullptr;
    };

    //-------------------------------------------------------------------------

    class Ammo
    {
    public:

        struct DmgModifiers
        {
            DamageType m_damageType		= DamageType::Normal;
            float m_damageModifier		= 1.f;
            float m_accuracyModifier	= 1.f;
        };

    public:

        virtual ~Ammo() = default;
        virtual bool Shoot(AmmoContext const& ctx, RangedWeaponInfo weaponDamage, Vector origin, Vector target);

    private:

        DmgModifiers m_dmgModifiers;
        float m_range = 300.f;
    };
}