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
            m_physicsMesh = meshResourceID;
        }

        #if EE_DEVELOPMENT_TOOLS
        inline ResourceID const& GetMeshResourceID() { return m_physicsMesh.GetResourceID(); }
        #endif

        // Local Scale
        //-------------------------------------------------------------------------

        // Get the local scaling multiplier
        inline Float3 const& GetLocalScale() const { return m_localScale; }

        // Do we have a local scale set?
        inline bool HasLocalScale() const { return m_localScale != Float3::One; }

    private:

        virtual void Initialize() override;
        virtual bool HasValidPhysicsSetup() const override final;
        virtual TInlineVector<StringID, 4> GetPhysicsMaterialIDs() const override final;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

    protected:

        // Optional: Allows the user to override physical materials needed for a triangle mesh. Invalid stringIDs will keep the material defined in the mesh
        EE_EXPOSE TVector<StringID>                         m_materialOverrideIDs;

        // The collision mesh to load (can be either convex or concave)
        EE_EXPOSE TResourcePtr<PhysicsMesh>                 m_physicsMesh;

        // A local scale that doesnt propagate but that can allow for non-uniform scaling of shapes
        EE_EXPOSE Float3                                    m_localScale = Float3::One;
    };
}