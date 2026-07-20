#include "ResourceLoader_RenderMaterial.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    namespace ParameterIDs
    {
        static StaticStringID const g_shaderIndexID( "m_shaderIndex" );
        static StaticStringID const g_textureSamplerLinear( "m_textureSamplerLinear" );
        static StaticStringID const g_textureSamplerPoint( "m_textureSamplerPoint" );
    }

    //-------------------------------------------------------------------------

    MaterialLoader::MaterialLoader()
    {
        m_loadableTypes.push_back( Material::GetStaticResourceTypeID() );
    }

    Resource::LoadResult MaterialLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        Material* pMaterial = EE::New<Material>();
        ( *pArchive ) << *pMaterial;

        // Resolve shader (or use placeholder)
        //-------------------------------------------------------------------------

        static StringID const s_PlaceholderShaderID = StringID( "Placeholder" );

        pMaterial->m_shaderIndex = m_pRenderSystem->FindMaterialShaderIndex( pMaterial->m_shaderID );
        if ( pMaterial->m_shaderIndex == -1 )
        {
            EE_LOG_WARNING( LogCategory::Render, "Material Loader", "Material does not have a valid shader: %s", resourceID.c_str() );
            pMaterial->m_shaderIndex = m_pRenderSystem->FindMaterialShaderIndex( s_PlaceholderShaderID );
        }

        pResourceRecord->SetResourceData( pMaterial );

        return Resource::LoadResult::Complete;
    }

    Resource::LoadResult MaterialLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        Material* pMaterial = pResourceRecord->GetResourceData<Material>();
        EE_ASSERT( pMaterial != nullptr );

        // Kick off async request
        if ( pMaterial->m_pMaterialParametersUpdate == nullptr )
        {
            pMaterial->m_pMaterialParametersUpdate = m_pRenderSystem->CreateMaterialParametersAsync( pMaterial->m_shaderIndex );
        }

        // Check async state
        AsyncResourceUpdateState const updateState = pMaterial->m_pMaterialParametersUpdate->m_updateState.load();
        switch ( updateState )
        {
            case AsyncResourceUpdateState::UpdatePending:
            {
                EE_ASSERT( !pMaterial->m_shaderParametersInstance.IsValid() );
                pMaterial->m_shaderParametersInstance = pMaterial->m_pMaterialParametersUpdate->m_materialShaderParameters;

                MaterialShaderParameterHandle shaderIndexHandle = pMaterial->m_shaderParametersInstance.FindParameter( ParameterIDs::g_shaderIndexID );
                pMaterial->m_shaderParametersInstance.SetScalar( shaderIndexHandle, pMaterial->m_shaderIndex );

                for ( auto& texture : pMaterial->m_parameterStorage.m_textures )
                {
                    MaterialShaderParameterHandle parameterHandle = pMaterial->m_shaderParametersInstance.FindParameter( texture.m_parameterID );
                    if ( parameterHandle.IsValid() )
                    {
                        texture.m_parameterValue = GetInstallDependency( installDependencies, texture.m_parameterValue.GetResourceID() );
                        if ( !texture.m_parameterValue.IsLoaded() )
                        {
                            return Resource::LoadResult::Failed;
                        }
                        pMaterial->m_shaderParametersInstance.SetTexture( parameterHandle, RHI::GetTextureHandle( texture.m_parameterValue->GetTexture(), RHI::DescriptorTypeFlags::Texture, 0 ) );
                    }
                    else
                    {
                        EE_LOG_WARNING( LogCategory::Render, "Material Loader", "Invalid parameter \"%s\" for shader \"%s\" in material \"%s\"", texture.m_parameterID.c_str(), pMaterial->m_shaderID.c_str(), resourceID.c_str() );
                    }
                }

                if ( !pMaterial->m_shaderParametersInstance.ValidateResourceHandles() )
                {
                    EE_ASSERT( false ); // Should never reach this point
                    return Resource::LoadResult::Failed;
                }

                for ( auto const& scalar : pMaterial->m_parameterStorage.m_scalars )
                {
                    MaterialShaderParameterHandle parameterHandle = pMaterial->m_shaderParametersInstance.FindParameter( scalar.m_parameterID );
                    if ( parameterHandle.IsValid() )
                    {
                        pMaterial->m_shaderParametersInstance.SetScalar( parameterHandle, scalar.m_parameterValue );
                    }
                    else
                    {
                        EE_LOG_WARNING( LogCategory::Render, "Material Loader", "Invalid parameter \"%s\" for shader \"%s\" in material \"%s\"", scalar.m_parameterID.c_str(), pMaterial->m_shaderID.c_str(), resourceID.c_str() );
                    }
                }

                for ( auto const& vector2 : pMaterial->m_parameterStorage.m_vectors2 )
                {
                    MaterialShaderParameterHandle parameterHandle = pMaterial->m_shaderParametersInstance.FindParameter( vector2.m_parameterID );
                    if ( parameterHandle.IsValid() )
                    {
                        pMaterial->m_shaderParametersInstance.SetVector( parameterHandle, vector2.m_parameterValue );
                    }
                    else
                    {
                        EE_LOG_WARNING( LogCategory::Render, "Material Loader", "Invalid parameter \"%s\" for shader \"%s\" in material \"%s\"", vector2.m_parameterID.c_str(), pMaterial->m_shaderID.c_str(), resourceID.c_str() );
                    }
                }

                for ( auto const& vector4 : pMaterial->m_parameterStorage.m_vectors4 )
                {
                    MaterialShaderParameterHandle parameterHandle = pMaterial->m_shaderParametersInstance.FindParameter( vector4.m_parameterID );
                    if ( parameterHandle.IsValid() )
                    {
                        pMaterial->m_shaderParametersInstance.SetVector( parameterHandle, vector4.m_parameterValue );
                    }
                    else
                    {
                        EE_LOG_WARNING( LogCategory::Render, "Material Loader", "Invalid parameter \"%s\" for shader \"%s\" in material \"%s\"", vector4.m_parameterID.c_str(), pMaterial->m_shaderID.c_str(), resourceID.c_str() );
                    }
                }

                for ( auto const& matrix : pMaterial->m_parameterStorage.m_matrices )
                {
                    MaterialShaderParameterHandle parameterHandle = pMaterial->m_shaderParametersInstance.FindParameter( matrix.m_parameterID );
                    if ( parameterHandle.IsValid() )
                    {
                        pMaterial->m_shaderParametersInstance.SetMatrix( parameterHandle, matrix.m_parameterValue );
                    }
                    else
                    {
                        EE_LOG_WARNING( LogCategory::Render, "Material Loader", "Invalid parameter \"%s\" for shader \"%s\" in material \"%s\"", matrix.m_parameterID.c_str(), pMaterial->m_shaderID.c_str(), resourceID.c_str() );
                    }
                }

                // TODO: Sampler parameters are not implemented yet!
                MaterialShaderParameterHandle samplerHandle = pMaterial->m_shaderParametersInstance.FindParameter( ParameterIDs::g_textureSamplerLinear );
                if ( samplerHandle.IsValid() )
                {
                    pMaterial->m_shaderParametersInstance.SetSampler( samplerHandle, RHI::GetSamplerStateHandle( m_pRenderSystem->GetLinearWrapSampler() ) );
                }

                samplerHandle = pMaterial->m_shaderParametersInstance.FindParameter( ParameterIDs::g_textureSamplerPoint );
                if ( samplerHandle.IsValid() )
                {
                    pMaterial->m_shaderParametersInstance.SetSampler( samplerHandle, RHI::GetSamplerStateHandle( m_pRenderSystem->GetPointWrapSampler() ) );
                }

                pMaterial->m_pMaterialParametersUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );
            }
            break;

            case AsyncResourceUpdateState::CompletePending:
            {
                pMaterial->m_pMaterialParametersUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                pMaterial->m_pMaterialParametersUpdate = nullptr;

                return Resource::LoadResult::Complete;
            }
            break;

            default:
            break;
        }

        return Resource::LoadResult::InProgress;
    }

    Resource::UnloadResult MaterialLoader::Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        Material* pMaterial = pResourceRecord->GetResourceData<Material>();

        if ( pMaterial != nullptr )
        {
            if ( pMaterial->m_pMaterialParametersUpdate != nullptr )
            {
                AsyncResourceUpdateState const updateState = pMaterial->m_pMaterialParametersUpdate->m_updateState.load();
                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        EE_ASSERT( !pMaterial->m_shaderParametersInstance.IsValid() );
                        m_pRenderSystem->QueueResourceDelete( eastl::move( pMaterial->m_pMaterialParametersUpdate->m_materialShaderParameters ) );

                        pMaterial->m_pMaterialParametersUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                        pMaterial->m_pMaterialParametersUpdate = nullptr;
                        return Resource::UnloadResult::Complete;
                    }
                    break;

                    case AsyncResourceUpdateState::CompletePending:
                    {
                        EE_ASSERT( pMaterial->m_shaderParametersInstance.IsValid() );
                        m_pRenderSystem->QueueResourceDelete( eastl::move( pMaterial->m_shaderParametersInstance ) );

                        pMaterial->m_pMaterialParametersUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                        pMaterial->m_pMaterialParametersUpdate = nullptr;
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
            else if ( pMaterial->m_shaderParametersInstance.IsValid() )
            {
                m_pRenderSystem->QueueResourceDelete( eastl::move( pMaterial->m_shaderParametersInstance ) );
            }
        }

        return Resource::UnloadResult::Complete;
    }
}
