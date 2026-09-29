#include "WorldSystem_Render.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderViewport.h"
#include "Base/Types/Arrays.h"
#include "Base/Profiling.h"
#include "Base/Render/RHI.h"
#include "Base/Threading/TaskSystem.h"

#include "Engine/Render/Shaders/MeshInstance.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    #if EE_DEVELOPMENT_TOOLS
    void RenderWorldSystem::UpdateViewportPickingData( RenderViewport* pViewport ) const
    {
        PickingData& pickingData = pViewport->GetPickingData();
        pickingData.clear();

        if ( !pViewport->IsPickingEnabled() )
        {
            return;
        }

        auto ResolvePickingData = [this] ( DeviceAppendBuffer<PickingResult> const& buffer, PickingData& pickingData )
        {
            auto TryResolvePickingID = [this] ( PickingResult const& pr )
            {
                if ( pr.m_hitTestID != PickingID::InvalidID )
                {
                    return PickingID( pr.m_hitTestID, PickingID::InvalidID, pr.m_sortPriority, pr.m_intersectionDistance );
                }

                for ( StaticMeshComponent const* pComponent : m_staticMeshComponents )
                {
                    HandleAllocator<uint32_t>::Handle const& instanceRootHandle = pComponent->m_meshInstanceRootProxy.m_instanceHandle;

                    if ( instanceRootHandle.IsValid() && ( instanceRootHandle.m_offset <= pr.m_instanceID ) && ( pr.m_instanceID < ( instanceRootHandle.m_offset + instanceRootHandle.m_size ) ) )
                    {
                        return PickingID( pComponent->GetEntityID().m_value, pComponent->GetID().m_value, pr.m_sortPriority, pr.m_intersectionDistance );
                    }
                }

                for ( SkeletalMeshComponent const* pComponent : m_skeletalMeshComponents )
                {
                    HandleAllocator<uint32_t>::Handle const& instanceRootHandle = pComponent->m_meshInstanceRootProxy.m_instanceHandle;

                    if ( instanceRootHandle.IsValid() && ( instanceRootHandle.m_offset <= pr.m_instanceID ) && ( pr.m_instanceID < ( instanceRootHandle.m_offset + instanceRootHandle.m_size ) ) )
                    {
                        return PickingID( pComponent->GetEntityID().m_value, pComponent->GetID().m_value, pr.m_sortPriority, pr.m_intersectionDistance );
                    }
                }

                return PickingID();
            };

            //-------------------------------------------------------------------------

            for ( PickingResult const& result : buffer.m_bufferData )
            {
                PickingID pickingID = TryResolvePickingID( result );
                if ( pickingID.IsSet() )
                {
                    pickingData.push_back( pickingID );
                }
            }
        };

        uint32_t frameIndex = m_pRenderSystem->GetFrameIndex();

        ResolvePickingData( pViewport->m_debugDrawPickingResultsBuffer, pickingData );
        ResolvePickingData( pViewport->m_instancePickingResultsBuffer, pickingData );

        pickingData.DeduplicateAndSort();
    }

    void RenderWorldSystem::SetOutlinedComponents( TArrayView<ComponentID> componentIDs )
    {
        // De-duplicate and check if the set of component IDs is actually different
        //-------------------------------------------------------------------------

        TVector<ComponentID> uniqueIDs;
        for ( ComponentID ID : componentIDs )
        {
            VectorEmplaceBackUnique( uniqueIDs, ID );
        }

        if ( uniqueIDs.size() == m_outlinedComponents.size() )
        {
            bool allElementsMatch = true;
            for ( ComponentID ID : uniqueIDs )
            {
                if ( !VectorContains( m_outlinedComponents, ID ) )
                {
                    allElementsMatch = false;
                    break;
                }
            }

            if ( allElementsMatch )
            {
                return;
            }
        }

        m_outlinedComponents.swap( uniqueIDs );

        // Set highlighted components
        //-------------------------------------------------------------------------

        if ( m_meshInstanceRootOutlineData.size() != m_deviceRenderWorld.GetNumMeshInstanceRootPages() )
        {
            m_meshInstanceRootOutlineData.resize( m_deviceRenderWorld.GetNumMeshInstanceRootPages(), 0 );
        }

        Memory::MemsetZero( m_meshInstanceRootOutlineData.data(), m_meshInstanceRootOutlineData.size() * sizeof( uint64_t ) );

        auto SetOutlineBit = [this] ( uint32_t rootIndex )
        {
            EE_ASSERT( rootIndex < m_meshInstanceRootOutlineData.size() * 64 );
            m_meshInstanceRootOutlineData[rootIndex >> 6] |= ( 1ULL << ( rootIndex & 63U ) );
        };

        for ( ComponentID const& componentID : m_outlinedComponents )
        {
            if ( StaticMeshComponent const* const* ppComponent = m_staticMeshComponents.FindItem( componentID ) )
            {
                HandleAllocator<uint32_t>::Handle const& instanceHandle = ( *ppComponent )->m_meshInstanceRootProxy.m_instanceHandle;
                if ( instanceHandle.IsValid() )
                {
                    SetOutlineBit( instanceHandle.m_offset );
                }
                continue;
            }

            if ( SkeletalMeshComponent const* const* ppComponent = m_skeletalMeshComponents.FindItem( componentID ) )
            {
                HandleAllocator<uint32_t>::Handle const& instanceHandle = ( *ppComponent )->m_meshInstanceRootProxy.m_instanceHandle;
                if ( instanceHandle.IsValid() )
                {
                    SetOutlineBit( instanceHandle.m_offset );
                }
            }
        }

        m_meshInstanceRootOutlineNeedUpdate = true;
    }

    void RenderWorldSystem::ClearOutlinedComponents()
    {
        if ( m_outlinedComponents.empty() )
        {
            return;
        }

        if ( !m_meshInstanceRootOutlineData.empty() )
        {
            Memory::MemsetZero( m_meshInstanceRootOutlineData.data(), m_meshInstanceRootOutlineData.size() * sizeof( uint64_t ) );
        }

        m_meshInstanceRootOutlineNeedUpdate = true;
        m_outlinedComponents.clear();
    }
    #endif

    void RenderWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        m_pTaskSystem = systemRegistry.GetSystem<TaskSystem>();
        m_pRenderSystem = systemRegistry.GetSystem<RenderSystem>();

        m_deviceRenderWorld.Initialize( m_pTaskSystem, m_pRenderSystem );

        #if EE_DEVELOPMENT_TOOLS
        m_meshInstanceRootOutlineBuffer.Initialize( m_pRenderSystem->GetContextRHI(), true );
        #endif
    }

    void RenderWorldSystem::ShutdownSystem()
    {
        m_pRenderSystem->WaitAllQueuesIdle();

        m_deviceRenderWorld.Shutdown( m_pRenderSystem );

        #if EE_DEVELOPMENT_TOOLS
        m_meshInstanceRootOutlineBuffer.Shutdown( m_pRenderSystem->GetContextRHI() );
        #endif

        m_pRenderSystem = nullptr;
        m_pTaskSystem = nullptr;
    }

    //-------------------------------------------------------------------------

    void RenderWorldSystem::UpdateDirectionalLightShadows( Math::ViewVolume const& viewVolume )
    {
        float const shadowMapResolution = float( m_pRenderSystem->GetRenderSettings()->m_cascadedShadowResolution );
        float const shadowMapTexelSize = 1.0F / shadowMapResolution;

        Matrix const textureScaleBiasMatrix
        (
            Vector( 0.5F, 0.0F, 0.0F, 0.0F ),
            Vector( 0.0F, -0.5F, 0.0F, 0.0F ),
            Vector( 0.0F, 0.0F, 1.0F, 0.0F ),
            Vector( 0.5F, 0.5F, 0.0F, 1.0F )
        );

        Math::ViewVolume::VolumeCorners frustumCorners = viewVolume.GetCorners();
        FloatRange const depthRange = viewVolume.GetDepthRange();

        for ( DirectionalLightComponent* pLightComponent : m_directionalLightComponents )
        {
            RenderViewProxy& renderViewProxy = pLightComponent->m_cascadedShadowRenderViewProxy;

            Vector lightDirection = -pLightComponent->GetLightDirection();
            lightDirection.SetW0();

            //-------------------------------------------------------------------------

            ShaderTypes::LightInstance_DirectionalLight light = {};
            light.m_maxIntensity = pLightComponent->GetMaxIntensity();
            light.m_packedTintedColor = pLightComponent->GetTintedColor().ToUInt32();
            light.m_shadowCascades = RHI::g_invalidResourceHandle;

            Float3 const lightDirectionAsFloat3 = lightDirection.ToFloat3();
            std::memcpy( &light.m_lightDirection, &lightDirectionAsFloat3, sizeof( Float3 ) );

            if ( !renderViewProxy.IsValid() )
            {
                pLightComponent->m_lightInstanceProxy.WriteDirectionalLight( light );
                continue;
            }

            // A sun close to the zenith would make the view direction parallel to the world up and explode.
            // Same fallback the light editor uses.
            Vector viewUpDirection = Vector::WorldUp;
            if ( Math::Abs( lightDirection.GetDot3( viewUpDirection ) ) > 0.99F )
            {
                viewUpDirection = Vector::WorldRight;
            }

            TArrayView<DeviceRenderView> shadowViews = renderViewProxy.m_renderViewHandle.m_data;

            DeviceRenderView& shadowView = shadowViews[0];
            EE_ASSERT( shadowView.m_depthTexture != nullptr );

            light.m_shadowCascades = RHI::GetTextureHandle( shadowView.m_depthTexture, RHI::DescriptorTypeFlags::Texture, 0 );
            light.m_cascadeSize[0] = shadowMapResolution;
            light.m_cascadeSize[1] = shadowMapResolution;
            light.m_cascadeSize[2] = shadowMapTexelSize;
            light.m_cascadeSize[3] = shadowMapTexelSize;

            //-------------------------------------------------------------------------

            Matrix globalShadowMatrix = Matrix::Identity;
            {
                Vector frustumCenter = Vector::Zero;
                for ( Vector const& corner : frustumCorners.m_points )
                {
                    frustumCenter += corner;
                }
                frustumCenter *= 1.0F / 8.0F;
                frustumCenter.SetW1();

                Matrix shadowProjectionMatrix = Math::CreateOrthographicProjectionMatrixOffCenter
                (
                    -0.5F, 0.5F, -0.5F, 0.5F,
                    0.0F, 1.0F
                );
                shadowProjectionMatrix = shadowProjectionMatrix * Matrix::ReverseZ;

                Matrix shadowViewMatrix = Math::CreateLookAtMatrix( frustumCenter + lightDirection * 0.5F, frustumCenter, viewUpDirection );
                Matrix shadowViewProjectionMatrix = shadowViewMatrix * shadowProjectionMatrix;
                globalShadowMatrix = shadowViewProjectionMatrix * textureScaleBiasMatrix;

                std::memcpy( light.m_shadowMatrix, globalShadowMatrix.m_rows, sizeof( light.m_shadowMatrix ) );
            }

            // Cascade splits
            //-------------------------------------------------------------------------

            TArray<float, g_NumCascadedShadowViews> cascadeSplits = {};

            float const lambda = 0.94F;
            float const minDistance = 0.0F;
            float const maxDistance = 1.0F;

            float const depthRangeLength = depthRange.GetLength();
            float const minZ = depthRange.m_begin + minDistance * depthRangeLength;
            float const maxZ = depthRange.m_begin + maxDistance * depthRangeLength;
            float const zRange = maxZ - minZ;
            float const zRatio = maxZ / minZ;

            for ( size_t split = 0; split < cascadeSplits.size(); ++split )
            {
                float const power = float( split + 1 ) / float( g_NumCascadedShadowViews );
                float const log = minZ * Math::Pow( zRatio, power );
                float const uniform = minZ + zRange * power;
                float const distance = lambda * ( log - uniform ) + uniform;
                cascadeSplits[split] = ( distance - depthRange.m_begin ) / zRange;
            }

            // Cascade fit
            //-------------------------------------------------------------------------

            renderViewProxy.StartRenderViewWrite();
            for ( uint32_t cascadeIndex = 0; cascadeIndex < g_NumCascadedShadowViews; ++cascadeIndex )
            {
                float const previousSplitDistance = cascadeIndex ? cascadeSplits[cascadeIndex - 1] : minDistance;
                float const splitDistance = cascadeSplits[cascadeIndex];

                TArray<Vector, 8> splitFrustumCorners = {};
                for ( size_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex )
                {
                    Vector const frustumRay = frustumCorners.m_points[cornerIndex + 4] - frustumCorners.m_points[cornerIndex];
                    splitFrustumCorners[cornerIndex] = frustumCorners.m_points[cornerIndex] + frustumRay * previousSplitDistance;
                    splitFrustumCorners[cornerIndex + 4] = frustumCorners.m_points[cornerIndex] + frustumRay * splitDistance;
                }

                Vector splitFrustumCenter = Vector::Zero;
                for ( Vector const& corner : splitFrustumCorners )
                {
                    splitFrustumCenter += corner;
                }
                splitFrustumCenter *= 1.0F / 8.0F;
                splitFrustumCenter.SetW1();

                float sphereRadius = 0.0F;
                for ( Vector const& corner : splitFrustumCorners )
                {
                    sphereRadius = Math::Max( sphereRadius, corner.GetDistance3( splitFrustumCenter ) );
                }

                sphereRadius = Math::Ceiling( sphereRadius * 16.0F ) / 16.0F;

                Matrix shadowProjectionMatrix = Math::CreateOrthographicProjectionMatrixOffCenter
                (
                    -sphereRadius, sphereRadius, -sphereRadius, sphereRadius,
                    -sphereRadius * 2.0F, sphereRadius * 2.0F
                );
                shadowProjectionMatrix = shadowProjectionMatrix * Matrix::ReverseZ;

                Matrix shadowViewMatrix = Math::CreateLookAtMatrix( splitFrustumCenter, splitFrustumCenter - lightDirection, viewUpDirection );
                Matrix shadowViewProjectionMatrix = shadowViewMatrix * shadowProjectionMatrix;

                // Snap the shadow map to the texel grid
                Vector const shadowOrigin = shadowViewProjectionMatrix.TransformVector4( Vector( 0.0F, 0.0F, 0.0F, 1.0F ) ) * ( shadowMapResolution * 0.5F );

                Vector roundedOffset = shadowOrigin.GetRound() - shadowOrigin;
                roundedOffset *= 2.0F / shadowMapResolution;
                roundedOffset.SetZ( 0.0F );
                roundedOffset.SetW( 0.0F );

                shadowProjectionMatrix.m_rows[3] += roundedOffset;
                shadowViewProjectionMatrix = shadowViewMatrix * shadowProjectionMatrix;

                //-------------------------------------------------------------------------

                renderViewProxy.WriteCascadedShadowRenderView( cascadeIndex, shadowViewMatrix, shadowProjectionMatrix, -sphereRadius * 2.0F, m_pRenderSystem->GetRenderSettings()->m_cascadedShadowResolution );

                //-------------------------------------------------------------------------

                Matrix const cascadeShadowMatrixInverse = ( shadowViewProjectionMatrix * textureScaleBiasMatrix ).GetInverse();

                Vector const cascadeCorner0 = globalShadowMatrix.TransformVector3( cascadeShadowMatrixInverse.TransformVector3( Vector::Zero ) );
                Vector const cascadeCorner1 = globalShadowMatrix.TransformVector3( cascadeShadowMatrixInverse.TransformVector3( Vector::One ) );

                Vector cascadeScale = Vector::One / ( cascadeCorner1 - cascadeCorner0 );
                cascadeScale.SetW0();

                Vector cascadeOffset = -cascadeCorner0;
                cascadeOffset.SetW0();

                cascadeOffset.Store( light.m_cascadeOffsets[cascadeIndex] );
                cascadeScale.Store( light.m_cascadeScales[cascadeIndex] );
            }

            renderViewProxy.SubmitRenderViewWrite();
            pLightComponent->m_lightInstanceProxy.WriteDirectionalLight( light );
        }
    }

    void RenderWorldSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            if ( pStaticMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( pStaticMeshComponent->m_meshInstanceProxies.empty() );

                pStaticMeshComponent->QueueMeshInstanceInitialize( &m_deviceRenderWorld, m_pRenderSystem->GetPlaceholderMaterial() );

                m_staticMeshComponents.Add( pStaticMeshComponent );
                m_staticMeshComponentInstanceUpdateQueue.Bind( pStaticMeshComponent, pStaticMeshComponent->GetInstanceDataUpdateSignal() );

                pStaticMeshComponent->GetInstanceDataUpdateSignal()->Send( pStaticMeshComponent );
            }
        }
        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            if ( pSkeletalMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( pSkeletalMeshComponent->m_meshInstanceProxies.empty() );
                EE_ASSERT( !pSkeletalMeshComponent->m_skinningProxy.IsValid() );

                pSkeletalMeshComponent->m_skinningProxy = m_deviceRenderWorld.AllocateSkinningInstance( pSkeletalMeshComponent->GetMesh()->GetNumBones() );

                pSkeletalMeshComponent->QueueMeshInstanceInitialize( &m_deviceRenderWorld, m_pRenderSystem->GetPlaceholderMaterial() );
                pSkeletalMeshComponent->UpdateSkinningProxy();

                m_skeletalMeshComponents.Add( pSkeletalMeshComponent );
                m_skeletalMeshComponentInstanceUpdateQueue.Bind( pSkeletalMeshComponent, pSkeletalMeshComponent->GetInstanceDataUpdateSignal() );

                pSkeletalMeshComponent->GetInstanceDataUpdateSignal()->Send( pSkeletalMeshComponent );
            }
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                pDirectionalLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocateDirectionalLight();

                if ( pDirectionalLightComponent->GetShadowed() )
                {
                    pDirectionalLightComponent->m_cascadedShadowRenderViewProxy = m_deviceRenderWorld.AllocateRenderViews( DeviceRenderViewType::CascadedShadowMap, g_NumCascadedShadowViews );

                    uint32_t const resolution = m_pRenderSystem->GetRenderSettings()->m_cascadedShadowResolution;
                    EE_ASSERT( resolution > 0 );
                    EE_ASSERT( Math::IsPowerOf2( resolution ) );

                    DeviceRenderView* const pShadowView = &pDirectionalLightComponent->m_cascadedShadowRenderViewProxy.m_renderViewHandle.m_data[0];
                    EE_ASSERT( pShadowView->m_depthTexture == nullptr );

                    RHI::TextureParameters depthParameters = {};
                    depthParameters.m_width = resolution;
                    depthParameters.m_height = resolution;
                    depthParameters.m_arrayLayers = g_NumCascadedShadowViews;
                    depthParameters.m_format = RHI::DataFormat::D16_UNorm;
                    depthParameters.m_initialState = RHI::TextureState::DepthWrite;
                    depthParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RenderTarget };
                    depthParameters.m_debugName.sprintf( "CascadedShadow Depth Array %u Cascades", g_NumCascadedShadowViews );

                    pShadowView->m_depthTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), depthParameters );

                    pDirectionalLightComponent->m_shadowMapResolution = resolution;
                }

                pDirectionalLightComponent->OnWorldTransformUpdated();

                m_directionalLightComponents.Add( pDirectionalLightComponent );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                pPointLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocatePointLight();

                if ( pPointLightComponent->GetShadowed() )
                {
                    pPointLightComponent->m_renderViewProxy = m_deviceRenderWorld.AllocateRenderViews( DeviceRenderViewType::PointShadowMap, g_NumPointShadowViews );

                    DeviceRenderView* const pShadowView = &pPointLightComponent->m_renderViewProxy.m_renderViewHandle.m_data[0];
                    EE_ASSERT( pShadowView->m_depthTexture == nullptr );

                    uint32_t const resolution = m_pRenderSystem->GetRenderSettings()->m_pointShadowResolution;
                    EE_ASSERT( resolution > 0 );
                    EE_ASSERT( Math::IsPowerOf2( resolution ) );

                    RHI::TextureParameters depthParameters = {};
                    depthParameters.m_width = resolution;
                    depthParameters.m_height = resolution;
                    depthParameters.m_arrayLayers = g_NumPointShadowViews;
                    depthParameters.m_format = RHI::DataFormat::D16_UNorm;
                    depthParameters.m_initialState = RHI::TextureState::DepthWrite;
                    depthParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RenderTarget };
                    depthParameters.m_debugName.sprintf( "PointLight Shadow Map %u", resolution );

                    pShadowView->m_depthTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), depthParameters );

                    pPointLightComponent->m_shadowMapHandle = RHI::GetTextureHandle( pShadowView->m_depthTexture, RHI::DescriptorTypeFlags::Texture, 0 );
                    pPointLightComponent->m_shadowMapResolution = resolution;
                }

                m_deviceRenderWorld.QueuePointLightInitialize( pPointLightComponent->m_lightInstanceProxy, pPointLightComponent->m_shadowMapHandle, pPointLightComponent->m_shadowMapResolution );

                pPointLightComponent->OnWorldTransformUpdated();

                m_pointLightComponents.Add( pPointLightComponent );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                pSpotLightComponent->m_lightInstanceProxy = m_deviceRenderWorld.AllocateSpotLight();

                if ( pSpotLightComponent->GetShadowed() )
                {
                    pSpotLightComponent->m_renderViewProxy = m_deviceRenderWorld.AllocateRenderViews( DeviceRenderViewType::SpotShadowMap, g_NumSpotShadowViews );

                    DeviceRenderView* const pShadowView = &pSpotLightComponent->m_renderViewProxy.m_renderViewHandle.m_data[0];
                    EE_ASSERT( pShadowView->m_depthTexture == nullptr );

                    uint32_t const resolution = m_pRenderSystem->GetRenderSettings()->m_spotShadowResolution;
                    EE_ASSERT( resolution > 0 );
                    EE_ASSERT( Math::IsPowerOf2( resolution ) );

                    RHI::TextureParameters depthParameters = {};
                    depthParameters.m_width = resolution;
                    depthParameters.m_height = resolution;
                    depthParameters.m_arrayLayers = g_NumSpotShadowViews;
                    depthParameters.m_format = RHI::DataFormat::D16_UNorm;
                    depthParameters.m_initialState = RHI::TextureState::DepthWrite;
                    depthParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Texture, RHI::DescriptorTypeFlags::RenderTarget };
                    depthParameters.m_debugName.sprintf( "SpotLight Shadow Map %u", resolution );

                    pShadowView->m_depthTexture = RHI::CreateTexture( m_pRenderSystem->GetContextRHI(), depthParameters );

                    pSpotLightComponent->m_shadowMapHandle = RHI::GetTextureHandle( pShadowView->m_depthTexture, RHI::DescriptorTypeFlags::Texture, 0 );
                    pSpotLightComponent->m_shadowMapResolution = resolution;
                }

                m_deviceRenderWorld.QueueSpotLightInitialize( pSpotLightComponent->m_lightInstanceProxy, pSpotLightComponent->m_shadowMapHandle, pSpotLightComponent->m_shadowMapResolution );

                pSpotLightComponent->OnWorldTransformUpdated();

                m_spotLightComponents.Add( pSpotLightComponent );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
        }
    }

    void RenderWorldSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            if ( pStaticMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( !pStaticMeshComponent->m_meshInstanceProxies.empty() );

                m_staticMeshComponentInstanceUpdateQueue.Unbind( pStaticMeshComponent, pStaticMeshComponent->GetInstanceDataUpdateSignal() );

                m_staticMeshComponents.Remove( pStaticMeshComponent->GetID() );

                for ( auto& meshInstanceProxyPair : pStaticMeshComponent->m_meshInstanceProxies )
                {
                    m_deviceRenderWorld.DeallocateMeshInstance( eastl::move( meshInstanceProxyPair.second ) );
                }
                pStaticMeshComponent->m_meshInstanceProxies.clear();

                m_deviceRenderWorld.DeallocateMeshInstanceRoot( eastl::move( pStaticMeshComponent->m_meshInstanceRootProxy ) );
            }
        }
        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            if ( pSkeletalMeshComponent->HasMeshResourceSet() )
            {
                EE_ASSERT( !pSkeletalMeshComponent->m_meshInstanceProxies.empty() );
                EE_ASSERT( pSkeletalMeshComponent->m_skinningProxy.IsValid() );

                m_skeletalMeshComponentInstanceUpdateQueue.Unbind( pSkeletalMeshComponent, pSkeletalMeshComponent->GetInstanceDataUpdateSignal() );

                m_skeletalMeshComponents.Remove( pSkeletalMeshComponent->GetID() );
                m_deviceRenderWorld.DeallocateSkinningInstance( eastl::move( pSkeletalMeshComponent->m_skinningProxy ) );

                for ( auto& meshInstanceProxyPair : pSkeletalMeshComponent->m_meshInstanceProxies )
                {
                    m_deviceRenderWorld.DeallocateMeshInstance( eastl::move( meshInstanceProxyPair.second ) );
                }
                pSkeletalMeshComponent->m_meshInstanceProxies.clear();

                m_deviceRenderWorld.DeallocateMeshInstanceRoot( eastl::move( pSkeletalMeshComponent->m_meshInstanceRootProxy ) );
            }
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocateRenderViews( eastl::move( pDirectionalLightComponent->m_cascadedShadowRenderViewProxy ) );
                m_deviceRenderWorld.DeallocateDirectionalLight( eastl::move( pDirectionalLightComponent->m_lightInstanceProxy ) );

                m_directionalLightComponents.Remove( pDirectionalLightComponent->GetID() );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocateRenderViews( eastl::move( pPointLightComponent->m_renderViewProxy ) );
                m_deviceRenderWorld.DeallocatePointLight( eastl::move( pPointLightComponent->m_lightInstanceProxy ) );

                pPointLightComponent->m_shadowMapHandle = RHI::g_invalidResourceHandle;
                pPointLightComponent->m_shadowMapResolution = 0;

                m_pointLightComponents.Remove( pPointLightComponent->GetID() );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_deviceRenderWorld.DeallocateRenderViews( eastl::move( pSpotLightComponent->m_renderViewProxy ) );
                m_deviceRenderWorld.DeallocateSpotLight( eastl::move( pSpotLightComponent->m_lightInstanceProxy ) );

                pSpotLightComponent->m_shadowMapHandle = RHI::g_invalidResourceHandle;
                pSpotLightComponent->m_shadowMapResolution = 0;

                m_spotLightComponents.Remove( pSpotLightComponent->GetID() );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            // Do nothing
        }
    }

    void RenderWorldSystem::UpdateDeviceResources()
    {
        EE_PROFILE_FUNCTION_RENDER();

        m_deviceRenderWorld.UpdateDeviceResources_BeforeInstanceInitialize( m_pRenderSystem );

        // InstanceUpdate StaticMesh
        //-------------------------------------------------------------------------
        {
            TEntityMessageQueue<StaticMeshComponent>::Message staticMeshComponentMessage = {};
            while ( m_staticMeshComponentInstanceUpdateQueue.Dequeue( staticMeshComponentMessage ) )
            {
                StaticMeshComponent* pStaticMeshComponent = staticMeshComponentMessage.m_pComponent;
                EE_ASSERT( pStaticMeshComponent );

                auto CopyBufferMemory = [pStaticMeshComponent] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
                {
                    pStaticMeshComponent->WriteInstanceData( { reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined ), sizeof( ShaderTypes::MeshInstanceRoot ) / sizeof( uint32_t ) } );
                };

                m_pRenderSystem->QueueBufferUpdate
                (
                    CopyBufferMemory,
                    m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
                    pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_offset * sizeof( ShaderTypes::MeshInstanceRoot ),
                    pStaticMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size * sizeof( ShaderTypes::MeshInstanceRoot )
                );
                pStaticMeshComponent->QueueMeshInstanceInitialize( &m_deviceRenderWorld, m_pRenderSystem->GetPlaceholderMaterial() );
            }

            m_staticMeshComponentInstanceUpdateQueue.ClearIgnoredComponents();
        }

        // InstanceUpdate SkeletalMesh
        //-------------------------------------------------------------------------
        {
            TEntityMessageQueue<SkeletalMeshComponent>::Message skeletalMeshComponentMessage = {};
            while ( m_skeletalMeshComponentInstanceUpdateQueue.Dequeue( skeletalMeshComponentMessage ) )
            {
                SkeletalMeshComponent* pSkeletalMeshComponent = skeletalMeshComponentMessage.m_pComponent;
                EE_ASSERT( pSkeletalMeshComponent );

                auto CopyBufferMemory = [pSkeletalMeshComponent] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
                {
                    pSkeletalMeshComponent->WriteInstanceData( { reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined ), sizeof( ShaderTypes::MeshInstanceRoot ) / sizeof( uint32_t ) } );
                };

                m_pRenderSystem->QueueBufferUpdate
                (
                    CopyBufferMemory,
                    m_deviceRenderWorld.GetMeshInstanceRootBuffer(),
                    pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_offset * sizeof( ShaderTypes::MeshInstanceRoot ),
                    pSkeletalMeshComponent->m_meshInstanceRootProxy.m_instanceHandle.m_size * sizeof( ShaderTypes::MeshInstanceRoot )
                );
                pSkeletalMeshComponent->QueueMeshInstanceInitialize( &m_deviceRenderWorld, m_pRenderSystem->GetPlaceholderMaterial() );
            }

            m_skeletalMeshComponentInstanceUpdateQueue.ClearIgnoredComponents();
        }

        m_deviceRenderWorld.UpdateDeviceResources_AfterInstanceInitialize( m_pRenderSystem );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_meshInstanceRootOutlineData.size() != m_deviceRenderWorld.GetNumMeshInstanceRootPages() )
        {
            m_meshInstanceRootOutlineData.resize( m_deviceRenderWorld.GetNumMeshInstanceRootPages(), 0 );
            m_meshInstanceRootOutlineNeedUpdate = true;
        }

        auto UpdateBuffer_MeshInstanceRootOutline = [this] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            m_meshInstanceRootOutlineNeedUpdate = true;

            RHI::BufferParameters outlineBufferParameters = {};
            outlineBufferParameters.m_bufferSize = newBufferSize;
            outlineBufferParameters.m_bufferStride = sizeof( uint64_t );
            outlineBufferParameters.m_debugName = "RenderWorldSystem MeshInstanceRoot Outline Buffer";

            RHI::Buffer* pOutlineBuffer = RHI::CreateBuffer( m_pRenderSystem->GetContextRHI(), outlineBufferParameters );

            if ( pOldBuffer )
            {
                m_pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pOutlineBuffer;
        };

        m_meshInstanceRootOutlineBuffer.UpdateDeviceResources
        (
            m_meshInstanceRootOutlineData.size() * sizeof( uint64_t ),
            UpdateBuffer_MeshInstanceRootOutline
        );

        if ( m_meshInstanceRootOutlineNeedUpdate )
        {
            EE_ASSERT( m_meshInstanceRootOutlineBuffer.m_pBuffer != nullptr );
            EE_ASSERT( m_meshInstanceRootOutlineBuffer.m_pBuffer->m_size >= m_meshInstanceRootOutlineData.size() * sizeof( uint64_t ) );

            size_t const outlineBufferSize = m_meshInstanceRootOutlineData.size() * sizeof( uint64_t );
            uint64_t const* pOutlineBits = m_meshInstanceRootOutlineData.data();

            auto CopyBufferMemory = [pOutlineBits, outlineBufferSize] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
            {
                EE_ASSERT( dstSize == outlineBufferSize );
                Memory::CopyToWriteCombined( pDstMemory_WriteCombined, pOutlineBits, outlineBufferSize );
            };

            m_pRenderSystem->QueueBufferUpdate
            (
                CopyBufferMemory,
                m_meshInstanceRootOutlineBuffer.m_pBuffer,
                0,
                outlineBufferSize
            );

            m_meshInstanceRootOutlineNeedUpdate = false;
        }
        #endif
    }

    #if EE_DEVELOPMENT_TOOLS
    RHI::BufferHandle RenderWorldSystem::GetMeshInstanceRootOutlineBufferHandle() const
    {
        return RHI::GetBufferHandle( m_meshInstanceRootOutlineBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }
    #endif
}
