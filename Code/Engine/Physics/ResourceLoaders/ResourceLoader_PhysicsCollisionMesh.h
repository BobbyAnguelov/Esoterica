#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsSystem;

    //-------------------------------------------------------------------------

    class CollisionMeshLoader : public Resource::ResourceLoader
    {
    public:

        CollisionMeshLoader();

    private:

        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override final;
        virtual void Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override final;
    };
}