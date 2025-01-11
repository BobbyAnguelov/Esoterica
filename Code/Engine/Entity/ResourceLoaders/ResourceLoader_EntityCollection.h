#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityCollectionLoader : public Resource::ResourceLoader
    {
    public:

        EntityCollectionLoader();
        ~EntityCollectionLoader() { EE_ASSERT( m_pTypeRegistry == nullptr ); }

        void SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry );
        inline void ClearTypeRegistryPtr() { m_pTypeRegistry = nullptr; }

    private:

        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
        virtual bool CanProceedWithFailedInstallDependency() const override { return true; }

    private:

        TypeSystem::TypeRegistry const* m_pTypeRegistry;
    };
}