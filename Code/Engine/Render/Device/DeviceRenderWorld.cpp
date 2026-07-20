#include "DeviceRenderWorld.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Profiling.h"
#include "Base/Types/Arrays.h"
#include "Base/Render/RHI.h"

#include "Engine/Render/Shaders/Renderer/WorldUpdate.esf"

//-----------------------------------------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------------------------------------

    uint32_t DeviceRenderWorld::GetSkinningTransformBufferCapacity() const
    {
        return m_skinningTransformHandleAllocator.GetCapacityInPages() * 64;
    }

    uint32_t DeviceRenderWorld::GetNumMeshInstancePages() const { return m_meshInstanceHandleAllocator.GetCapacityInPages(); }

    uint32_t DeviceRenderWorld::GetNumDirectionalLightPages() const { return m_directionalLightHandleAllocator.GetCapacityInPages(); }
    uint32_t DeviceRenderWorld::GetNumPointLightPages() const { return m_pointLightHandleAllocator.GetCapacityInPages(); }
    uint32_t DeviceRenderWorld::GetNumSpotLightPages() const { return m_spotLightHandleAllocator.GetCapacityInPages(); }

    void DeviceRenderWorld::Initialize( TaskSystem* pTaskSystem, RenderSystem* pRenderSystem )
    {
        m_pTaskSystem = pTaskSystem;

        m_meshInstanceRootHandleAllocator.Initialize( 1 );
        m_meshInstanceHandleAllocator.Initialize( 1 );
        m_skinningTransformHandleAllocator.Initialize( 1 );
        m_directionalLightHandleAllocator.Initialize( 1 );
        m_pointLightHandleAllocator.Initialize( 1 );
        m_spotLightHandleAllocator.Initialize( 1 );

        m_updatePool_MeshInstanceRoot.Initialize();
        m_updatePool_MeshInstance.Initialize();
        m_updatePool_DirectionalLight.Initialize();
        m_updatePool_PointLight.Initialize();
        m_updatePool_SpotLight.Initialize();
        m_updatePool_SkinningTransform.Initialize();

        static StringID const s_WorldUpdateShaderID = StringID( "WorldUpdate" );
        m_pWorldUpdateShader = pRenderSystem->FindComputeShader( s_WorldUpdateShaderID );

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_meshInstancePageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_directionalLightPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_pointLightPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_spotLightPageBuffers[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_initializeBuffers_MeshInstance[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_updateBuffers_MeshInstanceRoot[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_updateBuffers_MeshInstance[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_updateBuffers_DirectionalLight[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_updateBuffers_PointLight[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_updateBuffers_SpotLight[frameIndex].Initialize( pRenderSystem->GetContextRHI() );
            m_updateBuffers_SkinningTransform[frameIndex].Initialize( pRenderSystem->GetContextRHI() );

            RHI::BufferParameters constantBufferParameters = {};
            constantBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            constantBufferParameters.m_bufferSize = sizeof( ShaderTypes::WorldUpdateConstants );
            constantBufferParameters.m_bufferStride = sizeof( ShaderTypes::WorldUpdateConstants );
            constantBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            constantBufferParameters.m_descriptorTypes = RHI::DescriptorTypeFlags::ConstantBuffer;
            constantBufferParameters.m_debugName.sprintf( "DeviceRenderWorld WorldUpdate Constant Buffer %i", frameIndex );

            m_worldUpdateConstantBuffers[frameIndex] = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), constantBufferParameters );
        }

        m_meshInstanceRootBuffer.Initialize( pRenderSystem->GetContextRHI() );
        m_meshInstanceBuffer.Initialize( pRenderSystem->GetContextRHI() );
        m_directionalLightBuffer.Initialize( pRenderSystem->GetContextRHI() );
        m_pointLightBuffer.Initialize( pRenderSystem->GetContextRHI() );
        m_spotLightBuffer.Initialize( pRenderSystem->GetContextRHI() );
        m_skinningTransformBuffer.Initialize( pRenderSystem->GetContextRHI() );

        m_pPlaceholderMaterial = pRenderSystem->GetPlaceholderMaterial();
    }

    void DeviceRenderWorld::Shutdown( RenderSystem* pRenderSystem )
    {
        m_pPlaceholderMaterial = nullptr;

        EE_ASSERT( m_copyInitializeCommands_MeshInstance.GetIsComplete() );

        EE_ASSERT( m_copyUpdateCommands_MeshInstanceRoot.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_DirectionalLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_PointLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SpotLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SkinningTransform.GetIsComplete() );

        m_pTaskSystem = nullptr;

        m_meshInstanceRootHandleAllocator.Shutdown();
        m_meshInstanceHandleAllocator.Shutdown();
        m_skinningTransformHandleAllocator.Shutdown();
        m_directionalLightHandleAllocator.Shutdown();
        m_pointLightHandleAllocator.Shutdown();
        m_spotLightHandleAllocator.Shutdown();

        m_updatePool_MeshInstanceRoot.Shutdown();
        m_updatePool_MeshInstance.Shutdown();
        m_updatePool_DirectionalLight.Shutdown();
        m_updatePool_PointLight.Shutdown();
        m_updatePool_SpotLight.Shutdown();
        m_updatePool_SkinningTransform.Shutdown();

        for ( uint32_t frameIndex = 0; frameIndex < RHI::MaxPendingFrames; ++frameIndex )
        {
            m_meshInstancePageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_directionalLightPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_pointLightPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_spotLightPageBuffers[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_initializeBuffers_MeshInstance[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_MeshInstance[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_MeshInstanceRoot[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_DirectionalLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_PointLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_SpotLight[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );
            m_updateBuffers_SkinningTransform[frameIndex].Shutdown( pRenderSystem->GetContextRHI() );

            RHI::DestroyBuffer( pRenderSystem->GetContextRHI(), eastl::move( m_worldUpdateConstantBuffers[frameIndex] ) );
        }

        m_meshInstanceRootBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_meshInstanceBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_directionalLightBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_pointLightBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_spotLightBuffer.Shutdown( pRenderSystem->GetContextRHI() );
        m_skinningTransformBuffer.Shutdown( pRenderSystem->GetContextRHI() );
    }

    MeshInstanceProxy DeviceRenderWorld::AllocateMeshInstanceRoot( uint32_t num64ByteBlocks )
    {
        HandleAllocator<uint32_t>::Handle meshInstanceRootHandle = m_meshInstanceRootHandleAllocator.Allocate( num64ByteBlocks );

        size_t requiredMemoryCommited = m_meshInstanceRootHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_MeshInstanceRoot.m_memoryPool.Commit( requiredMemoryCommited );

        MeshInstanceProxy meshInstanceProxy = {};
        meshInstanceProxy.m_pTransformUpdateCounter = &m_updatePool_MeshInstanceRoot.m_counter;
        meshInstanceProxy.m_pTransformUpdateSequence = &m_updatePool_MeshInstanceRoot.m_sequence;
        meshInstanceProxy.m_pDstTransformUpdateCommands = m_updatePool_MeshInstanceRoot.m_memoryPool.GetData();
        meshInstanceProxy.m_instanceHandle = eastl::move( meshInstanceRootHandle );
        return meshInstanceProxy;
    }

    void DeviceRenderWorld::DeallocateMeshInstanceRoot( MeshInstanceProxy&& meshInstanceProxy )
    {
        m_meshInstanceRootHandleAllocator.Deallocate( eastl::move( meshInstanceProxy.m_instanceHandle ) );
        meshInstanceProxy = {};
    }

    MeshInstanceProxy DeviceRenderWorld::AllocateMeshInstance( uint32_t numInstances )
    {
        HandleAllocator<uint32_t>::Handle meshInstanceHandle = m_meshInstanceHandleAllocator.Allocate( numInstances );

        size_t requiredMemoryComitted = m_meshInstanceHandleAllocator.GetCapacityInPages() * 64;
        m_updatePool_MeshInstance.m_memoryPool.Commit( requiredMemoryComitted );

        MeshInstanceProxy meshInstanceProxy = {};
        meshInstanceProxy.m_pTransformUpdateCounter = &m_updatePool_MeshInstance.m_counter;
        meshInstanceProxy.m_pTransformUpdateSequence = &m_updatePool_MeshInstance.m_sequence;
        meshInstanceProxy.m_pDstTransformUpdateCommands = m_updatePool_MeshInstance.m_memoryPool.GetData();
        meshInstanceProxy.m_instanceHandle = eastl::move( meshInstanceHandle );
        return meshInstanceProxy;
    }

    void DeviceRenderWorld::DeallocateMeshInstance( MeshInstanceProxy&& meshInstanceProxy )
    {
        m_meshInstanceHandleAllocator.Deallocate( eastl::move( meshInstanceProxy.m_instanceHandle ) );
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

    void DeviceRenderWorld::QueueMeshInstanceInitialize
    (
        uint32_t                           instanceID,
        uint32_t                           rootInstanceID,
        Mesh const*                        pMesh,
        TArrayView<TResourcePtr<Material>> materialOverrides
    )
    {
        EE_ASSERT( rootInstanceID != ~0U );

        MeshHandle const& meshHandle = pMesh->GetMeshHandle();

        uint32_t const numSubmeshes = uint32_t( pMesh->GetNumSubmeshes() );
        for ( uint32_t submeshIdx = 0; submeshIdx < numSubmeshes; ++submeshIdx )
        {
            // Resolve material override
            Material const* pMaterial = pMesh->GetMaterial( submeshIdx );

            if ( submeshIdx < materialOverrides.size() )
            {
                TResourcePtr<Material> const& overrideMaterial = materialOverrides[submeshIdx];
                if ( overrideMaterial.IsLoaded() )
                {
                    EE_ASSERT( overrideMaterial->IsValid() );
                    pMaterial = overrideMaterial.GetPtr();
                }
            }

            if ( pMaterial == nullptr )
            {
                EE_LOG_ERROR( LogCategory::Render, "DeviceRenderWorld", "Failed to resolve material for mesh %s, reverting to placeholder", pMesh->GetResourceID().c_str() );
                pMaterial = m_pPlaceholderMaterial;
            }

            EE_ASSERT( pMaterial != nullptr );

            Mesh::Submesh const& submesh = pMesh->GetSubmesh( submeshIdx );

            Geometry const& geometry = pMesh->GetGeometry()[submesh.m_geometryIdx];

            RHI::Buffer* pClusterVertexBuffer = pMesh->GetClusterVertexBuffer( submesh.m_geometryIdx );
            RHI::Buffer* pClusterTriangleBuffer = pMesh->GetClusterTriangleBuffer( submesh.m_geometryIdx );

            ShaderTypes::MeshInstanceInitializeCommand instanceInitializeCommand = {};
            instanceInitializeCommand.m_instanceID = instanceID + submeshIdx;
            instanceInitializeCommand.m_meshIndex = uint16_t( meshHandle.m_handle.m_offset + submesh.m_geometryIdx );
            instanceInitializeCommand.m_clusterVertexBuffer = RHI::GetBufferHandle( pClusterVertexBuffer, RHI::DescriptorTypeFlags::Buffer );
            instanceInitializeCommand.m_clusterTriangleBuffer = RHI::GetBufferHandle( pClusterTriangleBuffer, RHI::DescriptorTypeFlags::Buffer );
            instanceInitializeCommand.m_lodMask = submesh.m_lodMask;
            instanceInitializeCommand.m_rootIndex = rootInstanceID;
            instanceInitializeCommand.m_meshVertexStride = geometry.GetClusterVertexStride();
            instanceInitializeCommand.m_shaderParametersOffsetIn32ByteBlocks = pMaterial->GetShaderParametersOffsetIn32ByteBlocks();

            m_initializeCommands_MeshInstance.emplace_back( eastl::move( instanceInitializeCommand ) );
        }
    }

    void DeviceRenderWorld::UpdateDeviceResources_BeforeInstanceInitialize( RenderSystem* pRenderSystem )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //---------------------------------------------------------------------------------------------------

        m_updatePool_MeshInstanceRoot.Update();
        m_updatePool_MeshInstance.Update();
        m_updatePool_DirectionalLight.Update();
        m_updatePool_PointLight.Update();
        m_updatePool_SpotLight.Update();
        m_updatePool_SkinningTransform.Update();

        //---------------------------------------------------------------------------------------------------

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

        m_updateBuffers_MeshInstanceRoot[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_MeshInstanceRoot.m_numUpdateCommands ) * sizeof( ShaderTypes::MeshInstanceTransformUpdateCommand ),
            UpdateBuffer_MeshInstanceTransformUpdate
        );

        //---------------------------------------------------------------------------------------------------

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
            transformUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::PointLightUpdateCommand );
            transformUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            transformUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld PointLight Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, transformUpdateBufferParameters );
        };

        m_updateBuffers_PointLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_PointLight.m_numUpdateCommands ) * sizeof( ShaderTypes::PointLightUpdateCommand ),
            UpdateBuffer_PointLightUpdate
        );

        auto UpdateBuffer_SpotLightUpdate = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters transformUpdateBufferParameters = {};
            transformUpdateBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            transformUpdateBufferParameters.m_bufferSize = newBufferSize;
            transformUpdateBufferParameters.m_bufferStride = sizeof( ShaderTypes::SpotLightUpdateCommand );
            transformUpdateBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            transformUpdateBufferParameters.m_debugName.sprintf( "DeviceRenderWorld SpotLight Update Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, transformUpdateBufferParameters );
        };

        m_updateBuffers_SpotLight[frameIndex].UpdateDeviceResources
        (
            Math::Max( 1U, m_updatePool_SpotLight.m_numUpdateCommands ) * sizeof( ShaderTypes::SpotLightUpdateCommand ),
            UpdateBuffer_SpotLightUpdate
        );

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstanceRoot = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters instanceRootBufferParameters = {};
            instanceRootBufferParameters.m_bufferSize = newBufferSize;
            instanceRootBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstanceRoot );
            instanceRootBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer, RHI::DescriptorTypeFlags::Raw );
            instanceRootBufferParameters.m_debugName = "DeviceRenderWorld MeshInstanceRoot Buffer";

            RHI::Buffer* pMeshInstanceRootBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), instanceRootBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pMeshInstanceRootBuffer, 0, pOldBuffer, 0, pOldBuffer->m_size );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pMeshInstanceRootBuffer;
        };

        auto UpdateBuffer_MeshInstance = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters instanceBufferParameters = {};
            instanceBufferParameters.m_bufferSize = newBufferSize;
            instanceBufferParameters.m_bufferStride = sizeof( ShaderTypes::MeshInstance );
            instanceBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            instanceBufferParameters.m_debugName = "DeviceRenderWorld MeshInstance Buffer";

            RHI::Buffer* pMeshInstanceBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), instanceBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pMeshInstanceBuffer, 0, pOldBuffer, 0, pOldBuffer->m_size );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pMeshInstanceBuffer;
        };

        m_meshInstanceRootBuffer.UpdateDeviceResources
        (
            m_meshInstanceRootHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::MeshInstanceRoot ),
            UpdateBuffer_MeshInstanceRoot
        );

        m_meshInstanceBuffer.UpdateDeviceResources
        (
            m_meshInstanceHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::MeshInstance ),
            UpdateBuffer_MeshInstance
        );

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_DirectionalLight = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters params = {};
            params.m_bufferSize = newBufferSize;
            params.m_bufferStride = sizeof( ShaderTypes::LightInstance_DirectionalLight );
            params.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            params.m_debugName = "DeviceRenderWorld DirectionalLight Buffer";

            RHI::Buffer* pBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), params );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pBuffer, 0, pOldBuffer, 0, pOldBuffer->m_size );
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
            RHI::BufferParameters params = {};
            params.m_bufferSize = newBufferSize;
            params.m_bufferStride = sizeof( ShaderTypes::LightInstance_PointLight );
            params.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            params.m_debugName = "DeviceRenderWorld PointLight Buffer";

            RHI::Buffer* pBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), params );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pBuffer, 0, pOldBuffer, 0, pOldBuffer->m_size );
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
            RHI::BufferParameters params = {};
            params.m_bufferSize = newBufferSize;
            params.m_bufferStride = sizeof( ShaderTypes::LightInstance_SpotLight );
            params.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            params.m_debugName = "DeviceRenderWorld SpotLight Buffer";

            RHI::Buffer* pBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), params );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pBuffer, 0, pOldBuffer, 0, pOldBuffer->m_size );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pBuffer;
        };

        m_spotLightBuffer.UpdateDeviceResources
        (
            m_spotLightHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::LightInstance_SpotLight ),
            UpdateBuffer_SpotLight
        );

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_SkinningTransform = [pRenderSystem] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::BufferParameters skinningTransformBufferParameters = {};
            skinningTransformBufferParameters.m_bufferSize = newBufferSize;
            skinningTransformBufferParameters.m_bufferStride = sizeof( ShaderTypes::SkinningTransform );
            skinningTransformBufferParameters.m_descriptorTypes.SetMultipleFlags( RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::RWBuffer );
            skinningTransformBufferParameters.m_debugName = "DeviceRenderWorld SkinningTransform Buffer";

            RHI::Buffer* pSkinningTransformBuffer = RHI::CreateBuffer( pRenderSystem->GetContextRHI(), skinningTransformBufferParameters );

            if ( pOldBuffer )
            {
                pRenderSystem->QueueBufferCopy( pSkinningTransformBuffer, 0, pOldBuffer, 0, pOldBuffer->m_size );
                pRenderSystem->QueueResourceDelete( eastl::move( pOldBuffer ) );
            }

            return pSkinningTransformBuffer;
        };

        m_skinningTransformBuffer.UpdateDeviceResources
        (
            m_skinningTransformHandleAllocator.GetCapacityInPages() * 64 * sizeof( ShaderTypes::SkinningTransform ),
            UpdateBuffer_SkinningTransform
        );

        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_MeshInstancePage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld MeshInstance Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        m_meshInstancePageBuffers[frameIndex].UpdateDeviceResources
        (
            m_meshInstanceHandleAllocator.GetCapacityInPages() * sizeof( uint64_t ),
            UpdateBuffer_MeshInstancePage
        );

        // Light page buffers
        //---------------------------------------------------------------------------------------------------

        auto UpdateBuffer_DirectionalLightPage = [pContextRHI, frameIndex] ( RHI::Buffer* && pOldBuffer, size_t newBufferSize )
        {
            RHI::DestroyBuffer( pContextRHI, eastl::move( pOldBuffer ) );

            RHI::BufferParameters pageBufferParameters = {};
            pageBufferParameters.m_memoryType = RHI::ResourceMemoryType::HostToDevice;
            pageBufferParameters.m_bufferSize = newBufferSize;
            pageBufferParameters.m_bufferStride = sizeof( uint64_t );
            pageBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
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
            pageBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
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
            pageBufferParameters.m_format = RHI::DataFormat::RG32_UInt;
            pageBufferParameters.m_flags = RHI::BufferFlags::PersistentMap;
            pageBufferParameters.m_debugName.sprintf( "DeviceRenderWorld SpotLight Page Buffer %i", frameIndex );

            return RHI::CreateBuffer( pContextRHI, pageBufferParameters );
        };

        m_spotLightPageBuffers[frameIndex].UpdateDeviceResources
        (
            m_spotLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t ),
            UpdateBuffer_SpotLightPage
        );

        //---------------------------------------------------------------------------------------------------

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

    }

    void DeviceRenderWorld::UpdateDeviceResources_AfterInstanceInitialize( RenderSystem* pRenderSystem )
    {
        EE_PROFILE_FUNCTION_RENDER();

        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //---------------------------------------------------------------------------------------------------

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
    }

    void DeviceRenderWorld::DispatchWorldUpdate( RHI::BufferHandle meshBuffer, RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex )
    {
        EE_ASSERT( m_copyInitializeCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_MeshInstance.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_MeshInstanceRoot.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_DirectionalLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_PointLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SpotLight.GetIsComplete() );
        EE_ASSERT( m_copyUpdateCommands_SkinningTransform.GetIsComplete() );

        //---------------------------------------------------------------------------------------------------

        EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE( pCommandBuffer, "DeviceRenderWorld Update" );

        bool const hasInitCommands = !m_initializeCommands_MeshInstance.empty();

        bool hasTransformUpdateCommands = false;

        if ( m_updatePool_MeshInstance.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_MeshInstance.m_pSrcMemory = m_updatePool_MeshInstance.m_memoryPool.GetData();
            m_copyUpdateCommands_MeshInstance.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::MeshInstanceTransformUpdateCommand*>( m_updateBuffers_MeshInstance[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_MeshInstance.m_SetSize = m_updatePool_MeshInstance.m_numUpdateCommands;
            m_copyUpdateCommands_MeshInstance.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_MeshInstance );
        }

        if ( m_updatePool_MeshInstanceRoot.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_MeshInstanceRoot.m_pSrcMemory = m_updatePool_MeshInstanceRoot.m_memoryPool.GetData();
            m_copyUpdateCommands_MeshInstanceRoot.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::MeshInstanceTransformUpdateCommand*>( m_updateBuffers_MeshInstanceRoot[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_MeshInstanceRoot.m_SetSize = m_updatePool_MeshInstanceRoot.m_numUpdateCommands;
            m_copyUpdateCommands_MeshInstanceRoot.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_MeshInstanceRoot );
        }

        if ( m_updatePool_DirectionalLight.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_DirectionalLight.m_pSrcMemory = m_updatePool_DirectionalLight.m_memoryPool.GetData();
            m_copyUpdateCommands_DirectionalLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::DirectionalLightUpdateCommand*>( m_updateBuffers_DirectionalLight[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_DirectionalLight.m_SetSize = m_updatePool_DirectionalLight.m_numUpdateCommands;
            m_copyUpdateCommands_DirectionalLight.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_DirectionalLight );
        }

        if ( m_updatePool_PointLight.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_PointLight.m_pSrcMemory = m_updatePool_PointLight.m_memoryPool.GetData();
            m_copyUpdateCommands_PointLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::PointLightUpdateCommand*>( m_updateBuffers_PointLight[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_PointLight.m_SetSize = m_updatePool_PointLight.m_numUpdateCommands;
            m_copyUpdateCommands_PointLight.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_PointLight );
        }

        if ( m_updatePool_SpotLight.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_SpotLight.m_pSrcMemory = m_updatePool_SpotLight.m_memoryPool.GetData();
            m_copyUpdateCommands_SpotLight.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::SpotLightUpdateCommand*>( m_updateBuffers_SpotLight[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_SpotLight.m_SetSize = m_updatePool_SpotLight.m_numUpdateCommands;
            m_copyUpdateCommands_SpotLight.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_SpotLight );
        }

        if ( m_updatePool_SkinningTransform.m_numUpdateCommands )
        {
            hasTransformUpdateCommands = true;

            m_copyUpdateCommands_SkinningTransform.m_pSrcMemory = m_updatePool_SkinningTransform.m_memoryPool.GetData();
            m_copyUpdateCommands_SkinningTransform.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::SkinningTransformUpdateCommand*>( m_updateBuffers_SkinningTransform[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyUpdateCommands_SkinningTransform.m_SetSize = m_updatePool_SkinningTransform.m_numUpdateCommands;
            m_copyUpdateCommands_SkinningTransform.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyUpdateCommands_SkinningTransform );
        }

        // Dispatch constants
        //---------------------------------------------------------------------------------------------------

        alignas( 32 ) ShaderTypes::WorldUpdateConstants worldUpdateConstants = {};
        worldUpdateConstants.m_numInitializeCommands_MeshInstance = uint32_t( m_initializeCommands_MeshInstance.size() );
        worldUpdateConstants.m_numUpdateCommands_MeshInstanceRoot = m_updatePool_MeshInstanceRoot.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_MeshInstance = m_updatePool_MeshInstance.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_DirectionalLight = m_updatePool_DirectionalLight.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_PointLight = m_updatePool_PointLight.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_SpotLight = m_updatePool_SpotLight.m_numUpdateCommands;
        worldUpdateConstants.m_numUpdateCommands_SkinningTransform = m_updatePool_SkinningTransform.m_numUpdateCommands;

        Memory::CopyToWriteCombined( m_worldUpdateConstantBuffers[frameIndex]->m_pMappedAddress_WriteCombined, &worldUpdateConstants, sizeof( worldUpdateConstants ) );

        // Init dispatch
        //---------------------------------------------------------------------------------------------------

        if ( hasInitCommands )
        {
            EE_ASSERT( ( m_initializeBuffers_MeshInstance[frameIndex].m_buffer->m_size / m_initializeBuffers_MeshInstance[frameIndex].m_buffer->m_stride ) >= m_initializeCommands_MeshInstance.size() );

            m_copyInitializeCommands_MeshInstance.m_pSrcMemory = m_initializeCommands_MeshInstance.data();
            m_copyInitializeCommands_MeshInstance.m_pDstMemory_WriteCombined = static_cast<ShaderTypes::MeshInstanceInitializeCommand*>( m_initializeBuffers_MeshInstance[frameIndex].m_buffer->m_pMappedAddress_WriteCombined );
            m_copyInitializeCommands_MeshInstance.m_SetSize = uint32_t( m_initializeCommands_MeshInstance.size() );
            m_copyInitializeCommands_MeshInstance.m_MinRange = 1024;

            m_pTaskSystem->ScheduleTask( &m_copyInitializeCommands_MeshInstance );

            ShaderTypes::WorldUpdateResourceTableData worldUpdateResourceTable = {};
            worldUpdateResourceTable.m_mode = 0;

            worldUpdateResourceTable.SetInitializeBuffer_MeshInstance( m_initializeBuffers_MeshInstance[frameIndex].m_buffer );

            worldUpdateResourceTable.SetMeshInstanceBuffer( m_meshInstanceBuffer.m_buffer );
            worldUpdateResourceTable.SetDirectionalLightBuffer( m_directionalLightBuffer.m_buffer );
            worldUpdateResourceTable.SetPointLightBuffer( m_pointLightBuffer.m_buffer );
            worldUpdateResourceTable.SetSpotLightBuffer( m_spotLightBuffer.m_buffer );

            RHI::CmdSetPipeline( pCommandBuffer, m_pWorldUpdateShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &worldUpdateResourceTable, sizeof( worldUpdateResourceTable ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, m_worldUpdateConstantBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( worldUpdateConstants.m_numInitializeCommands_MeshInstance + 63 ) / 64, 1, 1 );
            RHI::CmdBarrier( pCommandBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::ComputeShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::UnorderedAccess );
        }

        // Transform update dispatch
        //---------------------------------------------------------------------------------------------------

        if ( hasTransformUpdateCommands )
        {
            ShaderTypes::WorldUpdateResourceTableData worldUpdateResourceTable = {};
            worldUpdateResourceTable.m_mode = 1;

            worldUpdateResourceTable.SetUpdateBuffer_MeshInstanceRoot( m_updateBuffers_MeshInstanceRoot[frameIndex].m_buffer );
            worldUpdateResourceTable.SetUpdateBuffer_MeshInstance( m_updateBuffers_MeshInstance[frameIndex].m_buffer );
            worldUpdateResourceTable.SetUpdateBuffer_DirectionalLight( m_updateBuffers_DirectionalLight[frameIndex].m_buffer );
            worldUpdateResourceTable.SetUpdateBuffer_PointLight( m_updateBuffers_PointLight[frameIndex].m_buffer );
            worldUpdateResourceTable.SetUpdateBuffer_SpotLight( m_updateBuffers_SpotLight[frameIndex].m_buffer );
            worldUpdateResourceTable.SetUpdateBuffer_SkinningTransform( m_updateBuffers_SkinningTransform[frameIndex].m_buffer );

            worldUpdateResourceTable.SetSkinningTransformBuffer( m_skinningTransformBuffer.m_buffer );
            worldUpdateResourceTable.SetMeshInstanceRootBuffer( m_meshInstanceRootBuffer.m_buffer );
            worldUpdateResourceTable.SetMeshInstanceBuffer( m_meshInstanceBuffer.m_buffer );
            worldUpdateResourceTable.SetDirectionalLightBuffer( m_directionalLightBuffer.m_buffer );
            worldUpdateResourceTable.SetPointLightBuffer( m_pointLightBuffer.m_buffer );
            worldUpdateResourceTable.SetSpotLightBuffer( m_spotLightBuffer.m_buffer );

            uint32_t maxNumUpdateCommands = Math::Max
            (
                worldUpdateConstants.m_numUpdateCommands_DirectionalLight,
                worldUpdateConstants.m_numUpdateCommands_MeshInstance
            );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_PointLight );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_SpotLight );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_MeshInstanceRoot );
            maxNumUpdateCommands = Math::Max( maxNumUpdateCommands, worldUpdateConstants.m_numUpdateCommands_SkinningTransform );

            RHI::CmdSetPipeline( pCommandBuffer, m_pWorldUpdateShader->m_pPipeline );
            RHI::CmdSetRootConstants( pCommandBuffer, 0, &worldUpdateResourceTable, sizeof( worldUpdateResourceTable ) );
            RHI::CmdSetRootParameter( pCommandBuffer, 1, m_worldUpdateConstantBuffers[frameIndex], 0 );
            RHI::CmdDispatchCompute( pCommandBuffer, ( maxNumUpdateCommands + 63 ) / 64, 1, 1 );

            RHI::CmdBarrier( pCommandBuffer, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::AllShader, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::ShaderResource );

            m_updatePool_MeshInstanceRoot.Submit();
            m_updatePool_MeshInstance.Submit();
            m_updatePool_DirectionalLight.Submit();
            m_updatePool_PointLight.Submit();
            m_updatePool_SpotLight.Submit();
            m_updatePool_SkinningTransform.Submit();
        }

        //---------------------------------------------------------------------------------------------------

        Memory::CopyToWriteCombined
        (
            m_meshInstancePageBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined,
            m_meshInstanceHandleAllocator.GetPageData(),
            m_meshInstanceHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );

        Memory::CopyToWriteCombined
        (
            m_directionalLightPageBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined,
            m_directionalLightHandleAllocator.GetPageData(),
            m_directionalLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );

        Memory::CopyToWriteCombined
        (
            m_pointLightPageBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined,
            m_pointLightHandleAllocator.GetPageData(),
            m_pointLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );

        Memory::CopyToWriteCombined
        (
            m_spotLightPageBuffers[frameIndex].m_buffer->m_pMappedAddress_WriteCombined,
            m_spotLightHandleAllocator.GetPageData(),
            m_spotLightHandleAllocator.GetCapacityInPages() * sizeof( uint64_t )
        );
    }

    void DeviceRenderWorld::WaitForCopyTasks( RenderSystem* pRenderSystem )
    {
        uint32_t            frameIndex = pRenderSystem->GetFrameIndex();
        RHI::Context*       pContextRHI = pRenderSystem->GetContextRHI();

        //---------------------------------------------------------------------------------------------------

        m_pTaskSystem->WaitForTask( &m_copyInitializeCommands_MeshInstance );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_MeshInstanceRoot );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_MeshInstance );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_DirectionalLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_PointLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_SpotLight );
        m_pTaskSystem->WaitForTask( &m_copyUpdateCommands_SkinningTransform );

        Memory::WriteCombinedBarrier();

        m_initializeCommands_MeshInstance.clear();
    }

    RHI::BufferHandle DeviceRenderWorld::GetMeshInstancePageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_meshInstancePageBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetDirectionalLightPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_directionalLightPageBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetPointLightPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_pointLightPageBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetSpotLightPageBufferHandle( uint32_t frameIndex ) const
    {
        return RHI::GetBufferHandle( m_spotLightPageBuffers[frameIndex].m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetDirectionalLightBufferHandle() const
    {
        return RHI::GetBufferHandle( m_directionalLightBuffer.m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetPointLightBufferHandle() const
    {
        return RHI::GetBufferHandle( m_pointLightBuffer.m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetSpotLightBufferHandle() const
    {
        return RHI::GetBufferHandle( m_spotLightBuffer.m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetSkinningTransformBufferHandle() const
    {
        return RHI::GetBufferHandle( m_skinningTransformBuffer.m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetMeshInstanceBufferHandle() const
    {
        return RHI::GetBufferHandle( m_meshInstanceBuffer.m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::BufferHandle DeviceRenderWorld::GetMeshInstanceRootBufferHandle() const
    {
        return RHI::GetBufferHandle( m_meshInstanceRootBuffer.m_buffer, RHI::DescriptorTypeFlags::Buffer );
    }

    RHI::Buffer* DeviceRenderWorld::GetMeshInstanceRootBuffer() const
    {
        return m_meshInstanceRootBuffer.m_buffer;
    }
}
