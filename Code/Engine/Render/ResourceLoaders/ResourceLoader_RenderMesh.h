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

        virtual bool LoadInternal( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        virtual Resource::InstallResult Install( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const override;
        virtual Resource::InstallResult UpdateInstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;
        virtual void Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderDevice* m_pRenderDevice = nullptr;
    };
}