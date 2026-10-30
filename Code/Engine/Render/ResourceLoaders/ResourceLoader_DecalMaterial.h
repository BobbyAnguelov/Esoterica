#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderDecalMaterial.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class DecalMaterialLoader final : public Resource::ResourceLoader
    {
    public:

        DecalMaterialLoader();

    private:

        virtual Resource::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const override final;
        virtual Resource::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override final;
    };
}
