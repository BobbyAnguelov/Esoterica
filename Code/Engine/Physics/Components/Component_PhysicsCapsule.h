#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"
#include "Engine/Physics/PhysicsMaterial.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API CapsuleComponent : public PhysicsShapeComponent
    {
        EE_ENTITY_COMPONENT( CapsuleComponent );

        friend class PhysicsWorld;

    public:

        inline CapsuleComponent() = default;
        inline CapsuleComponent( StringID name ) : PhysicsShapeComponent( name ) {}

        //-------------------------------------------------------------------------

        // The capsule radius
        EE_FORCE_INLINE float GetCapsuleRadius() const { return m_defaultRadius; }

        // The half-height of the cylinder portion of the capsule
        EE_FORCE_INLINE float GetCapsuleHalfHeight() const { return m_defaultHalfHeight; }

        // Get the world space top point of the capsule
        EE_FORCE_INLINE Vector GetCapsuleTop() const
        {
            Transform const worldTransform = GetWorldTransform();
            return worldTransform.GetTranslation() + worldTransform.RotateVector( Vector( 0, 0, m_defaultRadius + m_defaultHalfHeight ) );
        }

        // Get the world space bottom point of the capsule
        EE_FORCE_INLINE Vector GetCapsuleBottom() const
        {
            Transform const worldTransform = GetWorldTransform();
            return worldTransform.GetTranslation() - worldTransform.RotateVector( Vector( 0, 0, m_defaultRadius + m_defaultHalfHeight ) );
        }

        // Resize the capsule with the new dimensions (radius > 0 and half-height >= 0)
        // By default, the resized capsule will keep its floor/foot position, if you want to leave the transform untouched, set 'keepFloorPosition' to false
        void ResizeCapsule( float newRadius, float newHalfHeight, bool keepFloorPosition = true );

        // Resize only the capsule's height with the new dimensions (half-height >= 0)
        // By default, the resized capsule will keep its floor/foot position, if you want to leave the transform untouched, set 'keepFloorPosition' to false
        EE_FORCE_INLINE void ResizeCapsuleHeight( float newHalfHeight, bool keepFloorPosition = true ) { ResizeCapsule( m_radius, newHalfHeight, keepFloorPosition ); }

        // Reset the capsule to the serialized settings
        EE_FORCE_INLINE void ResetCapsuleSize( bool keepFloorPosition = true ) { ResizeCapsule( m_defaultRadius, m_defaultHalfHeight, keepFloorPosition ); }

    protected:

        virtual void Initialize() override;
        virtual OBB CalculateLocalBounds() const override final;
        virtual bool HasValidPhysicsSetup() const override final;
        virtual void CreatePhysicsShape() override final;

    protected:

        EE_REFLECT( Category = "Shape" );
        float                                   m_defaultRadius = 0.5f;

        EE_REFLECT( Category = "Shape" );
        float                                   m_defaultHalfHeight = 1.0f;

        EE_REFLECT( Category = "Shape" );
        MaterialID                              m_materialID;

        float                                   m_radius = m_defaultRadius;
        float                                   m_halfHeight = m_defaultHalfHeight;
    };
}