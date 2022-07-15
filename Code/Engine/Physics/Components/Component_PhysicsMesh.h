#pragma once

#include "Engine/_Module/API.h"
#include "Component_PhysicsShape.h"
#include "Engine/Physics/PhysicsMesh.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsMesh;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsMeshComponent final : public PhysicsShapeComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( PhysicsMeshComponent );

        friend class PhysicsWorldSystem;

    public:

        using PhysicsShapeComponent::PhysicsShapeComponent;

        inline void SetMesh( ResourceID meshResourceID )
        {
            EE_ASSERT( IsUnloaded() );
            EE_ASSERT( meshResourceID.IsValid() );
            m_pPhysicsMesh = meshResourceID;
        }

    private:

        virtual void Initialize() override;
        virtual bool HasValidPhysicsSetup() const override final;
        virtual TInlineVector<StringID, 4> GetPhysicsMaterialIDs() const override final;

    protected:

        // Optional: Allows the user to override physical materials needed for a triangle mesh. Invalid stringIDs will keep the material defined in the mesh
        EE_EXPOSE TVector<StringID>                            m_materialOverrideIDs;

        // The collision mesh to load (can be either convex or concave)
        EE_EXPOSE TResourcePtr<PhysicsMesh>                    m_pPhysicsMesh;
    };
}