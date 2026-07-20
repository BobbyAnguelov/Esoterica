#include "ResourceLoader_PhysicsCollisionMesh.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    CollisionMeshLoader::CollisionMeshLoader()
    {
        m_loadableTypes.push_back( CollisionMesh::GetStaticResourceTypeID() );
    }

    Resource::LoadResult CollisionMeshLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        auto pCollision = EE::New<CollisionMesh>();
        ( *pArchive ) << *pCollision;

        pResourceRecord->SetResourceData( pCollision );
        return Resource::LoadResult::Complete;
    }
}