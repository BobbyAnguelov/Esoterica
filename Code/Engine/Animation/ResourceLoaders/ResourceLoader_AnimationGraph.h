#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphLoader final : public Resource::ResourceLoader
    {
    public:

        GraphLoader();
        ~GraphLoader() { EE_ASSERT( m_pTypeRegistry == nullptr ); }

        inline void SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry ) { EE_ASSERT( pTypeRegistry != nullptr ); m_pTypeRegistry = pTypeRegistry; }
        inline void ClearTypeRegistryPtr() { m_pTypeRegistry = nullptr; }

    private:

        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        virtual void Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

        virtual Resource::ResourceLoader::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        TypeSystem::TypeRegistry const* m_pTypeRegistry = nullptr;
    };
}