#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderSystem;

    class MaterialLoader : public Resource::ResourceLoader
    {
    public:

        MaterialLoader();

        EE_FORCE_INLINE void SetRenderSystem( RenderSystem* pRenderSystem )
        {
            EE_ASSERT( !m_pRenderSystem );
            m_pRenderSystem = pRenderSystem;
        }

    private:

        virtual Resource::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const final;
        virtual Resource::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const final;
        virtual Resource::UnloadResult Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderSystem* m_pRenderSystem = nullptr;
    };
}
