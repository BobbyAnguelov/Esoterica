#include "ResourceLoader_Navmesh.h"
#include "Engine/Navmesh/NavmeshData.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshLoader::NavmeshLoader()
    {
        m_loadableTypes.push_back( NavmeshData::GetStaticResourceTypeID() );
    }

    bool NavmeshLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        auto pNavmeshData = EE::New<NavmeshData>();
        archive << *pNavmeshData;
        pResourceRecord->SetResourceData( pNavmeshData );
        return true;
    }

    void NavmeshLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}