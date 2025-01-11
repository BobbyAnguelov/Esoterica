#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderDevice;
    class Mesh;

    //-------------------------------------------------------------------------

    class MeshLoader final : public Resource::ResourceLoader
    {
    public:

        MeshLoader();

        inline void SetRenderDevicePtr( RenderDevice* pRenderDevice )
        {
            EE_ASSERT( pRenderDevice != nullptr );
            m_pRenderDevice = pRenderDevice;
        }

        inline void ClearRenderDevicePtr() { m_pRenderDevice = nullptr; }

    private:

        virtual bool CanProceedWithFailedInstallDependency() const override { return true; }
        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        virtual Resource::ResourceLoader::LoadResult Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;
        virtual void Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderDevice* m_pRenderDevice = nullptr;
    };
}