#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshLoader : public Resource::ResourceLoader
    {
    public:

        NavmeshLoader();

    private:

        virtual bool LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override final;
        virtual void UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const override final;
    };
}