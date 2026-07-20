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

    Resource::LoadResult NavmeshLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        auto pNavmeshData = EE::New<NavmeshData>();
        ( *pArchive ) << *pNavmeshData;
        pResourceRecord->SetResourceData( pNavmeshData );
        return Resource::LoadResult::Complete;
    }
}