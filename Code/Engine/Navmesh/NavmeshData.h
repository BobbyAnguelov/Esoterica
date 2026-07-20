#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/IResource.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class EE_ENGINE_API NavmeshData : public Resource::IResource
    {
        EE_RESOURCE( "navmesh", "Navmesh", Colors::Cyan, 5, false );
        friend class NavmeshBuilder;
        friend class NavmeshLoader;

        EE_SERIALIZE( m_graphImage );

    public:

        static ResourceID GetNavmeshResourceIDForMap( ResourceID const& mapResourceID );
        static FileSystem::Path GetUserGeneratedNavmeshFilePathForMap( ResourceID const& mapResourceID, FileSystem::Path const& sourceDataDirectoryPath );

    public:

        virtual bool IsValid() const override { return !m_graphImage.empty(); }
        inline Blob const& GetGraphImage() const { return m_graphImage; }

    private:

        Blob m_graphImage;
    };
}