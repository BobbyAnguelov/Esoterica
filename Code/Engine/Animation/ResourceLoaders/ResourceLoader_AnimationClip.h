#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClipLoader final : public Resource::ResourceLoader
    {
    public:

        AnimationClipLoader();
        ~AnimationClipLoader() { EE_ASSERT( m_pTypeRegistry == nullptr ); }
        void SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry );
        void ClearTypeRegistryPtr() { m_pTypeRegistry = nullptr; }

    private:

        virtual bool Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        virtual void UnloadInternal( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;
        virtual Resource::InstallResult Install( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        TypeSystem::TypeRegistry const* m_pTypeRegistry = nullptr;
    };
}