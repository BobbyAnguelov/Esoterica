#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API SphereComponent : public PhysicsShapeComponent
    {
        EE_ENTITY_COMPONENT( SphereComponent );

        friend class PhysicsWorld;

    public:

        EE_FORCE_INLINE float GetRadius() const { return m_radius; }

    private:

        virtual OBB CalculateLocalBounds() const override final;
        virtual bool HasValidPhysicsSetup() const override final;

    protected:

        EE_REFLECT( "Category" : "Shape" );
        float                                   m_radius = 0.5f;

        EE_REFLECT( "Category" : "Physics" );
        MaterialID                              m_materialID;
    };
}