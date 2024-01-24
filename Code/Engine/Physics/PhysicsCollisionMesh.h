#pragma once

#include "PhysicsCollision.h"
#include "PhysicsMaterial.h"
#include "Base/Resource/IResource.h"
#include "Base/Math/BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace physx
{
    class PxBase;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API CollisionMesh : public Resource::IResource
    {
        EE_RESOURCE( 'pmsh', "Physics Collision Mesh" );
        friend class CollisionMeshCompiler;
        friend class CollisionMeshLoader;

        EE_SERIALIZE( m_materialIDs, m_isConvexMesh, m_collisionSettings );

    public:

        virtual bool IsValid() const override;

        //-------------------------------------------------------------------------

        inline OBB const& GetBounds() const { return m_bounds; }

        inline bool IsTriangleMesh() const { return !m_isConvexMesh; }
        inline bool IsConvexMesh() const { return m_isConvexMesh; }

        inline uint16_t GetNumMaterialsNeeded() const { return (uint16_t) m_materialIDs.size(); }
        inline TInlineVector<MaterialID, 4> const& GetPhysicsMaterials() const { return m_materialIDs; }

        inline CollisionSettings const& GetCollisionSettings() const { return m_collisionSettings; }

        //-------------------------------------------------------------------------

        physx::PxBase* GetMesh() const { return m_pMesh; }

    private:

        physx::PxBase*                  m_pMesh = nullptr;
        TInlineVector<MaterialID,4>     m_materialIDs;
        CollisionSettings               m_collisionSettings;
        OBB                             m_bounds;
        bool                            m_isConvexMesh = false;
    };
}