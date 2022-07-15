#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API SphereComponent : public PhysicsShapeComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( SphereComponent );

        friend class PhysicsWorldSystem;

    public:

        SphereComponent();

        EE_FORCE_INLINE float GetRadius() const { return m_radius; }

    private:

        virtual bool HasValidPhysicsSetup() const override final;
        virtual TInlineVector<StringID, 4> GetPhysicsMaterialIDs() const override final { return { m_materialID }; }

    protected:

        EE_EXPOSE StringID                                 m_materialID;
        EE_EXPOSE float                                    m_radius = 0.5f;
    };
}