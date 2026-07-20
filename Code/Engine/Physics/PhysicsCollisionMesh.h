#pragma once

#include "PhysicsMaterial.h"
#include "Base/Resource/IResource.h"
#include "Base/Math/BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API CollisionMesh : public Resource::IResource
    {
        EE_RESOURCE( "physmesh", "Physics Collision Mesh", Colors::LightBlue, 19, false );
        friend class CollisionMeshCompiler;
        friend class CollisionMeshLoader;
        friend class PhysicsWorld;

        EE_SERIALIZE( m_materialIDs, m_collisionSettings, m_bounds, m_shapeData, m_isConvexHull );

    public:

        virtual bool IsValid() const override;

        inline bool IsTriangleMesh() const { return !m_isConvexHull; }
        inline bool IsConvexHull() const { return m_isConvexHull; }

        inline AABB const& GetBounds() const { return m_bounds; }
        inline uint16_t GetNumMaterialsNeeded() const { return (uint16_t) m_materialIDs.size(); }
        inline TInlineVector<MaterialID, 4> const& GetPhysicsMaterials() const { return m_materialIDs; }
        inline CollisionSettings const& GetCollisionSettings() const { return m_collisionSettings; }

        b3MeshData const* GetMeshData() const { EE_ASSERT( IsTriangleMesh() ); return (b3MeshData const*) m_shapeData.data(); }
        b3HullData const* GetHullData() const { EE_ASSERT( IsConvexHull() ); return (b3HullData const*) m_shapeData.data(); }

    private:

        TInlineVector<MaterialID,4>     m_materialIDs;
        CollisionSettings               m_collisionSettings;
        AABB                            m_bounds;
        Blob                            m_shapeData;
        bool                            m_isConvexHull = false;
    };
}