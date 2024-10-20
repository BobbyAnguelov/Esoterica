#pragma once

#include "Component_PhysicsShape.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API CollisionMeshComponent final : public PhysicsShapeComponent
    {
        EE_ENTITY_COMPONENT( CollisionMeshComponent );

        friend class PhysicsWorld;

    public:

        using PhysicsShapeComponent::PhysicsShapeComponent;

        inline void SetCollision( ResourceID collisionResourceID )
        {
            EE_ASSERT( IsUnloaded() );
            EE_ASSERT( collisionResourceID.IsValid() );
            m_collisionMesh = TResourcePtr<CollisionMesh>( collisionResourceID );
        }

        inline bool IsOverridingCollisionSettings() const { return m_overrideCollisionSettings; }

        virtual CollisionSettings const& GetCollisionSettings() const override final;
        virtual void SetCollisionSettings( CollisionSettings const& newSettings ) override final;

        #if EE_DEVELOPMENT_TOOLS
        inline ResourceID const& GetCollisionResourceID() { return m_collisionMesh.GetResourceID(); }
        #endif

        //-------------------------------------------------------------------------

        virtual Float3 const& GetLocalScale() const override { return m_localScale; }
        virtual bool SupportsLocalScale() const override { return true; }

    private:

        virtual OBB CalculateLocalBounds() const override;
        virtual bool HasValidPhysicsSetup() const override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

    protected:

        // The collision mesh to load (can be either convex or concave)
        EE_REFLECT( Category = "Body" )
        TResourcePtr<CollisionMesh>                     m_collisionMesh;

        // A local scale that doesnt propagate but that can allow for non-uniform scaling of shapes
        EE_REFLECT()
        Float3                                          m_localScale = Float3::One;

        // Should we override the collision settings coming from the resource?
        EE_REFLECT( Category = "Colliision" )
        bool                                            m_overrideCollisionSettings = false;
    };
}