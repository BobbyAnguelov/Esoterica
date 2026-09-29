#include "DeviceRenderWorld.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Profiling.h"
#include "Base/Types/Arrays.h"
#include "Base/Render/RHI.h"

#include "Engine/Render/Shaders/Renderer/WorldUpdate.esf"
#include "Engine/Render/Shaders/Renderer/ClusterToInstanceUpdate.esf"
#include "Engine/Render/Shaders/Renderer/RendererTypes.esh"

//-------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------

    template<typename T>
    void DeviceRenderWorld::UpdateCommandsPool<T>::Initialize()
    {
        m_memoryPool.Initialize( 1 );
    }

    template <typename T>
    void DeviceRenderWorld::UpdateCommandsPool<T>::Shutdown()
    {
        m_memoryPool.Shutdown();
    }

    template <typename T>
    void DeviceRenderWorld::UpdateCommandsPool<T>::Update()
    {
        EE_ASSERT( m_numUpdateCommands == 0 );

        m_numUpdateCommands = m_counter.exchange( 0 );
        m_sequence++;
    }

    template <typename T>
    void DeviceRenderWorld::UpdateCommandsPool<T>::Submit()
    {
        m_numUpdateCommands = 0;
    }

    //-------------------------------------------------------------------------

    uint32_t DeviceRenderWorld::GetSkinningTransformBufferCapacity() const
    {
        return m_skinningTransformHandleAllocator.GetCapacityInPages() * 64;
    }

    uint32_t DeviceRenderWorld::GetNumMeshInstanceRootPages() const { return m_meshInstanceRootHandleAllocator.GetCapacityInPages(); }

    uint32_t DeviceRenderWorld::GetNumDirectionalLightPages() const { return m_directionalLightHandleAllocator.GetCapacityInPages(); }
    uint32_t DeviceRenderWorld::GetNumPointLightPages() const { return m_pointLightHandleAllocator.GetCapacityInPages(); }
    uint32_t DeviceRenderWorld::GetNumSpotLightPages() const { return m_spotLightHandleAllocator.GetCapacityInPages(); }

    void DeviceRenderWorld::Initialize( TaskSystem* pTaskSystem, RenderSystem* pRenderSystem )
    {
        m_pTaskSystem = pTaskSystem;
        m_pRenderSystem = pRenderSystem;

        m_meshInstanceRootHandleAllocator.Initialize( 1 );
        m_skinningTransformHandleAllocator.Initialize( 1 );
        m_directionalLightHandleAllocator.Initialize( 1 );
        m_pointLightHandleAllocator.Initialize( 1 );
        m_spotLightHandleAllocator.Initialize( 1 );
        m_renderViewAllocator.Initialize( 1 );

        m_updatePool_MeshInstanceRoot.Initialize();
        m_updatePool_MeshInstance.Initialize();
        m_updatePool_DirectionalLight.Initialize();
        m_updatePool_PointLight.Initialize();
        m_updatePool_SpotLight.Initialize();
        m_updatePool_SkinningTransform.Initialize();
        m_updatePool_RenderView.Initialize();

        static StringID const s_WorldUpdateShaderID = StringID( "WorldUpdate" );
        m_pWorldUpdateShader = pRenderSystem->FindComputeShader( s_WorldUpdateShaderID );

        static StringID const s_ClusterToInstanceUpdateShaderID = StringID( "ClusterToInstanceUpdate" );
        m_pClusterToInstanceUpdateShader = pRenderSystem->FindComputeShader( s_ClusterToInstanceUpdateShaderID );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_meshInstanceRootPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_directionalLightPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_pointLightPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_spotLightPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_initializeBuffers_MeshInstance[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_initializeBuffers_PointLight[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_initializeBuffers_SpotLight[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_MeshInstanceRoot[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_MeshInstance[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_DirectionalLight[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_PointLight[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_SpotLight[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_SkinningTransform[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );
            m_updateBuffers_RenderView[frameIndex].Initialize( pRenderSystem->GetContextRHI(), true );

            RHI::BufferParameters constantBufferParameters = {};
            constantBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            constantBufferParameters.m_bufferSize = sizeof( ShaderTypes::WorldUpdateConstants );
            constantBufferParameters.m_bufferStride = sizeof( ShaderTypes::WorldUpdateConstants );
            constantBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            constantBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            constantBufferParameters.m_debugName.sprintf( "DeviceRenderWorld WorldUpdate Constant Buffer %i", frameIndex );

            m_worldUpdateConstantBuffers[frameIndex] = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), constantBufferParameters );
        }

        m_meshInstanceRootBuffer.Initialize( pRenderSystem->GetContextRHI(), true );
        m_directionalLightBuffer.Initialize( pRenderSystem->GetContextRHI(), true );
        m_pointLightBuffer.Initialize( pRenderSystem->GetContextRHI(), true );
        m_spotLightBuffer.Initialize( pRenderSystem->GetContextRHI(), true );
        m_skinningTransformBuffer.Initialize( pRenderSystem->GetContextRHI(), true );

        m_renderViewBuffer.Initialize( pRenderSystem->GetContextRHI(), true );

        m_meshInstanceBufferHandles.Initialize( pRenderSystem->GetContextRHI(), true );
        m_clusterToInstanceBufferHandles.Initialize( pRenderSystem->GetContextRHI(), true );

        //-------------------------------------------------------------------------

        EE_ASSERT( m_meshInstanceShaderPools.empty() );

        m_meshInstanceShaderPools.reserve( pRenderSystem->GetMaterialShaders().size() );
        for ( uint32_t shaderIndex = 0; shaderIndex < pRenderSystem->GetMaterialShaders().size(); ++shaderIndex )
        {
            m_meshInstanceShaderPools.emplace_back();
        }

        for ( MeshInstanceShaderPool& shaderPool : m_meshInstanceShaderPools )
        {
            shaderPool.m_instanceAllocator.Initialize( 1 );
            shaderPool.m_clusterAllocator.Initialize( 1 );
            shaderPool.m_instanceBuffer.Initialize( m_pRenderSystem->GetContextRHI(), true );
            shaderPool.m_clusterToInstanceBuffer.Initialize( m_pRenderSystem->GetContextRHI(), true );
            for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
            {
                shaderPool.m_instancePageBuffers[frameIndex].Initialize( m_pRenderSystem->GetContextRHI(), true );
            }
        }
    }

    void DeviceRenderWorld::Shutdown( RenderSystem* pRenderSystem )
    {
        EE_ASSERT( m_copyInitializeCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyInitializeCommands_PointLight.GetIsComplete() );
        EE_ASSERT( m_copyInitializeCommands_SpotLight.GetIsComplete() );

        EE_ASSERT( m_copyUpdateCommands_MeshInstanceRoot.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_DirectionalLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_PointLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SpotLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SkinningTransform.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_RenderView.GetIsComplete() );

        m_meshInstanceRootHandleAllocator.Shutdown();
        m_skinningTransformHandleAllocator.Shutdown();
        m_directionalLightHandleAllocator.Shutdown();
        m_pointLightHandleAllocator.Shutdown();
        m_spotLightHandleAllocator.Shutdown();
        m_renderViewAllocator.Shutdown();

        m_updatePool_MeshInstanceRoot.Shutdown();
        m_updatePool_MeshInstance.Shutdown();
        m_updatePool_DirectionalLight.Shutdown();
        m_updatePool_PointLight.Shutdown();
        m_updatePool_SpotLight.Shutdown();
        m_updatePool_SkinningTransform.Shutdown();
        m_updatePool_RenderView.Shutdown();

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_meshInstanceRootPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_directionalLightPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_pointLightPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_spotLightPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_initializeBuffers_MeshInstance[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_initializeBuffers_PointLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_initializeBuffers_SpotLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_MeshInstance[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_MeshInstanceRoot[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_DirectionalLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_PointLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_SpotLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_SkinningTransform[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_RenderView[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );

            RHI::DestroyBuffer( pRenderSystem->GetContextRHI(), eastl::move( m_worldUpdateConstantBuffers[frameIndex] ) );
        }

        m_meshInstanceRootBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_directionalLightBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_pointLightBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_spotLightBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_skinningTransformBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_renderViewBuffer.Shutdown( pRenderSystem->GetContextRHI() );

        //-------------------------------------------------------------------------

        for ( MeshInstanceShaderPool& shaderPool : m_meshInstanceShaderPools )
        {
            shaderPool.m_instanceAllocator.Shutdown();
            shaderPool.m_clusterAllocator.Shutdown();
            shaderPool.m_instanceBuffer.Shutdown( pRenderSystem->GetContextRHI() );
            shaderPool.m_clusterToInstanceBuffer.Shutdown( pRenderSystem->GetContextRHI() );

            for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
            {
                shaderPool.m_instancePageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            }
        }
        m_meshInstanceShaderPools.clear();

        m_meshInstanceBufferHandles.Shutdown( pRenderSystem->GetContextRHI() );

        m_clusterToInstanceBufferHandles.Shutdown( pRenderSystem->GetContextRHI() );

        //-------------------------------------------------------------------------

        m_pTaskSystem = nullptr;
        m_pRenderSystem = nullptr;
    }

    MeshInstanceRootProxy DeviceRenderWorld::AllocateMeshInstanceRoot()
    {
        HandleAllocator<uint32_t>::Handle meshInstanceRootHandle = m_meshInstanceRootHandleAllocator.Allocate( 1 );

        size_t requiredMemoryCommited = m_meshInstanceRootHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_MeshInstanceRoot.m_memoryPool.Commit( requiredMemoryCommited );

        MeshInstanceRootProxy meshInstanceRootProxy = {};
        meshInstanceRootProxy.m_pTransformUpdateCounter = &m_updatePool_MeshInstanceRoot.m_counter;
        meshInstanceRootProxy.m_pTransformUpdateSequence = &m_updatePool_MeshInstanceRoot.m_sequence;
        meshInstanceRootProxy.m_pDstUpdateCommands = m_updatePool_MeshInstanceRoot.m_memoryPool.GetData();
        meshInstanceRootProxy.m_instanceHandle = eastl::move( meshInstanceRootHandle );
        return meshInstanceRootProxy;
    }

    void DeviceRenderWorld::DeallocateMeshInstanceRoot( MeshInstanceRootProxy&& meshInstanceRootProxy )
    {
        m_meshInstanceRootHandleAllocator.Deallocate( eastl::move( meshInstanceRootProxy.m_instanceHandle ) );
        meshInstanceRootProxy = {};
    }

    DeviceRenderWorld::MeshInstanceShaderPool& DeviceRenderWorld::GetMeshInstanceShaderPool( uint32_t shaderIndex )
    {
        EE_ASSERT( shaderIndex < m_meshInstanceShaderPools.size() );

        return m_meshInstanceShaderPools[shaderIndex];
    }

    MeshInstanceProxy DeviceRenderWorld::AllocateMeshInstance( uint32_t shaderIndex, TArrayView<uint32_t const> numClustersPerInstance )
    {
        EE_ASSERT( !numClustersPerInstance.empty() );

        MeshInstanceShaderPool& shaderPool = GetMeshInstanceShaderPool( shaderIndex );

        uint32_t numClusters = 0;
        for ( uint32_t instanceNumClusters : numClustersPerInstance )
        {
            numClusters += instanceNumClusters;
        }

        HandleAllocator<uint32_t>::Handle meshInstanceHandle = shaderPool.m_instanceAllocator.Allocate( uint32_t( numClustersPerInstance.size() ) );
        EE_ASSERT( meshInstanceHandle.IsValid() );

        HandleAllocator<uint32_t>::Handle clustersHandle = {};
        if ( numClusters > 0 )
        {
            clustersHandle = shaderPool.m_clusterAllocator.Allocate( numClusters );
            EE_ASSERT( clustersHandle.IsValid() );
        }

        // The single update pool must cover the full instance capacity of every shader
        uint64_t totalInstanceCapacity = 0;
        for ( auto const& meshInstanceShaderPool : m_meshInstanceShaderPools )
        {
            totalInstanceCapacity += uint64_t( meshInstanceShaderPool.m_instanceAllocator.GetCapacityInPages() ) * 64;
        }
        m_updatePool_MeshInstance.m_memoryPool.Commit( totalInstanceCapacity );

        MeshInstanceProxy meshInstanceProxy = {};
        meshInstanceProxy.m_pTransformUpdateCounter = &m_updatePool_MeshInstance.m_counter;
        meshInstanceProxy.m_pTransformUpdateSequence = &m_updatePool_MeshInstance.m_sequence;
        meshInstanceProxy.m_pDstTransformUpdateCommands = m_updatePool_MeshInstance.m_memoryPool.GetData();
        meshInstanceProxy.m_instanceHandle = eastl::move( meshInstanceHandle );
        meshInstanceProxy.m_clusterHandle = eastl::move( clustersHandle );
        meshInstanceProxy.m_shaderIndex = shaderIndex;
        return meshInstanceProxy;
    }

    void DeviceRenderWorld::DeallocateMeshInstance( MeshInstanceProxy&& meshInstanceProxy )
    {
        uint32_t const shaderIndex = meshInstanceProxy.m_shaderIndex;

        if ( meshInstanceProxy.m_clusterHandle.IsValid() )
        {
            MeshInstanceShaderPool& shaderPool = GetMeshInstanceShaderPool( shaderIndex );

            shaderPool.m_clusterAllocator.Deallocate( eastl::move( meshInstanceProxy.m_clusterHandle ) );
        }

        if ( meshInstanceProxy.m_instanceHandle.IsValid() )
        {
            MeshInstanceShaderPool& shaderPool = GetMeshInstanceShaderPool( shaderIndex );
            shaderPool.m_instanceAllocator.Deallocate( eastl::move( meshInstanceProxy.m_instanceHandle ) );
        }

        meshInstanceProxy = {};
    }

    LightInstanceProxy DeviceRenderWorld::AllocateDirectionalLight()
    {
        HandleAllocator<uint32_t>::Handle lightInstanceHandle = m_directionalLightHandleAllocator.Allocate( 1 );

        size_t requiredMemoryComitted = m_directionalLightHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_DirectionalLight.m_memoryPool.Commit( requiredMemoryComitted );

        LightInstanceProxy lightInstanceProxy = {};
        lightInstanceProxy.m_pTransformUpdateCounter = &m_updatePool_DirectionalLight.m_counter;
        lightInstanceProxy.m_pTransformUpdateSequence = &m_updatePool_DirectionalLight.m_sequence;
        lightInstanceProxy.m_pDstUpdateCommands = m_updatePool_DirectionalLight.m_memoryPool.GetData();
        lightInstanceProxy.m_instanceHandle = eastl::move( lightInstanceHandle );
        return lightInstanceProxy;
    }

    LightInstanceProxy DeviceRenderWorld::AllocatePointLight()
    {
        HandleAllocator<uint32_t>::Handle lightInstanceHandle = m_pointLightHandleAllocator.Allocate( 1 );

        size_t requiredMemoryComitted = m_pointLightHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_PointLight.m_memoryPool.Commit( requiredMemoryComitted );

        LightInstanceProxy lightInstanceProxy = {};
        lightInstanceProxy.m_pTransformUpdateCounter = &m_updatePool_PointLight.m_counter;
        lightInstanceProxy.m_pTransformUpdateSequence = &m_updatePool_PointLight.m_sequence;
        lightInstanceProxy.m_pDstUpdateCommands = m_updatePool_PointLight.m_memoryPool.GetData();
        lightInstanceProxy.m_instanceHandle = eastl::move( lightInstanceHandle );
        return lightInstanceProxy;
    }

    LightInstanceProxy DeviceRenderWorld::AllocateSpotLight()
    {
        HandleAllocator<uint32_t>::Handle lightInstanceHandle = m_spotLightHandleAllocator.Allocate( 1 );

        size_t requiredMemoryComitted = m_spotLightHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_SpotLight.m_memoryPool.Commit( requiredMemoryComitted );

        LightInstanceProxy lightInstanceProxy = {};
        lightInstanceProxy.m_pTransformUpdateCounter = &m_updatePool_SpotLight.m_counter;
        lightInstanceProxy.m_pTransformUpdateSequence = &m_updatePool_SpotLight.m_sequence;
        lightInstanceProxy.m_pDstUpdateCommands = m_updatePool_SpotLight.m_memoryPool.GetData();
        lightInstanceProxy.m_instanceHandle = eastl::move( lightInstanceHandle );
        return lightInstanceProxy;
    }

    void DeviceRenderWorld::DeallocateDirectionalLight( LightInstanceProxy&& lightInstanceProxy )
    {
        m_directionalLightHandleAllocator.Deallocate( eastl::move( lightInstanceProxy.m_instanceHandle ) );
        lightInstanceProxy = {};
    }

    void DeviceRenderWorld::DeallocatePointLight( LightInstanceProxy&& lightInstanceProxy )
    {
        m_pointLightHandleAllocator.Deallocate( eastl::move( lightInstanceProxy.m_instanceHandle ) );
        lightInstanceProxy = {};
    }

    void DeviceRenderWorld::DeallocateSpotLight( LightInstanceProxy&& lightInstanceProxy )
    {
        m_spotLightHandleAllocator.Deallocate( eastl::move( lightInstanceProxy.m_instanceHandle ) );
        lightInstanceProxy = {};
    }

    SkinningProxy DeviceRenderWorld::AllocateSkinningInstance( uint32_t numBones )
    {
        HandleAllocator<uint32_t>::Handle bonesHandle = m_skinningTransformHandleAllocator.Allocate( numBones );

        size_t requredMemoryComitted = m_skinningTransformHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_SkinningTransform.m_memoryPool.Commit( requredMemoryComitted );

        SkinningProxy skinningProxy = {};
        skinningProxy.m_pTransformUpdateCounter = &m_updatePool_SkinningTransform.m_counter;
        skinningProxy.m_pTransformUpdateSequence = &m_updatePool_SkinningTransform.m_sequence;
        skinningProxy.m_pDstTransformUpdateCommands = m_updatePool_SkinningTransform.m_memoryPool.GetData();
        skinningProxy.m_bonesHandle = eastl::move( bonesHandle );
        return skinningProxy;
    }

    void DeviceRenderWorld::DeallocateSkinningInstance( SkinningProxy&& skinningProxy )
    {
        m_skinningTransformHandleAllocator.Deallocate( eastl::move( skinningProxy.m_bonesHandle ) );
        skinningProxy = {};
    }

    // Render views
    //-------------------------------------------------------------------------

    RenderViewProxy DeviceRenderWorld::AllocateRenderViews( DeviceRenderViewType type, uint32_t numViews )
    {
        EE_ASSERT( type != DeviceRenderViewType::Invalid );
        EE_ASSERT( numViews == GetRenderViewGroupSize( type ) );

        PageAllocator<DeviceRenderView, uint16_t>::Handle viewHandle = m_renderViewAllocator.Allocate( uint16_t( numViews ) );
        EE_ASSERT( viewHandle.m_handle.IsValid() );

        m_updatePool_RenderView.m_memoryPool.Commit( m_renderViewAllocator.GetCapacityInItems() );

        DeviceRenderView* const pViews = m_renderViewAllocator.GetData();
        uint16_t const groupOffset = viewHandle.m_handle.m_offset;

        for ( uint16_t viewIndex = 0; viewIndex < uint16_t( numViews ); ++viewIndex )
        {
            pViews[groupOffset + viewIndex].Initialize( m_pRenderSystem );
            pViews[groupOffset + viewIndex].m_deviceRenderViewType = type;
        }

        //-------------------------------------------------------------------------

        RenderViewProxy renderViewProxy = {};
        renderViewProxy.m_pUpdateCounter = &m_updatePool_RenderView.m_counter;
        renderViewProxy.m_pUpdateSequence = &m_updatePool_RenderView.m_sequence;
        renderViewProxy.m_pDstUpdateCommands = m_updatePool_RenderView.m_memoryPool.GetData();
        renderViewProxy.m_renderViewHandle = viewHandle;

        return renderViewProxy;
    }

    void DeviceRenderWorld::DeallocateRenderViews( RenderViewProxy&& renderViewProxy )
    {
        if ( !renderViewProxy.IsValid() )
        {
            return;
        }

        uint16_t const groupOffset = uint16_t( renderViewProxy.GetBaseRenderViewIndex() );
        uint16_t const numViews = uint16_t( renderViewProxy.GetNumRenderViews() );

        DeviceRenderView* const pViews = m_renderViewAllocator.GetData();
        for ( uint16_t viewIndex = 0; viewIndex < numViews; ++viewIndex )
        {
            pViews[groupOffset + viewIndex].Shutdown( m_pRenderSystem );
        }

        m_renderViewAllocator.Deallocate( eastl::move( renderViewProxy.m_renderViewHandle ) );
        renderViewProxy = {};
    }

    void DeviceRenderWorld::QueueMeshInstanceInitialize( MeshInstanceProxy const& meshInstanceProxy, uint32_t rootInstanceID, RHI::Buffer* pMeshBuffer, uint32_t shaderParametersOffsetIn32ByteBlocks, uint32_t numClusters, uint32_t lodMask, uint32_t instanceIndex, uint32_t clusterToInstanceBase, bool instanceHidden )
    {
        EE_ASSERT( rootInstanceID != ~0U );
        EE_ASSERT( meshInstanceProxy.m_instanceHandle.IsValid() );
        EE_ASSERT( pMeshBuffer != nullptr );
        EE_ASSERT( instanceIndex < meshInstanceProxy.m_instanceHandle.m_size );
        EE_ASSERT( clusterToInstanceBase + numClusters <= meshInstanceProxy.m_clusterHandle.m_offset + meshInstanceProxy.m_clusterHandle.m_size );

        uint32_t const instanceID = uint32_t( meshInstanceProxy.m_instanceHandle.m_offset + instanceIndex );

        //-------------------------------------------------------------------------

        ShaderTypes::MeshInstanceInitializeCommand instanceInitializeCommand = {};
        instanceInitializeCommand.m_instanceID = instanceID;
        instanceInitializeCommand.m_instanceHidden = instanceHidden;
        instanceInitializeCommand.m_meshBuffer = RHI::GetBufferHandle( pMeshBuffer, RHI::DescriptorTypeFlags::Buffer );
        instanceInitializeCommand.m_lodMask = lodMask;
        instanceInitializeCommand.m_rootIndex = rootInstanceID;
        instanceInitializeCommand.m_shaderIndex = uint16_t( meshInstanceProxy.m_shaderIndex );
        instanceInitializeCommand.m_clusterToInstanceOffset = clusterToInstanceBase;
        instanceInitializeCommand.m_shaderParametersOffsetIn32ByteBlocks = shaderParametersOffsetIn32ByteBlocks;
        instanceInitializeCommand.m_numClusters = numClusters;

        m_initializeCommands_MeshInstance.emplace_back( eastl::move( instanceInitializeCommand ) );
    }

    void DeviceRenderWorld::QueuePointLightInitialize( LightInstanceProxy const& lightInstanceProxy, uint16_t shadowMapHandle, uint32_t shadowMapResolution )
    {
        EE_ASSERT( lightInstanceProxy.IsValid() );
        if ( shadowMapResolution == 0 )
        {
            EE_ASSERT( shadowMapHandle == RHI::g_invalidResourceHandle );
        }
        else
        {
            EE_ASSERT( Math::IsPowerOf2( shadowMapResolution ) );
            EE_ASSERT( shadowMapHandle != RHI::g_invalidResourceHandle );
        }

        ShaderTypes::PointLightInitializeCommand lightInitializeCommand = {};
        lightInitializeCommand.m_instanceID = uint32_t( lightInstanceProxy.m_instanceHandle.m_offset );
        lightInitializeCommand.m_shadowMapHandle = shadowMapHandle;
        lightInitializeCommand.m_shadowResolutionBits = ( shadowMapResolution == 0 ) ? 0 : uint16_t( Math::GetMostSignificantBit( shadowMapResolution ) );

        m_initializeCommands_PointLight.emplace_back( eastl::move( lightInitializeCommand ) );
    }

    void DeviceRenderWorld::QueueSpotLightInitialize( LightInstanceProxy const& lightInstanceProxy, uint16_t shadowMapHandle, uint32_t shadowMapResolution )
    {
        EE_ASSERT( lightInstanceProxy.IsValid() );
        if ( shadowMapResolution == 0 )
        {
            EE_ASSERT( shadowMapHandle == RHI::g_invalidResourceHandle );
        }
        else
        {
            EE_ASSERT( Math::IsPowerOf2( shadowMapResolution ) );
            EE_ASSERT( shadowMapHandle != RHI::g_invalidResourceHandle );
        }

        ShaderTypes::SpotLightInitializeCommand lightInitializeCommand = {};
        lightInitializeCommand.m_instanceID = uint32_t( lightInstanceProxy.m_instanceHandle.m_offset );
        lightInitializeCommand.m_shadowMapHandle = shadowMapHandle;
        lightInitializeCommand.m_shadowResolutionBits = ( shadowMapResolution == 0 ) ? 0 : uint16_t( Math::GetMostSignificantBit( shadowMapResolution ) );

        m_initializeCommands_SpotLight.emplace_back( eastl::move( lightInitializeCommand ) );
    }

    void DeviceRenderWorld::UpdateDeviceResources_BeforeInstanceInitialize( RenderSystem* pRenderSystem )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //-------------------------------------------------------------------------

        m_updatePool_MeshInstanceRoot.Update();
        m_updatePool_MeshInstance.Update();
        m_updatePool_DirectionalLight.Update();
        m_updatePool_PointLight.Update();
        m_updatePool_SpotLight.Update();
        m_updatePool_SkinningTransform.Update();
        m_updatePool_RenderView.Update();

        //-------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstanceTransformUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters transformUpdateBufferParameters = {};
            transformUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            transformUpdateBufferParameters.m_bufferSize = newBufferSize;
            transformUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstanceTransformUpdateCommand );
            transformUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            transformUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstance Transform Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, transformUpdateBufferParameters );
        };

        m_updateBuffers_MeshInstance[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_MeshInstance.m_numUpdateCommands ) * sizeof( ShaderTypes::MeshInstanceTransformUpdateCommand ),
            UpdateBuffer_MeshInstanceTransformUpdate
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstanceRootUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters rootUpdateBufferParameters = {};
            rootUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            rootUpdateBufferParameters.m_bufferSize = newBufferSize;
            rootUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstanceRootUpdateCommand );
            rootUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            rootUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstanceRoot Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, rootUpdateBufferParameters );
        };

        m_updateBuffers_MeshInstanceRoot[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_MeshInstanceRoot.m_numUpdateCommands ) * sizeof( ShaderTypes::MeshInstanceRootUpdateCommand ),
            UpdateBuffer_MeshInstanceRootUpdate
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_DirectionalLightUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters transformUpdateBufferParameters = {};
            transformUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            transformUpdateBufferParameters.m_bufferSize = newBufferSize;
            transformUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::DirectionalLightUpdateCommand );
            transformUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            transformUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld DirectionalLight Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, transformUpdateBufferParameters );
        };

        m_updateBuffers_DirectionalLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_DirectionalLight.m_numUpdateCommands ) * sizeof( ShaderTypes::DirectionalLightUpdateCommand ),
            UpdateBuffer_DirectionalLightUpdate
        );

        auto UpdateBuffer_PointLightUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters transformUpdateBufferParameters = {};
            transformUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            transformUpdateBufferParameters.m_bufferSize = newBufferSize;
            transformUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::PointLightTransformUpdateCommand );
            transformUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            transformUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld PointLight Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, transformUpdateBufferParameters );
        };

        m_updateBuffers_PointLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_PointLight.m_numUpdateCommands ) * sizeof( ShaderTypes::PointLightTransformUpdateCommand ),
            UpdateBuffer_PointLightUpdate
        );

        auto UpdateBuffer_SpotLightUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters transformUpdateBufferParameters = {};
            transformUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            transformUpdateBufferParameters.m_bufferSize = newBufferSize;
            transformUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::SpotLightTransformUpdateCommand );
            transformUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            transformUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld SpotLight Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, transformUpdateBufferParameters );
        };

        m_updateBuffers_SpotLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_SpotLight.m_numUpdateCommands ) * sizeof( ShaderTypes::SpotLightTransformUpdateCommand ),
            UpdateBuffer_SpotLightUpdate
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstanceRoot = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters instanceRootBufferParameters = {};
            instanceRootBufferParameters.m_bufferSize = newBufferSize;
            instanceRootBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstanceRoot );
            instanceRootBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            instanceRootBufferParameters.m_debugName = "DeviceRenderWorld MeshInstanceRoot Buffer";

            RHI::Buffer* pMeshInstanceRootBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), instanceRootBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pMeshInstanceRootBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pMeshInstanceRootBuffer;
        };

        m_meshInstanceRootBuffer.UpdateDeviceResources
        (
            m_meshInstanceRootHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::MeshInstanceRoot ),
            UpdateBuffer_MeshInstanceRoot
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_DirectionalLight = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters directionalLightBufferParameters = {};
            directionalLightBufferParameters.m_bufferSize = newBufferSize;
            directionalLightBufferParameters.m_bufferStride = sizeof( ShaderTypes::LightInstance_DirectionalLight );
            directionalLightBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            directionalLightBufferParameters.m_debugName = "DeviceRenderWorld DirectionalLight Buffer";

            RHI::Buffer* pBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), directionalLightBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pBuffer;
        };

        m_directionalLightBuffer.UpdateDeviceResources
        (
            m_directionalLightHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::LightInstance_DirectionalLight ),
            UpdateBuffer_DirectionalLight
        );

        auto UpdateBuffer_PointLight = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters pointLightBufferParameters = {};
            pointLightBufferParameters.m_bufferSize = newBufferSize;
            pointLightBufferParameters.m_bufferStride = sizeof( ShaderTypes::LightInstance_PointLight );
            pointLightBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            pointLightBufferParameters.m_debugName = "DeviceRenderWorld PointLight Buffer";

            RHI::Buffer* pBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), pointLightBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pBuffer;
        };

        m_pointLightBuffer.UpdateDeviceResources
        (
            m_pointLightHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::LightInstance_PointLight ),
            UpdateBuffer_PointLight
        );

        auto UpdateBuffer_SpotLight = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters spotLightBufferParameters = {};
            spotLightBufferParameters.m_bufferSize = newBufferSize;
            spotLightBufferParameters.m_bufferStride = sizeof( ShaderTypes::LightInstance_SpotLight );
            spotLightBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            spotLightBufferParameters.m_debugName = "DeviceRenderWorld SpotLight Buffer";

            RHI::Buffer* pBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), spotLightBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pBuffer;
        };

        m_spotLightBuffer.UpdateDeviceResources
        (
            m_spotLightHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::LightInstance_SpotLight ),
            UpdateBuffer_SpotLight
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_SkinningTransform = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters skinningTransformBufferParameters = {};
            skinningTransformBufferParameters.m_bufferSize = newBufferSize;
            skinningTransformBufferParameters.m_bufferStride = sizeof( ShaderTypes::SkinningTransform );
            skinningTransformBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            skinningTransformBufferParameters.m_debugName = "DeviceRenderWorld SkinningTransform Buffer";

            RHI::Buffer* pSkinningTransformBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), skinningTransformBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pSkinningTransformBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pSkinningTransformBuffer;
        };

        m_skinningTransformBuffer.UpdateDeviceResources
        (
            m_skinningTransformHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::SkinningTransform ),
            UpdateBuffer_SkinningTransform
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstancePage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstance Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        for ( MeshInstanceShaderPool& shaderPool : m_meshInstanceShaderPools )
        {
            size_t const pageBufferSize = shaderPool.m_instanceAllocator.GetCapacityInPages() * sizeof( uint64_t );
            shaderPool.m_instancePageBuffers[frameIndex].UpdateDeviceResources( pageBufferSize, UpdateBuffer_MeshInstancePage );
        }

        auto UpdateBuffer_MeshInstanceRootPage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstanceRoot Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        m_meshInstanceRootPageBuffers[frameIndex].UpdateDeviceResources
        (
            m_meshInstanceRootHandleAllocator.GetCapacityInPages() * sizeof( uint64_t ),
            UpdateBuffer_MeshInstanceRootPage
        );

        // Light page buffers
        //-------------------------------------------------------------------------

        auto UpdateBuffer_DirectionalLightPage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld DirectionalLight Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        m_directionalLightPageBuffers[frameIndex].UpdateDeviceResources
        (
            m_directionalLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t ),
            UpdateBuffer_DirectionalLightPage
        );

        auto UpdateBuffer_PointLightPage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld PointLight Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        m_pointLightPageBuffers[frameIndex].UpdateDeviceResources
        (
            m_pointLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t ),
            UpdateBuffer_PointLightPage
        );

        auto UpdateBuffer_SpotLightPage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld SpotLight Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        m_spotLightPageBuffers[frameIndex].UpdateDeviceResources
        (
            m_spotLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t ),
            UpdateBuffer_SpotLightPage
        );

        //-------------------------------------------------------------------------

        auto UpdateBuffer_SkinningTransforms = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters skinningTransformBufferParameters = {};
            skinningTransformBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            skinningTransformBufferParameters.m_bufferSize = newBufferSize;
            skinningTransformBufferParameters.m_bufferStride = sizeof( ShaderTypes::SkinningTransform );
            skinningTransformBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            skinningTransformBufferParameters.m_debugName.sprintf( "DeviceRenderWorld SkinningTransform UpdateCommands Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, skinningTransformBufferParameters );
        };

        m_updateBuffers_SkinningTransform[frameIndex].UpdateDeviceResources
        (
            GetSkinningTransformBufferCapacity() * sizeof( ShaderTypes::SkinningTransform ),
            UpdateBuffer_SkinningTransforms
        );

        //-------------------------------------------------------------------------

        uint32_t const renderViewCapacity = m_renderViewAllocator.GetCapacityInItems();

        auto UpdateBuffer_RenderView = [this] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters renderViewBufferParameters = {};
            renderViewBufferParameters.m_bufferSize = newBufferSize;
            renderViewBufferParameters.m_bufferStride = sizeof( ShaderTypes::RenderView );
            renderViewBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
            renderViewBufferParameters.m_debugName = "DeviceRenderWorld RenderView Buffer";

            RHI::Buffer* pRenderViewBuffer = RHI::CreateBuffer( m_pRenderSystem->GetContextRHI(), renderViewBufferParameters );

            if ( pOldBuffer )
            {
                m_pRenderSystem->QueueBufferCopy( pRenderViewBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                m_pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pRenderViewBuffer;
        };

        m_renderViewBuffer.UpdateDeviceResources
        (
            Math::Max( 1ULL, size_t( renderViewCapacity ) * sizeof( ShaderTypes::RenderView ) ),
            UpdateBuffer_RenderView
        );

        auto UpdateBuffer_RenderViewUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters renderViewUpdateBufferParameters = {};
            renderViewUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            renderViewUpdateBufferParameters.m_bufferSize = newBufferSize;
            renderViewUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::RenderViewUpdateCommand );
            renderViewUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            renderViewUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld RenderView UpdateCommands Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, renderViewUpdateBufferParameters );
        };

        m_updateBuffers_RenderView[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1ULL, size_t( renderViewCapacity ) * sizeof( ShaderTypes::RenderViewUpdateCommand ) ),
            UpdateBuffer_RenderViewUpdate
        );
    }

    void DeviceRenderWorld::UpdateDeviceResources_AfterInstanceInitialize( RenderSystem* pRenderSystem )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //-------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstanceInitialize = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters instanceInitializeBufferParameters = {};
            instanceInitializeBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            instanceInitializeBufferParameters.m_bufferSize = newBufferSize;
            instanceInitializeBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstanceInitializeCommand );
            instanceInitializeBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            instanceInitializeBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstance Initialize Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, instanceInitializeBufferParameters );
        };

        m_initializeBuffers_MeshInstance[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1ULL, m_initializeCommands_MeshInstance.size() ) * sizeof( ShaderTypes::MeshInstanceInitializeCommand ),
            UpdateBuffer_MeshInstanceInitialize
        );

        auto UpdateBuffer_PointLightInitialize = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters lightInitializeBufferParameters = {};
            lightInitializeBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            lightInitializeBufferParameters.m_bufferSize = newBufferSize;
            lightInitializeBufferParameters.m_bufferStride = sizeof( ShaderTypes::PointLightInitializeCommand );
            lightInitializeBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            lightInitializeBufferParameters.m_debugName.sprintf( "DeviceRenderWorld PointLight Initialize Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, lightInitializeBufferParameters );
        };

        m_initializeBuffers_PointLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1ULL, m_initializeCommands_PointLight.size() ) * sizeof( ShaderTypes::PointLightInitializeCommand ),
            UpdateBuffer_PointLightInitialize
        );

        auto UpdateBuffer_SpotLightInitialize = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters lightInitializeBufferParameters = {};
            lightInitializeBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            lightInitializeBufferParameters.m_bufferSize = newBufferSize;
            lightInitializeBufferParameters.m_bufferStride = sizeof( ShaderTypes::SpotLightInitializeCommand );
            lightInitializeBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            lightInitializeBufferParameters.m_debugName.sprintf( "DeviceRenderWorld SpotLight Initialize Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, lightInitializeBufferParameters );
        };

        m_initializeBuffers_SpotLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1ULL, m_initializeCommands_SpotLight.size() ) * sizeof( ShaderTypes::SpotLightInitializeCommand ),
            UpdateBuffer_SpotLightInitialize
        );

        //-------------------------------------------------------------------------

        EE_ASSERT( m_meshInstanceShaderPools.size() == pRenderSystem->GetMaterialShaders().size() );

        bool needInstanceBufferHandlesUpdate = m_meshInstanceBufferHandles.m_pBuffer == nullptr;
        bool needClusterToInstanceBufferHandlesUpdate = m_clusterToInstanceBufferHandles.m_pBuffer == nullptr;

        for ( uint32_t shaderIndex = 0; shaderIndex < m_meshInstanceShaderPools.size(); ++shaderIndex )
        {
            MeshInstanceShaderPool& shaderPool = m_meshInstanceShaderPools[shaderIndex];

            uint32_t const instanceCapacity = shaderPool.m_instanceAllocator.GetCapacityInPages() * 64;
            uint32_t const clusterCapacity = shaderPool.m_clusterAllocator.GetCapacityInPages() * 64;

            //-------------------------------------------------------------------------

            auto UpdateBuffer_MeshInstance = [pRenderSystem, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                RHI::BufferParameters instanceBufferParameters = {};
                instanceBufferParameters.m_bufferSize = newBufferSize;
                instanceBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstance );
                instanceBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
                instanceBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstance Buffer %u", shaderIndex );

                RHI::Buffer* pMeshInstanceBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), instanceBufferParameters );

                if ( pOldBuffer )
                {
                    pRenderSystem->QueueBufferCopy( pMeshInstanceBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                    pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
                }

                return pMeshInstanceBuffer;
            };

            RHI::Buffer* const pOldInstanceBuffer = shaderPool.m_instanceBuffer.m_pBuffer;
            shaderPool.m_instanceBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, size_t( instanceCapacity ) * sizeof( ShaderTypes::MeshInstance ) ),
                UpdateBuffer_MeshInstance
            );
            needInstanceBufferHandlesUpdate |= shaderPool.m_instanceBuffer.m_pBuffer != pOldInstanceBuffer;

            //-------------------------------------------------------------------------

            auto UpdateBuffer_ClusterToInstance = [pRenderSystem, shaderIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                RHI::BufferParameters clusterToInstanceBufferParameters = {};
                clusterToInstanceBufferParameters.m_bufferSize = newBufferSize;
                clusterToInstanceBufferParameters.m_bufferStride = sizeof( ShaderTypes::ClusterToInstance );
                clusterToInstanceBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer };
                clusterToInstanceBufferParameters.m_debugName.sprintf( "DeviceRenderWorld ClusterToInstance Buffer %u", shaderIndex );

                RHI::Buffer* pClusterToInstanceBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), clusterToInstanceBufferParameters );

                if ( pOldBuffer )
                {
                    pRenderSystem->QueueBufferCopy( pClusterToInstanceBuffer, 0, pOldBuffer, 0, Math::Min( pOldBuffer->m_size, newBufferSize ) );
                    pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
                }

                return pClusterToInstanceBuffer;
            };

            RHI::Buffer* const pOldClusterToInstanceBuffer = shaderPool.m_clusterToInstanceBuffer.m_pBuffer;
            shaderPool.m_clusterToInstanceBuffer.UpdateDeviceResources
            (
                Math::Max( 1ULL, size_t( clusterCapacity ) * sizeof( ShaderTypes::ClusterToInstance ) ),
                UpdateBuffer_ClusterToInstance
            );
            needClusterToInstanceBufferHandlesUpdate |= shaderPool.m_clusterToInstanceBuffer.m_pBuffer != pOldClusterToInstanceBuffer;
        }

        //-------------------------------------------------------------------------

        if ( needInstanceBufferHandlesUpdate )
        {
            size_t const handlesBufferSize = m_meshInstanceShaderPools.size() * sizeof( uint32_t );

            auto UpdateBuffer = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters handleBufferParameters = {};
                handleBufferParameters.m_bufferSize = newBufferSize;
                handleBufferParameters.m_bufferStride = sizeof( uint32_t );
                handleBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
                handleBufferParameters.m_debugName = "DeviceRenderWorld MeshInstance Buffer Handles";

                return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), handleBufferParameters );
            };

            m_meshInstanceBufferHandles.UpdateDeviceResources( Math::Max( 1ULL, handlesBufferSize ), UpdateBuffer );

            RHI::Buffer* pHandlesBuffer = m_meshInstanceBufferHandles.m_pBuffer;

            auto CopyMemory = [this, handlesBufferSize] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
            {
                EA_UNUSED( handlesBufferSize );
                EE_ASSERT( dstSize == handlesBufferSize );

                uint32_t* pHandles = reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined );
                for ( uint32_t shaderIndex = 0; shaderIndex < m_meshInstanceShaderPools.size(); ++shaderIndex )
                {
                    RHI::Buffer* pInstanceBuffer = m_meshInstanceShaderPools[shaderIndex].m_instanceBuffer.m_pBuffer;
                    pHandles[shaderIndex] = pInstanceBuffer ? uint32_t( RHI::GetBufferHandle( pInstanceBuffer, RHI::DescriptorTypeFlags::RWBuffer ) ) : 0;
                }
            };

            pRenderSystem->QueueBufferUpdate( CopyMemory, pHandlesBuffer, 0, handlesBufferSize );
        }

        //-------------------------------------------------------------------------

        if ( needClusterToInstanceBufferHandlesUpdate )
        {
            size_t const handlesBufferSize = m_meshInstanceShaderPools.size() * sizeof( uint32_t );

            auto UpdateBuffer = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
            {
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );

                RHI::BufferParameters handleBufferParameters = {};
                handleBufferParameters.m_bufferSize = newBufferSize;
                handleBufferParameters.m_bufferStride = sizeof( uint32_t );
                handleBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::Buffer;
                handleBufferParameters.m_debugName = "DeviceRenderWorld ClusterToInstance Buffer Handles";

                return RHI::CreateBuffer( pRenderSystem->GetContextRHI(), handleBufferParameters );
            };

            m_clusterToInstanceBufferHandles.UpdateDeviceResources( Math::Max( 1ULL, handlesBufferSize ), UpdateBuffer );

            RHI::Buffer* pHandlesBuffer = m_clusterToInstanceBufferHandles.m_pBuffer;

            auto CopyMemory = [this, handlesBufferSize] ( uint8_t* pDstMemory_WriteCombined, size_t dstSize )
            {
                EA_UNUSED( handlesBufferSize );
                EE_ASSERT( dstSize == handlesBufferSize );

                uint32_t* pHandles = reinterpret_cast<uint32_t*>( pDstMemory_WriteCombined );
                for ( uint32_t shaderIndex = 0; shaderIndex < m_meshInstanceShaderPools.size(); ++shaderIndex )
                {
                    RHI::Buffer* pClusterToInstanceBuffer = m_meshInstanceShaderPools[shaderIndex].m_clusterToInstanceBuffer.m_pBuffer;
                    pHandles[shaderIndex] = pClusterToInstanceBuffer ? uint32_t( RHI::GetBufferHandle( pClusterToInstanceBuffer, RHI::DescriptorTypeFlags::RWBuffer ) ) : 0;
                }
            };

            pRenderSystem->QueueBufferUpdate( CopyMemory, pHandlesBuffer, 0, handlesBufferSize );
        }
    }

    RHI::Buffer* DeviceRenderWorld::GetMeshInstanceRootBuffer() const
    {
        return m_meshInstanceRootBuffer.m_pBuffer;
    }

    uint32_t DeviceRenderWorld::GetNumMeshInstanceShaderPools() const
    {
        return uint32_t( m_meshInstanceShaderPools.size() );
    }

    uint32_t DeviceRenderWorld::GetMeshInstanceCapacity( size_t shaderIndex ) const
    {
        EE_ASSERT( shaderIndex < m_meshInstanceShaderPools.size() );

        return m_meshInstanceShaderPools[shaderIndex].m_instanceAllocator.GetCapacityInPages() * 64;
    }

    uint32_t DeviceRenderWorld::GetClusterCapacity( size_t shaderIndex ) const
    {
        EE_ASSERT( shaderIndex < m_meshInstanceShaderPools.size() );

        return m_meshInstanceShaderPools[shaderIndex].m_clusterAllocator.GetCapacityInPages() * 64;
    }

    RHI::Buffer* DeviceRenderWorld::GetMeshInstancePageBuffer( uint32_t shaderIndex, uint32_t frameIndex ) const
    {
        return m_meshInstanceShaderPools[shaderIndex].m_instancePageBuffers[frameIndex].m_pBuffer;
    }

    RHI::Buffer* DeviceRenderWorld::GetMeshInstanceBuffer( uint32_t shaderIndex ) const
    {
        return m_meshInstanceShaderPools[shaderIndex].m_instanceBuffer.m_pBuffer;
    }

    RHI::Buffer* DeviceRenderWorld::GetClusterToInstanceBuffer( uint32_t shaderIndex ) const
    {
        return m_meshInstanceShaderPools[shaderIndex].m_clusterToInstanceBuffer.m_pBuffer;
    }

    void DeviceRenderWorld::DispatchWorldUpdate( RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex )
    {
        EE_ASSERT( m_copyInitializeCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyInitializeCommands_PointLight.GetIsComplete() );
        EE_ASSERT( m_copyInitializeCommands_SpotLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_MeshInstanceRoot.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_DirectionalLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_PointLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SpotLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SkinningTransform.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_RenderView.GetIsComplete() );

        //-------------------------------------------------------------------------

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "DeviceRenderWorld Update" );

        //-------------------------------------------------------------------------

        bool const hasInitCommands = !m_initializeCommands_MeshInstance.empty() || !m_initializeCommands_PointLight.empty() || !m_initializeCommands_SpotLight.empty();

        bool hasTransformUpdateCommands = false;

        if ( m_updatePool_RenderView.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            EE_ASSERT( m_updateBuffers_RenderView[frameIndex].m_pBuffer->m_size >= size_t( m_updatePool_RenderView.m_numUpdateCommands ) * sizeof( ShaderTypes::RenderViewUpdateCommand ) );

            m_copyUpdateCommands_RenderView.m_pSrcMemory = m_updatePool_RenderView.m_memoryPool.GetData();
            m_copyUpdateCommands_RenderView.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::RenderViewUpdateCommand*>( m_updateBuffers_RenderView[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_RenderView.m_SetSize = m_updatePool_RenderView.m_numUpdateCommands;
            m_copyUpdateCommands_RenderView.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_RenderView );
        }

        if ( m_updatePool_MeshInstance.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_MeshInstance.m_pSrcMemory = m_updatePool_MeshInstance.m_memoryPool.GetData();
            m_copyUpdateCommands_MeshInstance.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::MeshInstanceTransformUpdateCommand*>( m_updateBuffers_MeshInstance[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_MeshInstance.m_SetSize = m_updatePool_MeshInstance.m_numUpdateCommands;
            m_copyUpdateCommands_MeshInstance.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_MeshInstance );
        }

        if ( m_updatePool_MeshInstanceRoot.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_MeshInstanceRoot.m_pSrcMemory = m_updatePool_MeshInstanceRoot.m_memoryPool.GetData();
            m_copyUpdateCommands_MeshInstanceRoot.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::MeshInstanceRootUpdateCommand*>( m_updateBuffers_MeshInstanceRoot[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_MeshInstanceRoot.m_SetSize = m_updatePool_MeshInstanceRoot.m_numUpdateCommands;
            m_copyUpdateCommands_MeshInstanceRoot.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_MeshInstanceRoot );
        }

        if ( m_updatePool_DirectionalLight.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_DirectionalLight.m_pSrcMemory = m_updatePool_DirectionalLight.m_memoryPool.GetData();
            m_copyUpdateCommands_DirectionalLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::DirectionalLightUpdateCommand*>( m_updateBuffers_DirectionalLight[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_DirectionalLight.m_SetSize = m_updatePool_DirectionalLight.m_numUpdateCommands;
            m_copyUpdateCommands_DirectionalLight.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_DirectionalLight );
        }

        if ( m_updatePool_PointLight.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_PointLight.m_pSrcMemory = m_updatePool_PointLight.m_memoryPool.GetData();
            m_copyUpdateCommands_PointLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::PointLightTransformUpdateCommand*>( m_updateBuffers_PointLight[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_PointLight.m_SetSize = m_updatePool_PointLight.m_numUpdateCommands;
            m_copyUpdateCommands_PointLight.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_PointLight );
        }

        if ( m_updatePool_SpotLight.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_SpotLight.m_pSrcMemory = m_updatePool_SpotLight.m_memoryPool.GetData();
            m_copyUpdateCommands_SpotLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::SpotLightTransformUpdateCommand*>( m_updateBuffers_SpotLight[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_SpotLight.m_SetSize = m_updatePool_SpotLight.m_numUpdateCommands;
            m_copyUpdateCommands_SpotLight.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_SpotLight );
        }

        if ( m_updatePool_SkinningTransform.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_SkinningTransform.m_pSrcMemory = m_updatePool_SkinningTransform.m_memoryPool.GetData();
            m_copyUpdateCommands_SkinningTransform.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::SkinningTransformUpdateCommand*>( m_updateBuffers_SkinningTransform[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_SkinningTransform.m_SetSize = m_updatePool_SkinningTransform.m_numUpdateCommands;
            m_copyUpdateCommands_SkinningTransform.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_SkinningTransform );
        }

        // Dispatch constants
        //-------------------------------------------------------------------------

        alignas( 32 ) ShaderTypes::WorldUpdateConstants worldUpdateConstants = {};
        worldUpdateConstants.m_numInitializeCommands_MeshInstance = uint32_t( m_initializeCommands_MeshInstance.size() );
        worldUpdateConstants.m_numInitializeCommands_PointLight = uint32_t( m_initializeCommands_PointLight.size() );
        worldUpdateConstants.m_numInitializeCommands_SpotLight = uint32_t( m_initializeCommands_SpotLight.size() );
        worldUpdateConstants.m_numUpdateCommands_MeshInstanceRoot = m_updatePool_MeshInstanceRoot.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_MeshInstance = m_updatePool_MeshInstance.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_DirectionalLight = m_updatePool_DirectionalLight.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_PointLight = m_updatePool_PointLight.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_SpotLight = m_updatePool_SpotLight.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_SkinningTransform = m_updatePool_SkinningTransform.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_RenderView = m_updatePool_RenderView.m_numUpdateCommands;

        Memory::CopyToWriteCombined( m_worldUpdateConstantBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &worldUpdateConstants, sizeof( worldUpdateConstants ) );

        // Init dispatch
        //-------------------------------------------------------------------------

        if ( hasInitCommands )
        {
            if ( !m_initializeCommands_MeshInstance.empty() )
            {
                EE_ASSERT( ( m_initializeBuffers_MeshInstance[frameIndex].m_pBuffer->m_size / m_initializeBuffers_MeshInstance[frameIndex].m_pBuffer->m_stride ) >= m_initializeCommands_MeshInstance.size() );

                m_copyInitializeCommands_MeshInstance.m_pSrcMemory = m_initializeCommands_MeshInstance.data();
                m_copyInitializeCommands_MeshInstance.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::MeshInstanceInitializeCommand*>( m_initializeBuffers_MeshInstance[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
                m_copyInitializeCommands_MeshInstance.m_SetSize = uint32_t( m_initializeCommands_MeshInstance.size() );
                m_copyInitializeCommands_MeshInstance.m_MinRange = 1024;

                m_pTaskSystem->ScheduleTask( &m_copyInitializeCommands_MeshInstance );
            }

            if ( !m_initializeCommands_PointLight.empty() )
            {
                EE_ASSERT( ( m_initializeBuffers_PointLight[frameIndex].m_pBuffer->m_size / m_initializeBuffers_PointLight[frameIndex].m_pBuffer->m_stride ) >= m_initializeCommands_PointLight.size() );

                m_copyInitializeCommands_PointLight.m_pSrcMemory = m_initializeCommands_PointLight.data();
                m_copyInitializeCommands_PointLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::PointLightInitializeCommand*>( m_initializeBuffers_PointLight[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
                m_copyInitializeCommands_PointLight.m_SetSize = uint32_t( m_initializeCommands_PointLight.size() );
                m_copyInitializeCommands_PointLight.m_MinRange = 1024;

                m_pTaskSystem->ScheduleTask( &m_copyInitializeCommands_PointLight );
            }

            if ( !m_initializeCommands_SpotLight.empty() )
            {
                EE_ASSERT( ( m_initializeBuffers_SpotLight[frameIndex].m_pBuffer->m_size / m_initializeBuffers_SpotLight[frameIndex].m_pBuffer->m_stride ) >= m_initializeCommands_SpotLight.size() );

                m_copyInitializeCommands_SpotLight.m_pSrcMemory = m_initializeCommands_SpotLight.data();
                m_copyInitializeCommands_SpotLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::SpotLightInitializeCommand*>( m_initializeBuffers_SpotLight[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined );
                m_copyInitializeCommands_SpotLight.m_SetSize = uint32_t( m_initializeCommands_SpotLight.size() );
                m_copyInitializeCommands_SpotLight.m_MinRange = 1024;

                m_pTaskSystem->ScheduleTask( &m_copyInitializeCommands_SpotLight );
            }

            uint32_t maxNumInitializeCommands = worldUpdateConstants.m_numInitializeCommands_MeshInstance;
            maxNumInitializeCommands = Math::Max( maxNumInitializeCommands, worldUpdateConstants.m_numInitializeCommands_PointLight );
            maxNumInitializeCommands = Math::Max( maxNumInitializeCommands, worldUpdateConstants.m_numInitializeCommands_SpotLight );

            ShaderTypes::WorldUpdateResourceTableData worldUpdateResourceTable = {};
            worldUpdateResourceTable.m_mode = 0;

            worldUpdateResourceTable.SetInitializeBuffer_MeshInstance( m_initializeBuffers_MeshInstance[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetInitializeBuffer_PointLight( m_initializeBuffers_PointLight[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetInitializeBuffer_SpotLight( m_initializeBuffers_SpotLight[frameIndex].m_pBuffer );

            worldUpdateResourceTable.SetMeshInstanceBufferHandles( m_meshInstanceBufferHandles.m_pBuffer );
            worldUpdateResourceTable.SetDirectionalLightBuffer( m_directionalLightBuffer.m_pBuffer );
            worldUpdateResourceTable.SetPointLightBuffer( m_pointLightBuffer.m_pBuffer );
            worldUpdateResourceTable.SetSpotLightBuffer( m_spotLightBuffer.m_pBuffer );

            RHI::CmdSetPipeline( pCommandBuffer, m_pWorldUpdateShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &worldUpdateResourceTable, sizeof( worldUpdateResourceTable ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, m_worldUpdateConstantBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( maxNumInitializeCommands + 63 ) / 64, 1, 1 );
            RHI::CmdBarrier( pCommandBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );

            //-------------------------------------------------------------------------

            if ( !m_initializeCommands_MeshInstance.empty() )
            {
                ShaderTypes::ClusterToInstanceUpdateResourceTableData clusterToInstanceUpdateResourceTable = {};
                clusterToInstanceUpdateResourceTable.SetInitializeBuffer_MeshInstance( m_initializeBuffers_MeshInstance[frameIndex].m_pBuffer );
                clusterToInstanceUpdateResourceTable.SetClusterToInstanceBufferHandles( m_clusterToInstanceBufferHandles.m_pBuffer );

                RHI::CmdSetPipeline( pCommandBuffer, m_pClusterToInstanceUpdateShader->m_pPipeline );
                RHI::CmdSetRootConstants( pCommandBuffer, 0, &clusterToInstanceUpdateResourceTable, sizeof( clusterToInstanceUpdateResourceTable ) );
                RHI::CmdDispatchCompute( pCommandBuffer, uint32_t( m_initializeCommands_MeshInstance.size() ), 1, 1 );
            }

            RHI::CmdBarrier( pCommandBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        // Transform update dispatch
        //-------------------------------------------------------------------------

        if ( hasTransformUpdateCommands )
        {
            ShaderTypes::WorldUpdateResourceTableData worldUpdateResourceTable = {};
            worldUpdateResourceTable.m_mode = 1;

            worldUpdateResourceTable.SetUpdateBuffer_MeshInstanceRoot( m_updateBuffers_MeshInstanceRoot[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetUpdateBuffer_MeshInstance( m_updateBuffers_MeshInstance[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetUpdateBuffer_DirectionalLight( m_updateBuffers_DirectionalLight[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetUpdateBuffer_PointLight( m_updateBuffers_PointLight[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetUpdateBuffer_SpotLight( m_updateBuffers_SpotLight[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetUpdateBuffer_SkinningTransform( m_updateBuffers_SkinningTransform[frameIndex].m_pBuffer );
            worldUpdateResourceTable.SetUpdateBuffer_RenderView( m_updateBuffers_RenderView[frameIndex].m_pBuffer );

            worldUpdateResourceTable.SetSkinningTransformBuffer( m_skinningTransformBuffer.m_pBuffer );
            worldUpdateResourceTable.SetMeshInstanceRootBuffer( m_meshInstanceRootBuffer.m_pBuffer );
            worldUpdateResourceTable.SetMeshInstanceBufferHandles( m_meshInstanceBufferHandles.m_pBuffer );
            worldUpdateResourceTable.SetDirectionalLightBuffer( m_directionalLightBuffer.m_pBuffer );
            worldUpdateResourceTable.SetPointLightBuffer( m_pointLightBuffer.m_pBuffer );
            worldUpdateResourceTable.SetSpotLightBuffer( m_spotLightBuffer.m_pBuffer );
            worldUpdateResourceTable.SetRenderViewBuffer( m_renderViewBuffer.m_pBuffer );

            uint32_t maxNumUpdateCommands = Math::Max
            (
                worldUpdateConstants.m_numUpdateCommands_DirectionalLight,
                worldUpdateConstants.m_numUpdateCommands_MeshInstance
            );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_PointLight );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_SpotLight );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_MeshInstanceRoot );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_SkinningTransform );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_RenderView );

            RHI::CmdSetPipeline( pCommandBuffer, m_pWorldUpdateShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &worldUpdateResourceTable, sizeof( worldUpdateResourceTable ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, m_worldUpdateConstantBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( maxNumUpdateCommands + 63 ) / 64, 1, 1 );

            RHI::CmdBarrier( pCommandBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );
        }

        if ( hasTransformUpdateCommands )
        {
            m_updatePool_MeshInstanceRoot.Submit();
            m_updatePool_MeshInstance.Submit();
            m_updatePool_DirectionalLight.Submit();
            m_updatePool_PointLight.Submit();
            m_updatePool_SpotLight.Submit();
            m_updatePool_SkinningTransform.Submit();
            m_updatePool_RenderView.Submit();
        }

        //-------------------------------------------------------------------------

        for ( MeshInstanceShaderPool& shaderPool : m_meshInstanceShaderPools )
        {
            size_t const pageBufferSize = shaderPool.m_instanceAllocator.GetCapacityInPages() * sizeof( uint64_t );

            Memory::CopyToWriteCombined
            (
                shaderPool.m_instancePageBuffers[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined,
                shaderPool.m_instanceAllocator.GetPageData(),
                pageBufferSize
            );
        }

        Memory::CopyToWriteCombined
        (
            m_meshInstanceRootPageBuffers[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined,
            m_meshInstanceRootHandleAllocator.GetPageData(),
            m_meshInstanceRootHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );

        Memory::CopyToWriteCombined
        (
            m_directionalLightPageBuffers[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined,
            m_directionalLightHandleAllocator.GetPageData(),
            m_directionalLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );

        Memory::CopyToWriteCombined
        (
            m_pointLightPageBuffers[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined,
            m_pointLightHandleAllocator.GetPageData(),
            m_pointLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );

        Memory::CopyToWriteCombined
        (
            m_spotLightPageBuffers[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined,
            m_spotLightHandleAllocator.GetPageData(),
            m_spotLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );
    }

    void DeviceRenderWorld::WaitForCopyTasks( RenderSystem* pRenderSystem )
    {
        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //-------------------------------------------------------------------------

        m_pTaskSystem->WaitForTask( &m_copyInitializeCommands_MeshInstance );
        m_pTaskSystem->WaitForTask( &m_copyInitializeCommands_PointLight );
        m_pTaskSystem->WaitForTask( &m_copyInitializeCommands_SpotLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_MeshInstanceRoot );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_MeshInstance );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_DirectionalLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_PointLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_SpotLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_SkinningTransform );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_RenderView );

        Memory::WriteCombinedBarrier();

        m_initializeCommands_MeshInstance.clear();
        m_initializeCommands_PointLight.clear();
        m_initializeCommands_SpotLight.clear();
    }

    RHI::Buffer* DeviceRenderWorld::GetRenderViewBuffer() const
    {
        return m_renderViewBuffer.m_pBuffer;
    }

    RHI::BufferHandle DeviceRenderWorld::GetRenderViewBufferHandle() const
    {
        return RHI::GetBufferHandle( m_renderViewBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetMeshInstanceRootPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_meshInstanceRootPageBuffers[frameIndex].m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetDirectionalLightPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_directionalLightPageBuffers[frameIndex].m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetPointLightPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_pointLightPageBuffers[frameIndex].m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetSpotLightPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_spotLightPageBuffers[frameIndex].m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetDirectionalLightBufferHandle() const
    {
        return RHI::GetBufferHandle( m_directionalLightBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetPointLightBufferHandle() const
    {
        return RHI::GetBufferHandle( m_pointLightBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetSpotLightBufferHandle() const
    {
        return RHI::GetBufferHandle( m_spotLightBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetSkinningTransformBufferHandle() const
    {
        return RHI::GetBufferHandle( m_skinningTransformBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetMeshInstanceRootBufferHandle() const
    {
        return RHI::GetBufferHandle( m_meshInstanceRootBuffer.m_pBuffer, RHI::DescriptorTypeFlags::Buffer );
    }
}
