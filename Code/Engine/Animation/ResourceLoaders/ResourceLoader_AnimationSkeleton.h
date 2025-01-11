#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class SkeletonLoader final : public Resource::ResourceLoader
    {
    public:

        SkeletonLoader();

    private:

        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
    };
}