#include "ResourceLoader_RenderShader.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool ShaderLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        // Get shader resource
        Shader* pShaderResource = nullptr;
        auto const shaderResourceTypeID = resourceID.GetResourceTypeID();

        if ( shaderResourceTypeID == VertexShader::GetStaticResourceTypeID() )
        {
            auto pVertexShaderResource = EE::New<VertexShader>();
            archive << *pVertexShaderResource;
            pShaderResource = pVertexShaderResource;
        }
        else if ( shaderResourceTypeID == PixelShader::GetStaticResourceTypeID() )
        {
            auto pPixelShaderResource = EE::New<PixelShader>();
            archive << *pPixelShaderResource;
            pShaderResource = pPixelShaderResource;
        }
        else
        {
            return false;
        }

        EE_ASSERT( pShaderResource != nullptr );

        // Create shader
        m_pRenderDevice->LockDevice();
        m_pRenderDevice->CreateShader( *pShaderResource );
        m_pRenderDevice->UnlockDevice();
        pResourceRecord->SetResourceData( pShaderResource );
        return true;
    }

    void ShaderLoader::UnloadInternal( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        auto pShaderResource = pResourceRecord->GetResourceData<Shader>();
        if ( pShaderResource != nullptr )
        {
            m_pRenderDevice->LockDevice();
            m_pRenderDevice->DestroyShader( *pShaderResource );
            m_pRenderDevice->UnlockDevice();
        }

        ResourceLoader::UnloadInternal( resourceID, pResourceRecord );
    }
}