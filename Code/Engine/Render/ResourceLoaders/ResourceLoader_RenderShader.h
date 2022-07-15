#pragma once

#include "Engine/_Module/API.h"
#include "System/Render/RenderShader.h"
#include "System/Render/RenderDevice.h"
#include "System/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class ShaderLoader : public Resource::ResourceLoader
    {
    public:

        ShaderLoader() : m_pRenderDevice( nullptr )
        {
            m_loadableTypes.push_back( PixelShader::GetStaticResourceTypeID() );
            m_loadableTypes.push_back( VertexShader::GetStaticResourceTypeID() );
        }

        inline void SetRenderDevicePtr( RenderDevice* pRenderDevice )
        {
            EE_ASSERT( pRenderDevice != nullptr );
            m_pRenderDevice = pRenderDevice;
        }

        inline void ClearRenderDevicePtr() { m_pRenderDevice = nullptr; }

    private:

        virtual bool LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
        virtual void UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const final;

    private:

        RenderDevice* m_pRenderDevice;
    };
}