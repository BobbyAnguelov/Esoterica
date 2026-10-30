#include "ResourceLoader_RenderMesh.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/ProceduralBones/ProceduralBoneSolver.h"
#include "Engine/Render/Shaders/MeshData.esh"
#include "Engine/Render/Shaders/DecalInstance.esh"
#include "Base/TypeSystem/TypeDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static void FillMeshDecalMaterial( ShaderTypes::MeshDecal& dstDecal, DecalMaterial const& material, Mesh::MeshDecal const& meshDecal )
    {
        // Full extent of the decal face in mesh space, used for tiling and parallax
        Float3 const worldSize = Float3( meshDecal.m_localTransform.GetScale() ) * meshDecal.m_nonUniformScale;
        dstDecal.m_worldSize[0] = Math::Max( worldSize.m_x, 0.002F );
        dstDecal.m_worldSize[1] = Math::Max( worldSize.m_y, 0.002F );

        dstDecal.m_albedoTexture = RHI::GetTextureHandle( material.GetAlbedoTexture()->GetTexture(), RHI::DescriptorTypeFlags::Texture, 0 );
        dstDecal.m_pbrTexture = RHI::GetTextureHandle( material.GetPBRTexture()->GetTexture(), RHI::DescriptorTypeFlags::Texture, 0 );
        dstDecal.m_normalTexture = RHI::GetTextureHandle( material.GetNormalTexture()->GetTexture(), RHI::DescriptorTypeFlags::Texture, 0 );
        dstDecal.m_emissiveTexture = RHI::GetTextureHandle( material.GetEmissiveTexture()->GetTexture(), RHI::DescriptorTypeFlags::Texture, 0 );
        dstDecal.m_parallaxTexture = RHI::GetTextureHandle( material.GetParallaxTexture()->GetTexture(), RHI::DescriptorTypeFlags::Texture, 0 );

        dstDecal.m_tiling = Math::FloatToHalf( material.GetTiling() );
        dstDecal.m_flags = material.GetFlagsMask();
        dstDecal.m_shapeRadius = uint32_t( Math::Clamp( material.GetShapeRadius(), 0.0F, 1.0F ) * 255.0F + 0.5F );

        float const heightOffset = Math::Clamp( material.GetHeightOffset(), -1.0F, 1.0F );
        dstDecal.m_heightOffset = 1U + uint32_t( ( heightOffset * 0.5F + 0.5F ) * 254.0F + 0.5F );

        Color const& tint = material.GetTint();
        dstDecal.m_packedTintedColor = ( tint.ToUInt32() & 0x00FFFFFF ) | ( ( uint32_t( Math::Clamp( material.GetOpacity(), 0.0F, 1.0F ) * 255.0F + 0.5F ) & 0xFF ) << 24 );
        dstDecal.m_packedEmissiveColor = material.GetEmissiveColor().ToLinear().ToUInt32();
    }

    //-------------------------------------------------------------------------

    MeshLoader::MeshLoader()
    {
        m_loadableTypes.push_back( StaticMesh::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( SkeletalMesh::GetStaticResourceTypeID() );
    }

    Resource::LoadResult MeshLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        Mesh* pMeshResource = pResourceRecord->GetResourceData<Mesh>();

        // Create Mesh
        //-------------------------------------------------------------------------

        if ( pMeshResource == nullptr )
        {
            // Static Mesh
            if ( resourceID.GetResourceTypeID() == StaticMesh::GetStaticResourceTypeID() )
            {
                StaticMesh* pStaticMesh = EE::New<StaticMesh>();
                ( *pArchive ) << *pStaticMesh;
                pMeshResource = pStaticMesh;

                #if EE_DEVELOPMENT_TOOLS
                ( *pArchive ) << pStaticMesh->m_statisticsPerLOD;
                #endif
            }
            else // Skeletal Mesh
            {
                SkeletalMesh* pSkeletalMesh = EE::New<SkeletalMesh>();
                ( *pArchive ) << *pSkeletalMesh;

                #if EE_DEVELOPMENT_TOOLS
                ( *pArchive ) << pSkeletalMesh->m_statisticsPerLOD;
                #endif

                bool hasProceduralBones = false;
                ( *pArchive ) << hasProceduralBones;

                // Create procedural bone solver
                if ( hasProceduralBones )
                {
                    LoadAndCreateProceduralBoneSettings( pSkeletalMesh, pArchive );
                }

                pMeshResource = pSkeletalMesh;
            }

            EE_ASSERT( pMeshResource->IsValid() );

            //-------------------------------------------------------------------------

            // The mesh buffers are created during install, once the decal material tables are filled in
            EE_ASSERT( pMeshResource->m_meshBuffers.empty() );
            EE_ASSERT( pMeshResource->m_meshBuffersState.empty() );

            //-------------------------------------------------------------------------

            pResourceRecord->SetResourceData( pMeshResource );
        }

        EE_ASSERT( pMeshResource != nullptr );

        return Resource::LoadResult::Complete;
    }

    void MeshLoader::LoadAndCreateProceduralBoneSettings( SkeletalMesh* pSkeletalMesh, Serialization::BinaryInputArchive* pArchive ) const
    {
        ProceduralBoneSettings* pBoneSettings = EE::New<ProceduralBoneSettings>();
        pBoneSettings->m_pMesh = pSkeletalMesh;

        // Get type descriptors and create settings
        TypeSystem::TypeDescriptorCollection typeDescriptors;
        ( *pArchive ) << typeDescriptors;
        typeDescriptors.CalculateCollectionRequirements( *m_pTypeRegistry );
        TypeSystem::TypeDescriptorCollection::InstantiateStaticCollection( *m_pTypeRegistry, typeDescriptors, pBoneSettings->m_constraintSettings );

        // Calculate runtime requirements
        size_t memoryOffset = 0;
        for ( auto pConstraintSettings : pBoneSettings->m_constraintSettings )
        {
            size_t memoryNeeded = 0, alignmentNeeded = 0;
            pConstraintSettings->GetMemoryRequirements( memoryNeeded, alignmentNeeded );

            pBoneSettings->m_constraintsRequiredAlignment = Math::Max( pBoneSettings->m_constraintsRequiredAlignment, alignmentNeeded );
            size_t const requiredPadding = Memory::CalculatePaddingForAlignment( memoryOffset, alignmentNeeded );
            pBoneSettings->m_constraintStartOffsets.emplace_back( uint32_t( memoryOffset + requiredPadding ) );

            memoryOffset += ( memoryNeeded + requiredPadding );
        }

        pBoneSettings->m_constraintsRequiredMemory = memoryOffset;

        pSkeletalMesh->m_pProceduralBoneSettings = pBoneSettings;
    }

    Resource::UnloadResult MeshLoader::Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        bool isEverythingUnloaded = true;

        //-------------------------------------------------------------------------

        auto pMeshResource = pResourceRecord->GetResourceData<Mesh>();
        if ( pMeshResource != nullptr )
        {
            auto UnloadBuffer = [this, &isEverythingUnloaded] ( AsyncBufferUpdate*& pAsyncBufferUpdate, RHI::Buffer*& pResourceBuffer )
            {
                // Cancel buffer allocation in-flight
                if ( pAsyncBufferUpdate != nullptr )
                {
                    AsyncResourceUpdateState updateState = pAsyncBufferUpdate->m_updateState.load();
                    switch ( updateState )
                    {
                        case AsyncResourceUpdateState::UpdatePending:
                        {
                            EE_ASSERT( pResourceBuffer == nullptr );
                            EE_ASSERT( pAsyncBufferUpdate->m_pDstBuffer != nullptr );

                            pResourceBuffer = pAsyncBufferUpdate->m_pDstBuffer;
                            memset( pAsyncBufferUpdate->m_pDstMemory_WriteCombined, 0, pAsyncBufferUpdate->m_dstSize );
                            pAsyncBufferUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );

                            isEverythingUnloaded = false;
                        }
                        break;

                        case AsyncResourceUpdateState::CompletePending:
                        {
                            EE_ASSERT( pResourceBuffer != nullptr );
                            m_pRenderSystem->QueueResourceDelete( eastl::move( pResourceBuffer ) );
                            pAsyncBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                            pAsyncBufferUpdate = nullptr;
                        }
                        break;

                        default:
                        {
                            isEverythingUnloaded = false;
                        }
                        break;
                    }
                }
                else // Delete allocated buffer
                {
                    if ( pResourceBuffer != nullptr )
                    {
                        m_pRenderSystem->QueueResourceDelete( eastl::move( pResourceBuffer ) );
                        EE_ASSERT( pResourceBuffer == nullptr );
                    }
                }
            };

            //-------------------------------------------------------------------------

            EE_ASSERT( pMeshResource->m_meshBuffers.size() == pMeshResource->GetGeometry().size() );
            EE_ASSERT( pMeshResource->m_meshBuffersState.size() == pMeshResource->GetGeometry().size() );

            for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
            {
                Mesh::ResourceUpdateState& meshUpdateState = pMeshResource->m_meshBuffersState[geometryIdx];

                UnloadBuffer( meshUpdateState.m_pMeshBufferUpdate, pMeshResource->m_meshBuffers[geometryIdx] );
            }
        }

        //-------------------------------------------------------------------------

        if ( !isEverythingUnloaded )
        {
            return Resource::UnloadResult::InProgress;
        }

        pMeshResource->m_meshBuffers.clear();
        pMeshResource->m_meshBuffersState.clear();

        return Resource::UnloadResult::Complete;
    }

    Resource::UnloadResult MeshLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        // Destroy Procedural Bone Settings
        //-------------------------------------------------------------------------

        if ( resourceID.GetResourceTypeID() == SkeletalMesh::GetStaticResourceTypeID() )
        {
            SkeletalMesh* pSkeletalMesh = pResourceRecord->GetResourceData<SkeletalMesh>();
            if ( pSkeletalMesh != nullptr && pSkeletalMesh->HasProceduralBones() )
            {
                TypeSystem::TypeDescriptorCollection::DestroyStaticCollection( pSkeletalMesh->m_pProceduralBoneSettings->m_constraintSettings );
                EE::Delete( pSkeletalMesh->m_pProceduralBoneSettings );
            }
        }

        return Resource::UnloadResult::Complete;
    }

    Resource::LoadResult MeshLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        Mesh* pMeshResource = pResourceRecord->GetResourceData<Mesh>();
        EE_ASSERT( pMeshResource != nullptr );

        //-------------------------------------------------------------------------

        if ( pMeshResource->m_meshBuffersState.empty() )
        {
            for ( Mesh::Submesh& submesh : pMeshResource->m_submeshes )
            {
                if ( submesh.m_material.IsSet() )
                {
                    submesh.m_material = GetInstallDependency( installDependencies, submesh.m_material.GetResourceID() );
                }
            }

            for ( Mesh::MeshDecal& meshDecal : pMeshResource->m_meshDecals )
            {
                if ( meshDecal.m_material.IsSet() )
                {
                    meshDecal.m_material = GetInstallDependency( installDependencies, meshDecal.m_material.GetResourceID() );
                }
            }

            // Fill the decal material tables before the geometry data is uploaded
            for ( Geometry& geometry : pMeshResource->m_geometry )
            {
                MeshHeader const& meshHeader = geometry.GetMeshHeader();
                if ( meshHeader.m_meshDecalOffset == 0 )
                {
                    continue;
                }

                ShaderTypes::MeshDecalData const& meshDecalData = *reinterpret_cast<ShaderTypes::MeshDecalData const*>( geometry.GetMeshData().data() + meshHeader.m_meshDecalOffset );
                ShaderTypes::MeshDecal* pDstDecals = reinterpret_cast<ShaderTypes::MeshDecal*>( geometry.GetMeshData().data() + meshDecalData.m_decalTableOffset );

                for ( size_t decalIndex = 0; decalIndex < pMeshResource->m_meshDecals.size(); ++decalIndex )
                {
                    Mesh::MeshDecal const& meshDecal = pMeshResource->m_meshDecals[decalIndex];
                    ShaderTypes::MeshDecal& dstDecal = pDstDecals[decalIndex];
                    dstDecal = {};

                    DecalMaterial const* pMaterial = meshDecal.m_material.IsLoaded() ? meshDecal.m_material.GetPtr() : nullptr;
                    if ( pMaterial != nullptr && pMaterial->IsValid() )
                    {
                        FillMeshDecalMaterial( dstDecal, *pMaterial, meshDecal );
                    }
                }
            }

            EE_ASSERT( pMeshResource->m_meshBuffers.empty() );

            pMeshResource->m_meshBuffers.resize( pMeshResource->GetGeometry().size() );
            pMeshResource->m_meshBuffersState.reserve( pMeshResource->GetGeometry().size() );

            for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->m_geometry.size(); ++geometryIdx )
            {
                Geometry& geometry = pMeshResource->m_geometry[geometryIdx];

                RHI::BufferParameters meshBufferParameters = {};
                meshBufferParameters.m_bufferSize = geometry.GetMeshData().size();
                meshBufferParameters.m_descriptorTypes = { RHI::DescriptorTypeFlags::Buffer, RHI::DescriptorTypeFlags::Raw };
                meshBufferParameters.m_debugName.sprintf( "MeshData %s", resourceID.c_str() );

                pMeshResource->m_meshBuffersState.emplace_back( Mesh::ResourceUpdateState{ m_pRenderSystem->CreateBufferAsync( meshBufferParameters ) } );
            }
        }

        //-------------------------------------------------------------------------

        bool everythingLoaded = true;

        for ( size_t geometryIndex = 0; geometryIndex < pMeshResource->GetGeometry().size(); ++geometryIndex )
        {
            Geometry const& geometry = pMeshResource->GetGeometry()[geometryIndex];
            Mesh::ResourceUpdateState& meshUpdateState = pMeshResource->m_meshBuffersState[geometryIndex];

            if ( meshUpdateState.m_pMeshBufferUpdate != nullptr )
            {
                AsyncResourceUpdateState const updateState = meshUpdateState.m_pMeshBufferUpdate->m_updateState.load();

                switch ( updateState )
                {
                    case AsyncResourceUpdateState::UpdatePending:
                    {
                        pMeshResource->m_meshBuffers[geometryIndex] = meshUpdateState.m_pMeshBufferUpdate->m_pDstBuffer;

                        Memory::CopyToWriteCombined( meshUpdateState.m_pMeshBufferUpdate->m_pDstMemory_WriteCombined, geometry.GetMeshData().data(), meshUpdateState.m_pMeshBufferUpdate->m_dstSize );

                        meshUpdateState.m_pMeshBufferUpdate->m_updateState.store( AsyncResourceUpdateState::SubmitPending );
                        everythingLoaded = false;
                    }
                    break;

                    case AsyncResourceUpdateState::CompletePending:
                    {
                        // Uploaded
                    }
                    break;

                    default:
                    {
                        everythingLoaded = false;
                    }
                    break;
                }
            }
        }

        if ( !everythingLoaded )
        {
            return Resource::LoadResult::InProgress;
        }

        // Release buffer updates
        //-------------------------------------------------------------------------

        for ( size_t geometryIdx = 0; geometryIdx < pMeshResource->GetGeometry().size(); ++geometryIdx )
        {
            Mesh::ResourceUpdateState& bufferUpdateState = pMeshResource->m_meshBuffersState[geometryIdx];

            if ( bufferUpdateState.m_pMeshBufferUpdate != nullptr )
            {
                bufferUpdateState.m_pMeshBufferUpdate->m_updateState.store( AsyncResourceUpdateState::Completed );
                bufferUpdateState.m_pMeshBufferUpdate = nullptr;
            }
        }

        return Resource::LoadResult::Complete;
    }
}
