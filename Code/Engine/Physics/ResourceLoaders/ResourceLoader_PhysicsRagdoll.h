#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class RagdollLoader final : public Resource::ResourceLoader
    {
    public:

        RagdollLoader();

    private:

        virtual bool Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        Resource::InstallResult Install( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;
    };
}