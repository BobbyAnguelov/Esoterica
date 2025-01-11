#pragma once

#include "Engine/_Module/API.h"
#include "Base/Render/RenderShader.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Resource/ResourceLoader.h"

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

        virtual Resource::ResourceLoader::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
        virtual void Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const final;

    private:

        RenderDevice* m_pRenderDevice;
    };
}