#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/IResource.h"
#include "geometry/PxTriangleMesh.h"
#include "geometry/PxConvexMesh.h"
#include "System/Types/StringID.h"
#include "System/Math/BoundingVolumes.h"

//-------------------------------------------------------------------------

namespace physx
{
    class PxTriangleMesh;
    class PxConvexMesh;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class EE_ENGINE_API PhysicsMesh : public Resource::IResource
    {
        EE_REGISTER_RESOURCE( 'pmsh', "Physics Mesh" );
        friend class PhysicsMeshCompiler;
        friend class PhysicsMeshLoader;

        EE_SERIALIZE( m_physicsMaterialIDs, m_isConvexMesh );

    public:

        virtual bool IsValid() const override;

        //-------------------------------------------------------------------------

        inline OBB const& GetBounds() const { return m_bounds; }

        inline bool IsTriangleMesh() const { return !m_isConvexMesh; }
        inline bool IsConvexMesh() const { return m_isConvexMesh; }

        inline uint16_t GetNumMaterialsNeeded() const { return (uint16_t) m_physicsMaterialIDs.size(); }
        inline TInlineVector<StringID, 4> const& GetPhysicsMaterials() const { return m_physicsMaterialIDs; }

        //-------------------------------------------------------------------------

        inline physx::PxTriangleMesh const* GetTriangleMesh() const
        {
            EE_ASSERT( !m_isConvexMesh );
            return m_pMesh->is<physx::PxTriangleMesh>();
        }
        
        inline physx::PxConvexMesh const* GetConvexMesh() const
        {
            EE_ASSERT( m_isConvexMesh );
            return m_pMesh->is<physx::PxConvexMesh>();
        }

    private:

        physx::PxBase*              m_pMesh = nullptr;
        TInlineVector<StringID, 4>  m_physicsMaterialIDs;
        bool                        m_isConvexMesh = false;
        OBB                         m_bounds;
    };
}