#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API BoxComponent : public PhysicsShapeComponent
    {
        EE_ENTITY_COMPONENT( BoxComponent );

        friend class PhysicsWorld;

    public:

        EE_FORCE_INLINE Float3 const& GetExtents() const { return m_boxHalfExtents; }

    private:

        virtual OBB CalculateLocalBounds() const override final;
        virtual bool HasValidPhysicsSetup() const override final;

    protected:

        EE_REFLECT( "Category" : "Shape" );
        Float3                              m_boxHalfExtents = Float3( 1.0f );

        EE_REFLECT( "Category" : "Physics" );
        MaterialID                          m_materialID;
    };
}