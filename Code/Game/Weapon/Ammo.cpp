#include "Ammo.h"
//-------------------------------------------------------------------------

#include "Engine/Physics/PhysicsLayers.h"
#include "Engine/Physics/PhysicsScene.h"
#include "System/Types/Color.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Weapon
{
    // define a context for ammo : physics world + delta time + debug stuff
    bool Ammo::Shoot( AmmoContext const& ctx, RangedWeaponInfo weaponDamage, Vector origin, Vector target )
    {
        // adjust accuracy
        // weaponDamage.m_baseAccuracy* m_dmgModifiers.m_accuracyModifier;

        // check if we have a valid target
        Vector const directionToTarget = ( target - origin ).GetNormalized3();
        // check if dir is valid
        Vector const traceEnd = origin + directionToTarget * m_range;
        ctx.m_pPhysicsScene->AcquireReadLock();

        Physics::QueryFilter filter;
        filter.SetLayerMask( Physics::CreateLayerMask( Physics::Layers::Environment ) );

        Physics::RayCastResults outTraceResults;
        bool const bHasValidHit = ctx.m_pPhysicsScene->RayCast( origin, traceEnd, filter, outTraceResults );

        #if EE_DEVELOPMENT_TOOLS
        // debug draw for now
        auto drawingCtx = ctx.m_pEntityWorldUpdateContext->GetDrawingContext();
        EE::Float4 DrawingColor = bHasValidHit ? Colors::HotPink : Colors::Green;
        drawingCtx.DrawLine( origin, traceEnd, DrawingColor, 2.f, EE::Drawing::DisableDepthTest, 2.f );
        #endif

        ctx.m_pPhysicsScene->ReleaseReadLock();

        if ( !bHasValidHit || !outTraceResults.hasBlock )
        {
            return false;
        }

        Vector const HitLocation = origin + directionToTarget * outTraceResults.block.distance;

        #if EE_DEVELOPMENT_TOOLS
        drawingCtx.DrawSphere( HitLocation, EE::Float3( 0.5f ), DrawingColor, 2.f, EE::Drawing::DisableDepthTest, 2.f );
        #endif

        // Calculate damage with ammo mods
        DamageDealtInfo damageToDeal;
        damageToDeal.m_damageType = m_dmgModifiers.m_damageType;
        damageToDeal.m_damageValue = weaponDamage.m_damageValue * m_dmgModifiers.m_damageModifier;
        damageToDeal.m_hitLocation = HitLocation;

        // request damage on hit to damage system with damageToDeal info + actor hit info

        return true;
    }
}