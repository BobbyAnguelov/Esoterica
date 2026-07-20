#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;
    class RenderSystem;

    //-------------------------------------------------------------------------

    class MeshLoader final : public Resource::ResourceLoader
    {
    public:

        MeshLoader();

        inline void SetRenderSystem( RenderSystem* pRenderSystem )
        {
            EE_ASSERT( !m_pRenderSystem );
            m_pRenderSystem = pRenderSystem;
        }

    private:

        virtual bool CanProceedWithFailedInstallDependency() const override { return true; }

        virtual Resource::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const override;
        virtual Resource::UnloadResult Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

        virtual Resource::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderSystem* m_pRenderSystem = nullptr;
    };
}
