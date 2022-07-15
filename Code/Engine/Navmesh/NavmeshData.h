#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/IResource.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class EE_ENGINE_API NavmeshData : public Resource::IResource
    {
        EE_REGISTER_RESOURCE( 'nav', "Navmesh");
        friend class NavmeshGenerator;
        friend class NavmeshLoader;

        EE_SERIALIZE( m_graphImage );

    public:

        virtual bool IsValid() const override { return !m_graphImage.empty(); }
        inline Blob const& GetGraphImage() const { return m_graphImage; }

    private:

        Blob   m_graphImage;
    };
}