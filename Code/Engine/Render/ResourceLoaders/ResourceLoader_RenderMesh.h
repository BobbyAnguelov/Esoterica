#pragma once

#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;
    class RenderSystem;
    class SkeletalMesh;

    //-------------------------------------------------------------------------

    class MeshLoader final : public Resource::ResourceLoader
    {
    public:

        MeshLoader();

        inline void SetRenderSystemPtr( RenderSystem* pRenderSystem ) { EE_ASSERT( pRenderSystem != nullptr ); m_pRenderSystem = pRenderSystem; }
        inline void ClearRenderSystemPtr() { m_pRenderSystem = nullptr; }

        inline void SetTypeRegistryPtr( TypeSystem::TypeRegistry const* pTypeRegistry ) { EE_ASSERT( pTypeRegistry != nullptr ); m_pTypeRegistry = pTypeRegistry; }
        inline void ClearTypeRegistryPtr() { m_pTypeRegistry = nullptr; }

    private:

        virtual bool CanProceedWithFailedInstallDependency() const override { return true; }

        virtual Resource::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const override;
        virtual Resource::UnloadResult Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

        virtual Resource::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;
        virtual Resource::UnloadResult Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

        void LoadAndCreateProceduralBoneSettings( SkeletalMesh* pSkeletalMesh, Serialization::BinaryInputArchive* pArchive ) const;

    private:

        RenderSystem*                       m_pRenderSystem = nullptr;
        TypeSystem::TypeRegistry const*     m_pTypeRegistry = nullptr;
    };
}