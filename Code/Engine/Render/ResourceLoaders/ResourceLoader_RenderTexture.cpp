#include "ResourceLoader_RenderTexture.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileStreams.h"
#include "Engine/Render/RenderSystem.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    TextureLoader::TextureLoader()
    {
        m_loadableTypes.push_back( TextureResource::GetStaticResourceTypeID() );
    }

    Resource::LoadResult TextureLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        TextureResource* pTextureResource = pResourceRecord->GetResourceData<TextureResource>();

        // Create resource and request texture
        //-------------------------------------------------------------------------

        if ( pTextureResource == nullptr )
        {
            pTextureResource = EE::New<TextureResource>();
            ( *pArchive ) << *pTextureResource;

            //-------------------------------------------------------------------------

            pResourceRecord->SetResourceData( pTextureResource );

            RHI::TextureParameters textureParameters = {};
            textureParameters.m_width = pTextureResource->GetWidth();
            textureParameters.m_height = pTextureResource->GetHeight();
            textureParameters.m_depth = pTextureResource->GetDepth();
            textureParameters.m_arrayLayers = pTextureResource->GetArrayLayers();
            textureParameters.m_mipLevels = pTextureResource->GetMipLevels();
            textureParameters.m_format = RHI::DataFormat( pTextureResource->GetFormat() );
            textureParameters.m_initialState = RHI::TextureState::Common;
            textureParameters.m_debugName = resourcePath.c_str();

            pTextureResource->m_pAsyncTextureUpdate = m_pRenderSystem->CreateTextureAsync( textureParameters );
        }

        // Wait for async queue
        //-------------------------------------------------------------------------

        EE_ASSERT( pTextureResource != nullptr );
        EE_ASSERT( pTextureResource->m_pAsyncTextureUpdate != nullptr );

        AsyncResourceUpdateState const updateState = pTextureResource->m_pAsyncTextureUpdate->m_updateState.load();
        switch ( updateState )
        {
            case AsyncResourceUpdateState::UpdatePending:
            {
                FileSystem::Path const additionalDataFilePath = Resource::IResource::GetAdditionalDataFilePath( resourcePath );
                FileSystem::InputFileStream inputFileStream( additionalDataFilePath );
                if ( !inputFileStream.IsValid() )
                {
                    return Resource::LoadResult::Failed;
                }

                size_t dstOffset = 0;
                for ( uint32_t arrayLayer = 0; arrayLayer < pTextureResource->m_pAsyncTextureUpdate->m_numArrayLayers; ++arrayLayer )
                {
                    for ( uint32_t mipLevel = 0; mipLevel < pTextureResource->m_pAsyncTextureUpdate->m_numMipLevels; ++mipLevel )
                    {
                        RHI::TextureCopyRegion copyRegion = pTextureResource->m_pAsyncTextureUpdate->m_dstCopyRegion.SkipTo( mipLevel, arrayLayer );
                        uint32_t srcRowCount = RHI::ComputeFormatNumRows( pTextureResource->m_pAsyncTextureUpdate->m_pDstTexture->m_format, copyRegion.m_height );
                        uint32_t srcRowStride = RHI::ComputeFormatRowStride( pTextureResource->m_pAsyncTextureUpdate->m_pDstTexture->m_format, copyRegion.m_width );
                        uint32_t dstRowStride = RHI::GetTextureCopyRowStride( pTextureResource->m_pAsyncTextureUpdate->m_pDstTexture, mipLevel, arrayLayer );

                        for ( uint32_t slice = 0; slice < copyRegion.m_depth; ++slice )
                        {
                            for ( uint32_t row = 0; row < srcRowCount; ++row )
                            {
                                inputFileStream.Read( pTextureResource->m_pAsyncTextureUpdate->m_pDstMemory_WriteCombined + dstOffset, srcRowStride );
                                dstOffset += dstRowStride;
                            }
                        }
                    }
                }

                pTextureResource->m_pTexture = pTextureResource->m_pAsyncTextureUpdate->m_pDstTexture;
                pTextureResource->m_pAsyncTextureUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );

                return Resource::LoadResult::InProgress;
            }
            break;

            case AsyncResourceUpdateState::CompletePending:
            {
                pTextureResource->m_pAsyncTextureUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                pTextureResource->m_pAsyncTextureUpdate = nullptr;

                return Resource::LoadResult::Complete;
            }
            break;

            default:
            break;
        }

        return Resource::LoadResult::InProgress;
    }

    Resource::UnloadResult TextureLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pTextureResource = pResourceRecord->GetResourceData<TextureResource>();
        if ( pTextureResource != nullptr )
        {
            // If we have an active async request
            if ( pTextureResource->m_pAsyncTextureUpdate != nullptr )
            {
                AsyncResourceUpdateState updateState = pTextureResource->m_pAsyncTextureUpdate->m_updateState.load();
                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        EE_ASSERT( pTextureResource->m_pTexture == nullptr );
                        EE_ASSERT( pTextureResource->m_pAsyncTextureUpdate->m_pDstTexture != nullptr );

                        m_pRenderSystem->QueueResourceDelete( eastl::move( pTextureResource->m_pAsyncTextureUpdate->m_pDstTexture ) );

                        pTextureResource->m_pAsyncTextureUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                        pTextureResource->m_pAsyncTextureUpdate = nullptr;
                        return Resource::UnloadResult::Complete;
                    }
                    break;

                    case AsyncResourceUpdateState::CompletePending:
                    {
                        EE_ASSERT( pTextureResource->m_pTexture != nullptr );
                        m_pRenderSystem->QueueResourceDelete( eastl::move( pTextureResource->m_pTexture ) );

                        pTextureResource->m_pAsyncTextureUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                        pTextureResource->m_pAsyncTextureUpdate = nullptr;
                        return Resource::UnloadResult::Complete;
                    }
                    break;

                    default:
                    {
                        return Resource::UnloadResult::InProgress;
                    }
                    break;
                }
            }
            // If the texture is fully loaded
            else if ( pTextureResource->m_pTexture != nullptr )
            {
                m_pRenderSystem->QueueResourceDelete( eastl::move( pTextureResource->m_pTexture ) );
            }
        }

        return Resource::UnloadResult::Complete;
    }
}