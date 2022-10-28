#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API BoxComponent : public PhysicsShapeComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( BoxComponent );

        friend class PhysicsWorldSystem;

    public:

        BoxComponent();

        EE_FORCE_INLINE Float3 const& GetExtents() const { return m_boxExtents; }

    private:

        virtual OBB CalculateLocalBounds() const override final;
        virtual bool HasValidPhysicsSetup() const override final;
        virtual TInlineVector<StringID, 4> GetPhysicsMaterialIDs() const override final { return { m_materialID }; }

    protected:

        EE_EXPOSE StringID                                 m_materialID;
        EE_EXPOSE Float3                                   m_boxExtents = Float3( 1.0f );
    };
}