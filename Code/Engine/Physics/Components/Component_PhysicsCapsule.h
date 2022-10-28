#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API CapsuleComponent : public PhysicsShapeComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( CapsuleComponent );

        friend class PhysicsWorldSystem;

    public:

        CapsuleComponent();

        // The capsule radius
        EE_FORCE_INLINE float GetRadius() const { return m_radius; }

        // The half-height of the cylinder portion of the capsule
        EE_FORCE_INLINE float GetCylinderPortionHalfHeight() const { return m_cylinderPortionHalfHeight; }

        // Get the full height of the capsule
        EE_FORCE_INLINE float GetCapsuleHeight() const { return ( m_cylinderPortionHalfHeight + m_radius ) * 2; }

        // Get the half-height of the capsule
        EE_FORCE_INLINE float GetCapsuleHalfHeight() const { return ( m_cylinderPortionHalfHeight + m_radius ); }

    private:

        virtual OBB CalculateLocalBounds() const override final;
        virtual bool HasValidPhysicsSetup() const override final;
        virtual TInlineVector<StringID, 4> GetPhysicsMaterialIDs() const override final { return { m_materialID }; }

    protected:

        EE_EXPOSE StringID                                 m_materialID;
        EE_EXPOSE float                                    m_radius = 0.5f;
        EE_EXPOSE float                                    m_cylinderPortionHalfHeight = 1.0f;
    };
}