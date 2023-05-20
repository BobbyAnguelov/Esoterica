#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API CapsuleComponent : public PhysicsShapeComponent
    {
        EE_ENTITY_COMPONENT( CapsuleComponent );

        friend class PhysicsWorld;

    public:

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
        virtual Transform ConvertTransformToPhysics( Transform const& esotericaTransform ) const override final;

    protected:

        EE_REFLECT( "Category" : "Shape" );
        float                                   m_radius = 0.5f;

        EE_REFLECT( "Category" : "Shape" );
        float                                   m_cylinderPortionHalfHeight = 1.0f;

        EE_REFLECT( "Category" : "Physics" );
        MaterialID                              m_materialID;
    };
}