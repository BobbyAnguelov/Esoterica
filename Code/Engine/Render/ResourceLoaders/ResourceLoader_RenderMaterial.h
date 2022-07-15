#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class MaterialLoader : public Resource::ResourceLoader
    {
    public:

        MaterialLoader();

    private:

        virtual bool LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
        virtual Resource::InstallResult Install( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const final;
    };
}