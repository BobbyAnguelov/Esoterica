#include "WorldRenderer.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Shaders/EngineShaders.h"
#include "Engine/Render/Systems/WorldSystem_Renderer.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Render/RenderCoreResources.h"
#include "Base/Render/RenderViewport.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static Matrix ComputeShadowMatrix( Viewport const& viewport, Transform const& lightWorldTransform, float shadowDistance )
    {
        Transform lightTransform = lightWorldTransform;
        lightTransform.SetTranslation( Vector::Zero );
        Transform const invLightTransform = lightTransform.GetInverse();

        // Get a modified camera view volume that has the shadow distance as the z far.
        // This will get us the appropriate corners to translate into light space.

        // To make these cascade, you do this in a loop and move the depth range along by your
        // cascade distance.
        EE::Math::ViewVolume camVolume = viewport.GetViewVolume();
        camVolume.SetDepthRange( FloatRange( 1.0f, shadowDistance ) );

        Math::ViewVolume::VolumeCorners corners = camVolume.GetCorners();

        // Translate into light space.
        for ( int32_t i = 0; i < 8; i++ )
        {
            corners.m_points[i] = invLightTransform.TransformPoint( corners.m_points[i] );
        }

        // Note for understanding, cornersMin and cornersMax are in light space, not world space.
        Vector cornersMin = Vector::One * FLT_MAX;
        Vector cornersMax = Vector::One * -FLT_MAX;

        for ( int32_t i = 0; i < 8; i++ )
        {
            cornersMin = Vector::Min( cornersMin, corners.m_points[i] );
            cornersMax = Vector::Max( cornersMax, corners.m_points[i] );
        }

        Vector lightPosition = Vector::Lerp( cornersMin, cornersMax, 0.5f );
        lightPosition = Vector::Select( lightPosition, cornersMax, Vector::Select0100 ); //force lightPosition to the "back" of the box.
        lightPosition = lightTransform.TransformPoint( lightPosition );   //Light position now in world space.
        lightTransform.SetTranslation( lightPosition );   //Assign to the lightTransform, now it's positioned above our view frustum.

        Float3 const delta = ( cornersMax - cornersMin ).ToFloat3();
        float dim = Math::Max( delta.m_x, delta.m_z );
        Math::ViewVolume lightViewVolume( Float2( dim ), FloatRange( 1.0, delta.m_y ), lightTransform.ToMatrix() );

        Matrix viewProjMatrix = lightViewVolume.GetViewProjectionMatrix();
        Matrix viewMatrix = lightViewVolume.GetViewMatrix();
        Matrix projMatrix = lightViewVolume.GetProjectionMatrix();
        Matrix viewProj = lightViewVolume.GetViewProjectionMatrix(); // TODO: inverse z???
        return viewProj;
    }

    //-------------------------------------------------------------------------

    bool WorldRenderer::Initialize( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( m_pRenderDevice == nullptr && pRenderDevice != nullptr );
        m_pRenderDevice = pRenderDevice;

        TVector<RenderBuffer> cbuffers;
        RenderBuffer buffer;

        // Create Static Mesh Vertex Shader
        //-------------------------------------------------------------------------

        cbuffers.clear();

        // World transform const buffer
        buffer.m_byteSize = sizeof( ObjectTransforms );
        buffer.m_byteStride = sizeof( Matrix ); // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        buffer.m_slot = 0;
        cbuffers.push_back( buffer );

        // Shaders
        auto const vertexLayoutDescStatic = VertexLayoutRegistry::GetDescriptorForFormat( VertexFormat::StaticMesh );
        m_vertexShaderStatic = VertexShader( g_byteCode_VS_StaticPrimitive, sizeof( g_byteCode_VS_StaticPrimitive ), cbuffers, vertexLayoutDescStatic );
        m_pRenderDevice->CreateShader( m_vertexShaderStatic );

        // Create Skeletal Mesh Vertex Shader
        //-------------------------------------------------------------------------

        // Vertex shader constant buffer - contains the world view projection matrix and bone transforms
        buffer.m_byteSize = sizeof( Matrix ) * 255; // ( 1 WVP matrix + 255 bone matrices )
        buffer.m_byteStride = sizeof( Matrix ); // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        buffer.m_slot = 0;
        cbuffers.push_back( buffer );

        auto const vertexLayoutDescSkeletal = VertexLayoutRegistry::GetDescriptorForFormat( VertexFormat::SkeletalMesh );
        m_vertexShaderSkeletal = VertexShader( g_byteCode_VS_SkinnedPrimitive, sizeof( g_byteCode_VS_SkinnedPrimitive ), cbuffers, vertexLayoutDescSkeletal );
        pRenderDevice->CreateShader( m_vertexShaderSkeletal );

        if ( !m_vertexShaderStatic.IsValid() )
        {
            return false;
        }

        // Create Skybox Vertex Shader
        //-------------------------------------------------------------------------

        cbuffers.clear();

        // Transform constant buffer
        buffer.m_byteSize = sizeof( Matrix );
        buffer.m_byteStride = sizeof( Matrix ); // Vector4 aligned
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        buffer.m_slot = 0;
        cbuffers.push_back( buffer );

        auto const vertexLayoutDescNone = VertexLayoutRegistry::GetDescriptorForFormat( VertexFormat::None );
        m_vertexShaderSkybox = VertexShader( g_byteCode_VS_Cube, sizeof( g_byteCode_VS_Cube ), cbuffers, vertexLayoutDescNone );
        m_pRenderDevice->CreateShader( m_vertexShaderSkybox );

        if ( !m_vertexShaderSkybox.IsValid() )
        {
            return false;
        }

        // Create Pixel Shader
        //-------------------------------------------------------------------------

        cbuffers.clear();

        // Pixel shader constant buffer - contains light info
        buffer.m_byteSize = sizeof( LightData );
        buffer.m_byteStride = sizeof( LightData );
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        buffer.m_slot = 0;
        cbuffers.push_back( buffer );

        buffer.m_byteSize = sizeof( MaterialData );
        buffer.m_byteStride = sizeof( MaterialData );
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        buffer.m_slot = 1;
        cbuffers.push_back( buffer );

        m_pixelShader = PixelShader( g_byteCode_PS_Lit, sizeof( g_byteCode_PS_Lit ), cbuffers );
        m_pRenderDevice->CreateShader( m_pixelShader );

        if ( !m_pixelShader.IsValid() )
        {
            return false;
        }

        // Create Skybox Pixel Shader
        //-------------------------------------------------------------------------

        m_pixelShaderSkybox = PixelShader( g_byteCode_PS_Skybox, sizeof( g_byteCode_PS_Skybox ), cbuffers );
        m_pRenderDevice->CreateShader( m_pixelShaderSkybox );

        if ( !m_pixelShaderSkybox.IsValid() )
        {
            return false;
        }

        // Create Picking-Enabled Pixel Shader
        //-------------------------------------------------------------------------

        buffer.m_byteSize = sizeof( PickingData );
        buffer.m_byteStride = sizeof( PickingData );
        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        buffer.m_type = RenderBuffer::Type::Constant;
        buffer.m_slot = 2;
        cbuffers.push_back( buffer );

        m_pixelShaderPicking = PixelShader( g_byteCode_PS_LitPicking, sizeof( g_byteCode_PS_LitPicking ), cbuffers );
        m_pRenderDevice->CreateShader( m_pixelShaderPicking );

        if ( !m_pixelShaderPicking.IsValid() )
        {
            return false;
        }

        // Create Empty Pixel Shader
        //-------------------------------------------------------------------------

        cbuffers.clear();

        m_emptyPixelShader = PixelShader( g_byteCode_PS_Empty, sizeof( g_byteCode_PS_Empty ), cbuffers );
        m_pRenderDevice->CreateShader( m_emptyPixelShader );

        if ( !m_emptyPixelShader.IsValid() )
        {
            return false;
        }

        // Create BRDF Integration Compute Shader
        //-------------------------------------------------------------------------

        cbuffers.clear();

        m_precomputeDFGComputeShader = ComputeShader( g_byteCode_CS_PrecomputeDFG, sizeof( g_byteCode_CS_PrecomputeDFG ), cbuffers );

        m_pRenderDevice->CreateShader( m_precomputeDFGComputeShader );
        if ( !m_precomputeDFGComputeShader.IsValid() )
        {
            return false;
        }

        // Create blend state
        //-------------------------------------------------------------------------

        m_blendState.m_srcValue = BlendValue::SourceAlpha;
        m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
        m_blendState.m_blendOp = BlendOp::Add;
        m_blendState.m_blendEnable = true;

        m_pRenderDevice->CreateBlendState( m_blendState );

        if ( !m_blendState.IsValid() )
        {
            return false;
        }

        // Create rasterizer state
        //-------------------------------------------------------------------------

        m_rasterizerState.m_cullMode = CullMode::BackFace;
        m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
        m_rasterizerState.m_fillMode = FillMode::Solid;
        m_pRenderDevice->CreateRasterizerState( m_rasterizerState );
        if ( !m_rasterizerState.IsValid() )
        {
            return false;
        }

        // Set up samplers
        //-------------------------------------------------------------------------

        m_pRenderDevice->CreateSamplerState( m_bilinearSampler );
        if ( !m_bilinearSampler.IsValid() )
        {
            return false;
        }

        m_bilinearClampedSampler.m_addressModeU = TextureAddressMode::Clamp;
        m_bilinearClampedSampler.m_addressModeV = TextureAddressMode::Clamp;
        m_bilinearClampedSampler.m_addressModeW = TextureAddressMode::Clamp;
        m_pRenderDevice->CreateSamplerState( m_bilinearClampedSampler );
        if ( !m_bilinearClampedSampler.IsValid() )
        {
            return false;
        }

        m_shadowSampler.m_addressModeU = TextureAddressMode::Border;
        m_shadowSampler.m_addressModeV = TextureAddressMode::Border;
        m_shadowSampler.m_addressModeW = TextureAddressMode::Border;
        m_shadowSampler.m_borderColor = Float4( 1.0f );
        m_pRenderDevice->CreateSamplerState( m_shadowSampler );
        if ( !m_shadowSampler.IsValid() )
        {
            return false;
        }

        // Set up input bindings
        //-------------------------------------------------------------------------

        m_pRenderDevice->CreateShaderInputBinding( m_vertexShaderStatic, vertexLayoutDescStatic, m_inputBindingStatic );
        if ( !m_inputBindingStatic.IsValid() )
        {
            return false;
        }

        m_pRenderDevice->CreateShaderInputBinding( m_vertexShaderSkeletal, vertexLayoutDescSkeletal, m_inputBindingSkeletal );
        if ( !m_inputBindingSkeletal.IsValid() )
        {
            return false;
        }

        // Set up pipeline states
        //-------------------------------------------------------------------------

        m_pipelineStateStatic.m_pVertexShader = &m_vertexShaderStatic;
        m_pipelineStateStatic.m_pPixelShader = &m_pixelShader;
        m_pipelineStateStatic.m_pBlendState = &m_blendState;
        m_pipelineStateStatic.m_pRasterizerState = &m_rasterizerState;

        m_pipelineStateStaticPicking = m_pipelineStateStatic;
        m_pipelineStateStaticPicking.m_pPixelShader = &m_pixelShaderPicking;

        m_pipelineStateSkeletal.m_pVertexShader = &m_vertexShaderSkeletal;
        m_pipelineStateSkeletal.m_pPixelShader = &m_pixelShader;
        m_pipelineStateSkeletal.m_pBlendState = &m_blendState;
        m_pipelineStateSkeletal.m_pRasterizerState = &m_rasterizerState;

        m_pipelineStateSkeletalPicking = m_pipelineStateSkeletal;
        m_pipelineStateSkeletalPicking.m_pPixelShader = &m_pixelShaderPicking;

        m_pipelineStateStaticShadow.m_pVertexShader = &m_vertexShaderStatic;
        m_pipelineStateStaticShadow.m_pPixelShader = &m_emptyPixelShader;
        m_pipelineStateStaticShadow.m_pBlendState = &m_blendState;
        m_pipelineStateStaticShadow.m_pRasterizerState = &m_rasterizerState;

        m_pipelineStateSkeletalShadow.m_pVertexShader = &m_vertexShaderSkeletal;
        m_pipelineStateSkeletalShadow.m_pPixelShader = &m_emptyPixelShader;
        m_pipelineStateSkeletalShadow.m_pBlendState = &m_blendState;
        m_pipelineStateSkeletalShadow.m_pRasterizerState = &m_rasterizerState;

        m_pipelineSkybox.m_pVertexShader = &m_vertexShaderSkybox;
        m_pipelineSkybox.m_pPixelShader = &m_pixelShaderSkybox;

        m_pRenderDevice->CreateTexture( m_precomputedBRDF, DataFormat::Float_R16G16, Float2( 512, 512 ), USAGE_UAV | USAGE_SRV ); // TODO: load from memory?
        m_pipelinePrecomputeBRDF.m_pComputeShader = &m_precomputeDFGComputeShader;

        // TODO create on directional light add and destroy on remove
        m_pRenderDevice->CreateTexture( m_shadowMap, DataFormat::Float_X32, Float2( 1536, 1536 ), USAGE_SRV | USAGE_RT_DS );

        {
            auto const& renderContext = m_pRenderDevice->GetImmediateContext();
            renderContext.SetPipelineState( m_pipelinePrecomputeBRDF );
            renderContext.SetUnorderedAccess( PipelineStage::Compute, 0, m_precomputedBRDF.GetUnorderedAccessView() );
            renderContext.Dispatch( 512 / 16, 512 / 16, 1 );
            renderContext.ClearUnorderedAccess( PipelineStage::Compute, 0 );
        }

        m_initialized = true;
        return true;
    }

    void WorldRenderer::Shutdown()
    {
        m_pipelineStateStatic.Clear();
        m_pipelineStateSkeletal.Clear();

        if ( m_inputBindingStatic.IsValid() )
        {
            m_pRenderDevice->DestroyShaderInputBinding( m_inputBindingStatic );
        }

        if ( m_inputBindingSkeletal.IsValid() )
        {
            m_pRenderDevice->DestroyShaderInputBinding( m_inputBindingSkeletal );
        }

        if ( m_rasterizerState.IsValid() )
        {
            m_pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        }

        if ( m_blendState.IsValid() )
        {
            m_pRenderDevice->DestroyBlendState( m_blendState );
        }

        if ( m_bilinearSampler.IsValid() )
        {
            m_pRenderDevice->DestroySamplerState( m_bilinearSampler );
        }

        if ( m_shadowSampler.IsValid() )
        {
            m_pRenderDevice->DestroySamplerState( m_shadowSampler );
        }

        if ( m_bilinearClampedSampler.IsValid() )
        {
            m_pRenderDevice->DestroySamplerState( m_bilinearClampedSampler );
        }

        if ( m_vertexShaderStatic.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_vertexShaderStatic );
        }

        if ( m_vertexShaderSkeletal.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_vertexShaderSkeletal );
        }

        if ( m_pixelShader.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_pixelShader );
        }

        if ( m_emptyPixelShader.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_emptyPixelShader );
        }

        if ( m_vertexShaderSkybox.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_vertexShaderSkybox );
        }

        if ( m_pixelShaderSkybox.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_pixelShaderSkybox );
        }

        if ( m_precomputeDFGComputeShader.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_precomputeDFGComputeShader );
        }

        if ( m_precomputedBRDF.IsValid() )
        {
            m_pRenderDevice->DestroyTexture( m_precomputedBRDF );
        }

        if ( m_shadowMap.IsValid() )
        {
            m_pRenderDevice->DestroyTexture( m_shadowMap );
        }

        if ( m_pixelShaderPicking.IsValid() )
        {
            m_pRenderDevice->DestroyShader( m_pixelShaderPicking );
        }

        m_pRenderDevice = nullptr;
        m_initialized = false;
    }

    //-------------------------------------------------------------------------

    void WorldRenderer::SetMaterial( RenderContext const& renderContext, PixelShader& pixelShader, Material const* pMaterial )
    {
        EE_ASSERT( pMaterial != nullptr );

        ViewSRVHandle const& defaultSRV = CoreResources::GetMissingTexture()->GetShaderResourceView();

        // TODO: cache on GPU in buffer
        MaterialData materialData;
        materialData.m_surfaceFlags |= pMaterial->HasAlbedoTexture() ? MATERIAL_USE_ALBEDO_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasMetalnessTexture() ? MATERIAL_USE_METALNESS_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasRoughnessTexture() ? MATERIAL_USE_ROUGHNESS_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasNormalMapTexture() ? MATERIAL_USE_NORMAL_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasAOTexture() ? MATERIAL_USE_AO_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_metalness = pMaterial->GetMetalnessValue();
        materialData.m_roughness = pMaterial->GetRoughnessValue();
        materialData.m_normalScaler = pMaterial->GetNormalScalerValue();
        materialData.m_albedo = pMaterial->GetAlbedoValue().ToFloat4();

        renderContext.WriteToBuffer( pixelShader.GetConstBuffer( 1 ), &materialData, sizeof( materialData ) );

        renderContext.SetShaderResource( PipelineStage::Pixel, 0, ( materialData.m_surfaceFlags & MATERIAL_USE_ALBEDO_TEXTURE ) ? pMaterial->GetAlbedoTexture()->GetShaderResourceView() : defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 1, ( materialData.m_surfaceFlags & MATERIAL_USE_NORMAL_TEXTURE ) ? pMaterial->GetNormalMapTexture()->GetShaderResourceView() : defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 2, ( materialData.m_surfaceFlags & MATERIAL_USE_METALNESS_TEXTURE ) ? pMaterial->GetMetalnessTexture()->GetShaderResourceView() : defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 3, ( materialData.m_surfaceFlags & MATERIAL_USE_ROUGHNESS_TEXTURE ) ? pMaterial->GetRoughnessTexture()->GetShaderResourceView() : defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 4, ( materialData.m_surfaceFlags & MATERIAL_USE_AO_TEXTURE ) ? pMaterial->GetAOTexture()->GetShaderResourceView() : defaultSRV );
    }

    void WorldRenderer::SetDefaultMaterial( RenderContext const& renderContext, PixelShader& pixelShader )
    {
        ViewSRVHandle const& defaultSRV = CoreResources::GetMissingTexture()->GetShaderResourceView();

        MaterialData materialData{};
        materialData.m_surfaceFlags |= MATERIAL_USE_ALBEDO_TEXTURE;
        materialData.m_metalness = 0.0f;
        materialData.m_roughness = 0.0f;
        materialData.m_normalScaler = 1.0f;
        materialData.m_albedo = Float4::One;
        renderContext.WriteToBuffer( pixelShader.GetConstBuffer( 1 ), &materialData, sizeof( materialData ) );

        renderContext.SetShaderResource( PipelineStage::Pixel, 0, defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 1, defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 2, defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 3, defaultSRV );
        renderContext.SetShaderResource( PipelineStage::Pixel, 4, defaultSRV );
    }

    //-------------------------------------------------------------------------

    void WorldRenderer::SetupRenderStates( Viewport const& viewport, PixelShader* pShader, RenderData const& data )
    {
        EE_ASSERT( pShader != nullptr && pShader->IsValid() );
        auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        renderContext.SetViewport( Float2( viewport.GetDimensions() ), Float2( viewport.GetTopLeftPosition() ) );
        renderContext.SetDepthTestMode( DepthTestMode::On );

        renderContext.SetSampler( PipelineStage::Pixel, 0, m_bilinearSampler );
        renderContext.SetSampler( PipelineStage::Pixel, 1, m_bilinearClampedSampler );
        renderContext.SetSampler( PipelineStage::Pixel, 2, m_shadowSampler );

        renderContext.WriteToBuffer( pShader->GetConstBuffer( 0 ), &data.m_lightData, sizeof( data.m_lightData ) );

        // Shadows
        if ( data.m_lightData.m_lightingFlags & LIGHTING_ENABLE_SUN_SHADOW )
        {
            renderContext.SetShaderResource( PipelineStage::Pixel, 10, m_shadowMap.GetShaderResourceView() );
        }
        else
        {
            renderContext.SetShaderResource( PipelineStage::Pixel, 10, CoreResources::GetMissingTexture()->GetShaderResourceView() );
        }

        // Skybox
        if ( data.m_pSkyboxRadianceTexture )
        {
            renderContext.SetShaderResource( PipelineStage::Pixel, 11, m_precomputedBRDF.GetShaderResourceView() );
            renderContext.SetShaderResource( PipelineStage::Pixel, 12, data.m_pSkyboxRadianceTexture->GetShaderResourceView() );
        }
        else
        {
            renderContext.SetShaderResource( PipelineStage::Pixel, 11, CoreResources::GetMissingTexture()->GetShaderResourceView() );
            renderContext.SetShaderResource( PipelineStage::Pixel, 12, ViewSRVHandle{} ); // TODO: fix add default cubemap resource
        }
    }

    void WorldRenderer::RenderStaticMeshes( Viewport const& viewport, RenderTarget const& renderTarget, RenderData const& data )
    {
        EE_PROFILE_FUNCTION_RENDER();

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        // Set primary render state and clear the render buffer
        //-------------------------------------------------------------------------

        PipelineState* pPipelineState = renderTarget.HasPickingRT() ? &m_pipelineStateStaticPicking : &m_pipelineStateStatic;
        SetupRenderStates( viewport, pPipelineState->m_pPixelShader, data );

        renderContext.SetPipelineState( *pPipelineState );
        renderContext.SetShaderInputBinding( m_inputBindingStatic );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //-------------------------------------------------------------------------

        for ( StaticMeshComponent const* pMeshComponent : data.m_staticMeshComponents )
        {
            auto pMesh = pMeshComponent->GetMesh();
            Vector const finalScale = pMeshComponent->GetLocalScale() * pMeshComponent->GetWorldTransform().GetScale();
            Matrix const worldTransform = Matrix( pMeshComponent->GetWorldTransform().GetRotation(), pMeshComponent->GetWorldTransform().GetTranslation(), finalScale );

            ObjectTransforms transforms = data.m_transforms;
            transforms.m_worldTransform = worldTransform;
            transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
            transforms.m_normalTransform = transforms.m_worldTransform.GetInverse().Transpose();
            renderContext.WriteToBuffer( m_vertexShaderStatic.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

            if ( renderTarget.HasPickingRT() )
            {
                PickingData const pd( pMeshComponent->GetEntityID().m_value, pMeshComponent->GetID().m_value );
                renderContext.WriteToBuffer( m_pixelShaderPicking.GetConstBuffer( 2 ), &pd, sizeof( PickingData ) );
            }

            renderContext.SetVertexBuffer( pMesh->GetVertexBuffer() );
            renderContext.SetIndexBuffer( pMesh->GetIndexBuffer() );

            TVector<Material const*> const& materials = pMeshComponent->GetMaterials();
            uint64_t const visibility = pMeshComponent->GetSectionVisibilityMask();

            auto const numSubMeshes = pMesh->GetNumSections();
            for ( auto i = 0u; i < numSubMeshes; i++ )
            {
                // Skip hidden sections
                if ( ( visibility & ( 1ull << i ) ) == 0 )
                {
                    continue;
                }

                // Set material
                if ( i < materials.size() && materials[i] )
                {
                    SetMaterial( renderContext, *pPipelineState->m_pPixelShader, materials[i] );
                }
                else // Use default material
                {
                    SetDefaultMaterial( renderContext, *pPipelineState->m_pPixelShader );
                }

                auto const& subMesh = pMesh->GetSection( i );
                renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
            }
        }
        renderContext.ClearShaderResource( PipelineStage::Pixel, 10 );
    }

    void WorldRenderer::RenderSkeletalMeshes( Viewport const& viewport, RenderTarget const& renderTarget, RenderData const& data )
    {
        EE_PROFILE_FUNCTION_RENDER();

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        // Set primary render state and clear the render buffer
        //-------------------------------------------------------------------------

        PipelineState* pPipelineState = renderTarget.HasPickingRT() ? &m_pipelineStateSkeletalPicking : &m_pipelineStateSkeletal;
        SetupRenderStates( viewport, pPipelineState->m_pPixelShader, data );

        renderContext.SetPipelineState( *pPipelineState );
        renderContext.SetShaderInputBinding( m_inputBindingSkeletal );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //-------------------------------------------------------------------------

        SkeletalMesh const* pCurrentMesh = nullptr;

        for ( SkeletalMeshComponent const* pMeshComponent : data.m_skeletalMeshComponents )
        {
            if ( pMeshComponent->GetMesh() != pCurrentMesh )
            {
                pCurrentMesh = pMeshComponent->GetMesh();
                EE_ASSERT( pCurrentMesh != nullptr && pCurrentMesh->IsValid() );

                renderContext.SetVertexBuffer( pCurrentMesh->GetVertexBuffer() );
                renderContext.SetIndexBuffer( pCurrentMesh->GetIndexBuffer() );
            }

            // Update Bones and Transforms
            //-------------------------------------------------------------------------

            Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
            ObjectTransforms transforms = data.m_transforms;
            transforms.m_worldTransform = worldTransform;
            transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
            transforms.m_normalTransform = transforms.m_worldTransform.GetInverse().Transpose();
            renderContext.WriteToBuffer( m_vertexShaderSkeletal.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

            auto const& bonesConstBuffer = m_vertexShaderSkeletal.GetConstBuffer( 1 );
            auto const& boneTransforms = pMeshComponent->GetSkinningTransforms();
            EE_ASSERT( boneTransforms.size() == pCurrentMesh->GetNumBones() );
            renderContext.WriteToBuffer( bonesConstBuffer, boneTransforms.data(), sizeof( Matrix ) * pCurrentMesh->GetNumBones() );

            if ( renderTarget.HasPickingRT() )
            {
                PickingData const pd( pMeshComponent->GetEntityID().m_value, pMeshComponent->GetID().m_value );
                renderContext.WriteToBuffer( m_pixelShaderPicking.GetConstBuffer( 2 ), &pd, sizeof( PickingData ) );
            }

            // Draw sub-meshes
            //-------------------------------------------------------------------------

            TVector<Material const*> const& materials = pMeshComponent->GetMaterials();
            uint64_t const visibility = pMeshComponent->GetSectionVisibilityMask();

            auto const numSubMeshes = pCurrentMesh->GetNumSections();
            for ( auto i = 0u; i < numSubMeshes; i++ )
            {
                // Skip hidden sections
                if ( ( visibility & ( 1ull << i ) ) == 0 )
                {
                    continue;
                }

                // Set material
                if ( i < materials.size() && materials[i] )
                {
                    SetMaterial( renderContext, *pPipelineState->m_pPixelShader, materials[i] );
                }
                else // Use default material
                {
                    SetDefaultMaterial( renderContext, *pPipelineState->m_pPixelShader );
                }

                // Draw mesh
                auto const& subMesh = pCurrentMesh->GetSection( i );
                renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
            }
        }
        renderContext.ClearShaderResource( PipelineStage::Pixel, 10 );
    }

    void WorldRenderer::RenderSkybox( Viewport const& viewport, RenderData const& data )
    {
        EE_PROFILE_FUNCTION_RENDER();

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();
        if ( data.m_pSkyboxTexture )
        {
            Matrix const skyboxTransform = Matrix( Quaternion::Identity, viewport.GetViewPosition(), Vector::One ) * data.m_transforms.m_viewprojTransform;

            renderContext.SetViewport( Float2( viewport.GetDimensions() ), Float2( viewport.GetTopLeftPosition() ), Float2( 1, 1 )/*TODO: fix for inv z*/ );
            renderContext.SetPipelineState( m_pipelineSkybox );
            renderContext.SetShaderInputBinding( ShaderInputBindingHandle() );
            renderContext.SetPrimitiveTopology( Topology::TriangleStrip );
            renderContext.WriteToBuffer( m_vertexShaderSkybox.GetConstBuffer( 0 ), &skyboxTransform, sizeof( Matrix ) );
            renderContext.WriteToBuffer( m_pixelShaderSkybox.GetConstBuffer( 0 ), &data.m_lightData, sizeof( data.m_lightData ) );
            renderContext.SetShaderResource( PipelineStage::Pixel, 0, data.m_pSkyboxTexture->GetShaderResourceView() );
            renderContext.Draw( 14, 0 );
        }
    }

    void WorldRenderer::RenderSunShadows( Viewport const& viewport, DirectionalLightComponent* pDirectionalLightComponent, RenderData const& data )
    {
        EE_PROFILE_FUNCTION_RENDER();

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        if ( !pDirectionalLightComponent || !pDirectionalLightComponent->GetShadowed() ) return;

        // Set primary render state and clear the render buffer
        //-------------------------------------------------------------------------

        renderContext.ClearDepthStencilView( m_shadowMap.GetDepthStencilView(), 1.0f/*TODO: inverse z*/, 0 );
        renderContext.SetRenderTarget( m_shadowMap.GetDepthStencilView() );
        renderContext.SetViewport( Float2( (float) m_shadowMap.GetDimensions().m_x, (float) m_shadowMap.GetDimensions().m_y ), Float2( 0.0f, 0.0f ) );
        renderContext.SetDepthTestMode( DepthTestMode::On );

        ObjectTransforms transforms;
        transforms.m_viewprojTransform = data.m_lightData.m_sunShadowMapMatrix;

        // Static Meshes
        //-------------------------------------------------------------------------

        renderContext.SetPipelineState( m_pipelineStateStaticShadow );
        renderContext.SetShaderInputBinding( m_inputBindingStatic );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );

        for ( StaticMeshComponent const* pMeshComponent : data.m_staticMeshComponents )
        {
            auto pMesh = pMeshComponent->GetMesh();
            Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
            transforms.m_worldTransform = worldTransform;
            renderContext.WriteToBuffer( m_vertexShaderStatic.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

            renderContext.SetVertexBuffer( pMesh->GetVertexBuffer() );
            renderContext.SetIndexBuffer( pMesh->GetIndexBuffer() );

            auto const numSubMeshes = pMesh->GetNumSections();
            for ( auto i = 0u; i < numSubMeshes; i++ )
            {
                auto const& subMesh = pMesh->GetSection( i );
                renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
            }
        }

        // Skeletal Meshes
        //-------------------------------------------------------------------------

        renderContext.SetPipelineState( m_pipelineStateSkeletalShadow );
        renderContext.SetShaderInputBinding( m_inputBindingSkeletal );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );

        for ( SkeletalMeshComponent const* pMeshComponent : data.m_skeletalMeshComponents )
        {
            auto pMesh = pMeshComponent->GetMesh();

            // Update Bones and Transforms
            //-------------------------------------------------------------------------

            Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
            transforms.m_worldTransform = worldTransform;
            transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
            renderContext.WriteToBuffer( m_vertexShaderSkeletal.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

            auto const& bonesConstBuffer = m_vertexShaderSkeletal.GetConstBuffer( 1 );
            auto const& boneTransforms = pMeshComponent->GetSkinningTransforms();
            EE_ASSERT( boneTransforms.size() == pMesh->GetNumBones() );
            renderContext.WriteToBuffer( bonesConstBuffer, boneTransforms.data(), sizeof( Matrix ) * pMesh->GetNumBones() );

            renderContext.SetVertexBuffer( pMesh->GetVertexBuffer() );
            renderContext.SetIndexBuffer( pMesh->GetIndexBuffer() );

            // Draw sub-meshes
            //-------------------------------------------------------------------------
            auto const numSubMeshes = pMesh->GetNumSections();
            for ( auto i = 0u; i < numSubMeshes; i++ )
            {
                // Draw mesh
                auto const& subMesh = pMesh->GetSection( i );
                renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
            }
        }
    }

    //-------------------------------------------------------------------------

    void WorldRenderer::RenderWorld( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld )
    {
        EE_ASSERT( IsInitialized() && Threading::IsMainThread() );
        EE_PROFILE_FUNCTION_RENDER();

        if ( !viewport.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pWorldSystem = pWorld->GetWorldSystem<RendererWorldSystem>();
        EE_ASSERT( pWorldSystem != nullptr );

        //-------------------------------------------------------------------------

        RenderData renderData
        {
            ObjectTransforms(),
            LightData(),
            nullptr,
            nullptr,
            pWorldSystem->m_visibleStaticMeshComponents,
            pWorldSystem->m_visibleSkeletalMeshComponents,
        };

        renderData.m_transforms.m_viewprojTransform = viewport.GetViewVolume().GetViewProjectionMatrix();

        //-------------------------------------------------------------------------

        uint32_t lightingFlags = 0;

        DirectionalLightComponent* pDirectionalLightComponent = nullptr;
        if ( !pWorldSystem->m_registeredDirectionLightComponents.empty() )
        {
            pDirectionalLightComponent = pWorldSystem->m_registeredDirectionLightComponents[0];
            lightingFlags |= LIGHTING_ENABLE_SUN;
            lightingFlags |= pDirectionalLightComponent->GetShadowed() ? LIGHTING_ENABLE_SUN_SHADOW : 0;
            renderData.m_lightData.m_SunDirIndirectIntensity = -pDirectionalLightComponent->GetLightDirection();
            Float4 colorIntensity = pDirectionalLightComponent->GetLightColor();
            renderData.m_lightData.m_SunColorRoughnessOneLevel = colorIntensity * pDirectionalLightComponent->GetLightIntensity();
            // TODO: conditional
            renderData.m_lightData.m_sunShadowMapMatrix = ComputeShadowMatrix( viewport, pDirectionalLightComponent->GetWorldTransform(), 50.0f/*TODO: configure*/ );
        }

        renderData.m_lightData.m_SunColorRoughnessOneLevel.SetW0();
        if ( !pWorldSystem->m_registeredGlobalEnvironmentMaps.empty() )
        {
            GlobalEnvironmentMapComponent* pGlobalEnvironmentMapComponent = pWorldSystem->m_registeredGlobalEnvironmentMaps[0];
            if ( pGlobalEnvironmentMapComponent->HasSkyboxRadianceTexture() && pGlobalEnvironmentMapComponent->HasSkyboxTexture() )
            {
                lightingFlags |= LIGHTING_ENABLE_SKYLIGHT;
                renderData.m_pSkyboxRadianceTexture = pGlobalEnvironmentMapComponent->GetSkyboxRadianceTexture();
                renderData.m_pSkyboxTexture = pGlobalEnvironmentMapComponent->GetSkyboxTexture();
                renderData.m_lightData.m_SunColorRoughnessOneLevel.SetW( Math::Max( Math::Floor( Math::Log2f( (float) renderData.m_pSkyboxRadianceTexture->GetDimensions().m_x ) ) - 1.0f, 0.0f ) );
                renderData.m_lightData.m_SunDirIndirectIntensity.SetW( pGlobalEnvironmentMapComponent->GetSkyboxIntensity() );
                renderData.m_lightData.m_manualExposure = pGlobalEnvironmentMapComponent->GetExposure();
            }
        }

        int32_t const numPointLights = Math::Min( pWorldSystem->m_registeredPointLightComponents.size(), (int32_t) s_maxPunctualLights );
        uint32_t lightIndex = 0;
        for ( int32_t i = 0; i < numPointLights; ++i )
        {
            EE_ASSERT( lightIndex < s_maxPunctualLights );
            PointLightComponent* pPointLightComponent = pWorldSystem->m_registeredPointLightComponents[i];
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr = pPointLightComponent->GetLightPosition();
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr.SetW( Math::Sqr( 1.0f / pPointLightComponent->GetLightRadius() ) );
            renderData.m_lightData.m_punctualLights[lightIndex].m_dir = Vector::Zero;
            renderData.m_lightData.m_punctualLights[lightIndex].m_color = Vector( pPointLightComponent->GetLightColor().ToFloat4() ) * pPointLightComponent->GetLightIntensity();
            renderData.m_lightData.m_punctualLights[lightIndex].m_spotAngles = Vector( -1.0f, 1.0f, 0.0f );
            ++lightIndex;
        }

        int32_t const numSpotLights = Math::Min( pWorldSystem->m_registeredSpotLightComponents.size(), (int32_t) s_maxPunctualLights - numPointLights );
        for ( int32_t i = 0; i < numSpotLights; ++i )
        {
            EE_ASSERT( lightIndex < s_maxPunctualLights );
            SpotLightComponent* pSpotLightComponent = pWorldSystem->m_registeredSpotLightComponents[i];
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr = pSpotLightComponent->GetLightPosition();
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr.SetW( Math::Sqr( 1.0f / pSpotLightComponent->GetLightRadius() ) );
            renderData.m_lightData.m_punctualLights[lightIndex].m_dir = -pSpotLightComponent->GetLightDirection();
            renderData.m_lightData.m_punctualLights[lightIndex].m_color = Vector( pSpotLightComponent->GetLightColor().ToFloat4() ) * pSpotLightComponent->GetLightIntensity();
            Radians innerAngle = pSpotLightComponent->GetLightInnerUmbraAngle().ToRadians();
            Radians outerAngle = pSpotLightComponent->GetLightOuterUmbraAngle().ToRadians();
            innerAngle.Clamp( 0, Math::PiDivTwo );
            outerAngle.Clamp( 0, Math::PiDivTwo );

            float cosInner = Math::Cos( (float) innerAngle );
            float cosOuter = Math::Cos( (float) outerAngle );
            renderData.m_lightData.m_punctualLights[lightIndex].m_spotAngles = Vector( cosOuter, 1.0f / Math::Max( cosInner - cosOuter, 0.001f ), 0.0f );
            ++lightIndex;
        }

        renderData.m_lightData.m_numPunctualLights = lightIndex;

        //-------------------------------------------------------------------------

        renderData.m_lightData.m_lightingFlags = lightingFlags;

        #if EE_DEVELOPMENT_TOOLS
        renderData.m_lightData.m_lightingFlags = renderData.m_lightData.m_lightingFlags | ( (int32_t) pWorldSystem->GetVisualizationMode() << (int32_t) RendererWorldSystem::VisualizationMode::BitShift );
        #endif

        //-------------------------------------------------------------------------

        auto const& immediateContext = m_pRenderDevice->GetImmediateContext();

        RenderSunShadows( viewport, pDirectionalLightComponent, renderData );
        {
            immediateContext.SetRenderTarget( renderTarget );
            RenderStaticMeshes( viewport, renderTarget, renderData );
            RenderSkeletalMeshes( viewport, renderTarget, renderData );
        }
        RenderSkybox( viewport, renderData );
    }
}