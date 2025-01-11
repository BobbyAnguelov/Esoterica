#include "ResourceLoader_Navmesh.h"
#include "Engine/Navmesh/NavmeshData.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshLoader::NavmeshLoader()
    {
        m_loadableTypes.push_back( NavmeshData::GetStaticResourceTypeID() );
    }

    Resource::ResourceLoader::LoadResult NavmeshLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        auto pNavmeshData = EE::New<NavmeshData>();
        archive << *pNavmeshData;
        pResourceRecord->SetResourceData( pNavmeshData );
        return Resource::ResourceLoader::LoadResult::Succeeded;
    }

    void NavmeshLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        ResourceLoader::Unload( resourceID, pResourceRecord );
    }
}