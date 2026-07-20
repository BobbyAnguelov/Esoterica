#pragma once

#include "EngineTools/Import/ImportedMesh.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Base/Math/Transform.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Types/Function.h"
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }
namespace EE::EntityModel { class EntityCollection; }
namespace EE::Physics { class CollisionMeshComponent; }
namespace EE::Import { struct Source; }

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshBuildData : public Log
    {
    public:

        struct MeshInstance
        {
            Transform                                              m_worldTransform;
            Vector                                                 m_nonUniformScale;
        };

        static ResourceID GetNavmeshResourceIDForMap( ResourceID const& mapResourceID );

    public:

        ~NavmeshBuildData();

        void Clear();
        bool HasData() const;
        bool IsReadyToBuild() const { return m_collisionMeshesLoaded && HasData() && !HasErrors(); }

        void ExtractBuildData( TypeSystem::TypeRegistry const& typeRegistry, EntityModel::EntityCollection const& entityCollection );

        bool LoadCollisionMeshes( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDirectoryPath, TFunction<void( ResourceID const&, Import::Source&, TVector<String>& )> customMeshLoadInfoFunction = nullptr );
        inline bool AreCollisionMeshesLoaded() const { return m_collisionMeshesLoaded; }

    private:

        void ExtractComponentData( NavmeshComponent const* pComponent );
        void ExtractComponentData( Physics::CollisionMeshComponent const* pComponent );

    public:

        NavmeshBuildSettings                                        m_buildSettings;
        THashMap<ResourceID, TVector<MeshInstance>>                 m_collisionMeshInstances;
        THashMap<ResourceID, Import::Mesh*>                         m_importedCollisionMeshes;
        FileSystem::Path                                            m_optionalBuildLogPath; // If this is set then we will output a build log at the specified path
        bool                                                        m_collisionMeshesLoaded = false;
    };
}