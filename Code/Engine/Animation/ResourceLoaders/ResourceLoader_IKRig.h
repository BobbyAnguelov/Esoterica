#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IKRigLoader final : public Resource::ResourceLoader
    {
    public:

        IKRigLoader();

    private:

        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
        virtual Resource::ResourceLoader::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;
    };
}