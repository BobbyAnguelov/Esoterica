#pragma once

#include "Engine/_Module/API.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Render/RenderTexture.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class TextureLoader final : public Resource::ResourceLoader
    {
    public:

        TextureLoader();

        inline void SetRenderDevicePtr( RenderDevice* pRenderDevice )
        {
            EE_ASSERT( pRenderDevice != nullptr );
            m_pRenderDevice = pRenderDevice;
        }

        inline void ClearRenderDevice() { m_pRenderDevice = nullptr; }

    private:

        virtual bool Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        virtual Resource::InstallResult Install( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const override;
        virtual Resource::InstallResult UpdateInstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;
        virtual void Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderDevice* m_pRenderDevice;
    };
}