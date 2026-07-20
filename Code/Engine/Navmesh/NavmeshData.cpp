#include "NavmeshData.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    ResourceID NavmeshData::GetNavmeshResourceIDForMap( ResourceID const& mapResourceID )
    {
        ResourceID navmeshResourceID = mapResourceID;
        InlineString const subResourceName( InlineString::CtorSprintf(), "navmesh.%s", Navmesh::NavmeshData::GetStaticResourceTypeID().ToString().c_str() );
        navmeshResourceID.SetSubResourceName( subResourceName.c_str() );
        return navmeshResourceID;
    }

    FileSystem::Path NavmeshData::GetUserGeneratedNavmeshFilePathForMap( ResourceID const& mapResourceID, FileSystem::Path const& sourceDataDirectoryPath )
    {
        DataPath navmeshPath = mapResourceID.GetDataPath();

        String mapFilename = navmeshPath.GetFilenameWithoutExtension();
        mapFilename.append_sprintf( "_navmesh.user%s", Navmesh::NavmeshData::GetStaticResourceTypeID().ToString().c_str() );
        navmeshPath.ReplaceFilename( mapFilename );

        return navmeshPath.GetFileSystemPath( sourceDataDirectoryPath );
    }
}