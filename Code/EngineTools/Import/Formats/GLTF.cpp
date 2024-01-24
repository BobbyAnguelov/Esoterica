#include "GLTF.h"
#include "EngineTools/Import/ImportedSkeleton.h"
#include "EngineTools/Import/ImportedAnimation.h"
#include "EngineTools/Import/ImportedMesh.h"

#define CGLTF_IMPLEMENTATION
#include "EngineTools/ThirdParty/cgltf/cgltf.h"

//-------------------------------------------------------------------------
// Scene Context
//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    static char const* const g_errorStrings[] =
    {
        ""
        "data_too_short",
        "unknown_format",
        "invalid_json",
        "invalid_gltf",
        "invalid_options",
        "file_not_found",
        "io_error",
        "out_of_memory",
        "legacy_gltf",
    };

    //-------------------------------------------------------------------------

    SceneContext::SceneContext( FileSystem::Path const& filePath, float additionalScalingFactor )
    {
        Initialize( filePath, additionalScalingFactor );
    }

    SceneContext::~SceneContext()
    {
        Shutdown();
    }

    //-------------------------------------------------------------------------

    void SceneContext::LoadFile( FileSystem::Path const& filePath, float additionalScalingFactor )
    {
        Shutdown();
        Initialize( filePath, additionalScalingFactor );
    }

    //-------------------------------------------------------------------------

    void SceneContext::Initialize( FileSystem::Path const& filePath, float additionalScalingFactor )
    {
        cgltf_options options = { cgltf_file_type_invalid, 0 };

        // Parse gltf/glb files
        //-------------------------------------------------------------------------

        cgltf_result const parseResult = cgltf_parse_file( &options, filePath.c_str(), &m_pSceneData );
        if ( parseResult != cgltf_result_success )
        {
            m_error.sprintf( "Failed to load specified gltf file ( %s ) : %s", filePath.c_str(), g_errorStrings[parseResult] );
            return;
        }

        // Load all data buffers
        //-------------------------------------------------------------------------

        cgltf_result bufferLoadResult = cgltf_load_buffers( &options, m_pSceneData, filePath.c_str() );
        if ( bufferLoadResult != cgltf_result_success )
        {
            m_error.sprintf( "Failed to load gltf file buffers ( %s ) : %s", filePath.c_str(), g_errorStrings[parseResult] );
            return;
        }

        m_scaleConversionMultiplier *= additionalScalingFactor;
    }

    void SceneContext::Shutdown()
    {
        if ( m_pSceneData != nullptr )
        {
            cgltf_free( m_pSceneData );
            m_pSceneData = nullptr;
        }

        m_scaleConversionMultiplier = 1.0f;
    }
}

//-------------------------------------------------------------------------
// Skeleton
//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    class gltfImportedSkeleton : public ImportedSkeleton
    {
        friend class gltfSkeletonFileReader;
        friend class gltfMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class gltfSkeletonFileReader
    {

    public:

        static TUniquePtr<ImportedSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<ImportedSkeleton> pSkeleton( EE::New<gltfImportedSkeleton>() );
            gltfImportedSkeleton* pImportedSkeleton = (gltfImportedSkeleton*) pSkeleton.get();
            pImportedSkeleton->m_sourcePath = sourceFilePath;

            //-------------------------------------------------------------------------

            gltf::SceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                ReadSkeleton( sceneCtx, skeletonRootBoneName, *pImportedSkeleton );
            }
            else
            {
                pImportedSkeleton->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pSkeleton;
        }

        static void ReadSkeleton( gltf::SceneContext const& sceneCtx, String const& skeletonRootBoneName, gltfImportedSkeleton& ImportedSkeleton )
        {
            auto pSceneData = sceneCtx.GetSceneData();

            if ( pSceneData->skins_count == 0 )
            {
                return;
            }

            cgltf_node* pSkeletonToRead = nullptr;
            if ( !skeletonRootBoneName.empty() )
            {
                for ( int32_t i = 0; i < pSceneData->skins_count; i++ )
                {
                    if ( pSceneData->skins[i].joints_count > 0 && pSceneData->skins[i].joints[0]->name == skeletonRootBoneName )
                    {
                        pSkeletonToRead = pSceneData->skins[i].joints[0];
                        break;
                    }
                }
            }
            else // Use first skin we find
            {
                if ( pSceneData->skins[0].joints_count > 0 )
                {
                    pSkeletonToRead = pSceneData->skins[0].joints[0];
                }
            }

            //-------------------------------------------------------------------------

            if ( pSkeletonToRead == nullptr )
            {
                if ( skeletonRootBoneName.empty() )
                {
                    ImportedSkeleton.LogError( "Failed to find any skeletons" );
                }
                else
                {
                    ImportedSkeleton.LogError( "Failed to find specified skeleton: %s", skeletonRootBoneName.c_str() );
                }
                return;
            }

            ImportedSkeleton.m_name = StringID( (char const*) pSkeletonToRead->name );
            ReadBoneHierarchy( ImportedSkeleton, sceneCtx, pSkeletonToRead, -1 );

            if ( ImportedSkeleton.GetNumBones() > 0 )
            {
                ImportedSkeleton.m_bones[0].m_localTransform = sceneCtx.ApplyUpAxisCorrection( ImportedSkeleton.m_bones[0].m_localTransform );
                ImportedSkeleton.CalculateModelSpaceTransforms();
            }
        }

        static void ReadBoneHierarchy( gltfImportedSkeleton& ImportedSkeleton, gltf::SceneContext const& sceneCtx, cgltf_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr );

            auto const boneIdx = (int32_t) ImportedSkeleton.m_bones.size();
            ImportedSkeleton.m_bones.push_back( ImportedSkeleton::Bone( pNode->name ) );
            ImportedSkeleton.m_bones[boneIdx].m_parentBoneName = ( parentIdx != InvalidIndex ) ? ImportedSkeleton.m_bones[parentIdx].m_name : StringID();
            ImportedSkeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Default Bone transform
            ImportedSkeleton.m_bones[boneIdx].m_localTransform = sceneCtx.GetNodeTransform( pNode );

            // Read child bones
            for ( int i = 0; i < pNode->children_count; i++ )
            {
                cgltf_node* pChildNode = pNode->children[i];
                ReadBoneHierarchy( ImportedSkeleton, sceneCtx, pChildNode, boneIdx );
            }
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<ImportedSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName )
    {
        return gltfSkeletonFileReader::ReadSkeleton( sourceFilePath, skeletonRootBoneName );
    }
}

//-------------------------------------------------------------------------
// Animation
//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    class gltfImportedAnimation : public ImportedAnimation
    {
        friend class gltfAnimationFileReader;

    public:

        using ImportedAnimation::ImportedAnimation;
    };

    //-------------------------------------------------------------------------

    class gltfAnimationFileReader
    {
        struct TransformData
        {
            Quaternion m_rotation;
            Vector     m_translation;
            float      m_scale;
        };

    public:

        static TUniquePtr<ImportedAnimation> ReadAnimation( FileSystem::Path const& sourceFilePath, ImportedSkeleton const& importedSkeleton, String const& animationName )
        {
            EE_ASSERT( sourceFilePath.IsValid() && importedSkeleton.IsValid() );

            TUniquePtr<ImportedAnimation> pAnimation( EE::New<gltfImportedAnimation>( importedSkeleton ) );
            gltfImportedAnimation* pImportedAnimation = (gltfImportedAnimation*) pAnimation.get();
            pImportedAnimation->m_sourcePath = sourceFilePath;

            gltf::SceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                auto pSceneData = sceneCtx.GetSceneData();

                if ( pSceneData->animations_count == 0 )
                {
                    pImportedAnimation->LogError( "Could not find any animations in the gltf scene", animationName.c_str() );
                    return pAnimation;
                }

                // Get animation node
                //-------------------------------------------------------------------------

                cgltf_animation* pAnimationNode = nullptr;

                if ( animationName.empty() )
                {
                    pAnimationNode = &pSceneData->animations[0];
                }
                else // Try to find the specified animation
                {
                    for ( auto i = 0; i < pSceneData->animations_count; i++ )
                    {
                        if ( pSceneData->animations[i].name != nullptr && animationName == pSceneData->animations[i].name )
                        {
                            pAnimationNode = &pSceneData->animations[i];
                        }
                    }

                    if ( pAnimationNode == nullptr )
                    {
                        pImportedAnimation->LogError( "Could not find requested animation (%s) in gltf scene", animationName.c_str() );
                        return pAnimation;
                    }
                }

                EE_ASSERT( pAnimationNode != nullptr );

                // Get animation details
                //-------------------------------------------------------------------------

                float animationDuration = -1.0f;
                size_t numFrames = 0;
                for ( auto s = 0; s < pAnimationNode->samplers_count; s++ )
                {
                    cgltf_accessor const* pInputAccessor = pAnimationNode->samplers[s].input;
                    EE_ASSERT( pInputAccessor->has_max );
                    animationDuration = Math::Max( pInputAccessor->max[0], animationDuration );
                    numFrames = Math::Max( pInputAccessor->count, numFrames );
                }

                pImportedAnimation->m_duration = animationDuration;
                pImportedAnimation->m_numFrames = (uint32_t) numFrames;
                pImportedAnimation->m_samplingFrameRate = animationDuration / numFrames;

                // Read animation transforms
                //-------------------------------------------------------------------------

                ReadAnimationData( sceneCtx, *pImportedAnimation, pAnimationNode );
            }
            else
            {
                pImportedAnimation->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pAnimation;
        }

        static void ReadAnimationData( gltf::SceneContext const& ctx, gltfImportedAnimation& ImportedAnimation, cgltf_animation const* pAnimation )
        {
            EE_ASSERT( pAnimation != nullptr );

            // Create temporary transform storage
            size_t const numBones = ImportedAnimation.GetNumBones();
            TVector<TVector<TransformData>> trackData;
            trackData.resize( numBones );
            for ( auto& track : trackData )
            {
                track.resize( ImportedAnimation.m_numFrames );
            }

            // Read raw transform data
            //-------------------------------------------------------------------------

            size_t const numFrames = ImportedAnimation.m_numFrames;
            for ( auto c = 0; c < pAnimation->channels_count; c++ )
            {
                cgltf_animation_channel const& channel = pAnimation->channels[c];

                if ( channel.target_node->name == nullptr )
                {
                    ImportedAnimation.LogError( "Invalid setup in gltf file, bones have no names" );
                    return;
                }

                int32_t const boneIdx = ImportedAnimation.m_skeleton.GetBoneIndex( StringID( channel.target_node->name ) );
                if ( boneIdx == InvalidIndex )
                {
                    continue;
                }

                EE_ASSERT( channel.sampler->output->count == numFrames );

                switch ( channel.target_path )
                {
                    case cgltf_animation_path_type_rotation:
                    {
                        EE_ASSERT( channel.sampler->output->type == cgltf_type_vec4 );

                        for ( auto i = 0; i < numFrames; i++ )
                        {
                            Float4 rotation;
                            cgltf_accessor_read_float( channel.sampler->output, i, &rotation.m_x, 4 );
                            trackData[boneIdx][i].m_rotation = Quaternion( rotation );
                        }
                    }
                    break;

                    case cgltf_animation_path_type_translation:
                    {
                        EE_ASSERT( channel.sampler->output->type == cgltf_type_vec3 );

                        for ( auto i = 0; i < numFrames; i++ )
                        {
                            Float3 translation;
                            cgltf_accessor_read_float( channel.sampler->output, i, &translation.m_x, 3 );
                            trackData[boneIdx][i].m_translation = translation;
                        }
                    }
                    break;

                    case cgltf_animation_path_type_scale:
                    {
                        EE_ASSERT( channel.sampler->output->type == cgltf_type_vec3 );

                        for ( auto i = 0; i < numFrames; i++ )
                        {
                            Float3 scale;
                            cgltf_accessor_read_float( channel.sampler->output, i, &scale.m_x, 3 );
                            // TODO: Log warning
                            EE_ASSERT( scale.m_x == scale.m_y && scale.m_y == scale.m_z );
                            trackData[boneIdx][i].m_scale = scale.m_x;
                        }
                    }
                    break;

                    default:
                    break;
                }
            }

            // Create matrices from raw gltf data
            //-------------------------------------------------------------------------

            ImportedAnimation.m_tracks.resize( ImportedAnimation.GetNumBones() );
            for ( auto boneIdx = 0u; boneIdx < numBones; boneIdx++ )
            {
                ImportedAnimation.m_tracks[boneIdx].m_localTransforms.resize( ImportedAnimation.m_numFrames );
                for ( auto f = 0; f < ImportedAnimation.m_numFrames; f++ )
                {
                    TransformData const& transformData = trackData[boneIdx][f];
                    ImportedAnimation.m_tracks[boneIdx].m_localTransforms[f] = Transform( transformData.m_rotation, transformData.m_translation, transformData.m_scale );

                    // Correct the up-axis for all root transform
                    if ( boneIdx == 0 )
                    {
                        ImportedAnimation.m_tracks[boneIdx].m_localTransforms[f] = ctx.ApplyUpAxisCorrection( ImportedAnimation.m_tracks[boneIdx].m_localTransforms[f] );
                    }
                }
            }
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<ImportedAnimation> ReadAnimation( FileSystem::Path const& animationFilePath, ImportedSkeleton const& ImportedSkeleton, String const& takeName )
    {
        return gltfAnimationFileReader::ReadAnimation( animationFilePath, ImportedSkeleton, takeName );
    }
}

//-------------------------------------------------------------------------
// Mesh
//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    class gltfImportedMesh : public ImportedMesh
    {
        friend class gltfMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class gltfMeshFileReader
    {
    public:

        struct SkinInfo
        {
            cgltf_skin*             m_pSkin = nullptr;
            TVector<cgltf_mesh*>    m_meshes;
        };

        //-------------------------------------------------------------------------

        static bool IsValidTriangleMesh( cgltf_mesh const& meshNode )
        {
            for ( auto p = 0; p < meshNode.primitives_count; p++ )
            {
                if ( meshNode.primitives[p].type != cgltf_primitive_type_triangles )
                {
                    return false;
                }
            }

            return true;
        }

        static void ReadMeshGeometry( gltf::SceneContext const& ctx, gltfImportedMesh& ImportedMesh, cgltf_mesh const& meshData )
        {
            EE_ASSERT( IsValidTriangleMesh( meshData ) );

            for ( auto p = 0; p < meshData.primitives_count; p++ )
            {
                cgltf_primitive const& primitive = meshData.primitives[p];

                // Create new geometry section and allocate vertex memory
                //-------------------------------------------------------------------------

                auto& geometrySection = ImportedMesh.m_geometrySections.emplace_back( ImportedMesh::GeometrySection() );
                geometrySection.m_name = meshData.name;
                geometrySection.m_clockwiseWinding = false;

                EE_ASSERT( primitive.attributes_count > 0 );
                size_t const numVertices = primitive.attributes[0].data->count;
                geometrySection.m_vertices.resize( numVertices );

                // Read vertex data
                //-------------------------------------------------------------------------

                // Check how many texture coordinate attributes do we have?
                uint32_t numTexcoordAttributes = 0;
                for ( auto a = 0; a < primitive.attributes_count; a++ )
                {
                    if ( primitive.attributes[a].type == cgltf_attribute_type_texcoord )
                    {
                        numTexcoordAttributes++;
                    }
                }

                if ( numTexcoordAttributes > 0 )
                {
                    for ( auto v = 0; v < numVertices; v++ )
                    {
                        geometrySection.m_vertices[v].m_texCoords.resize( numTexcoordAttributes );
                    }
                }

                geometrySection.m_numUVChannels = numTexcoordAttributes;

                //-------------------------------------------------------------------------

                for ( auto a = 0; a < primitive.attributes_count; a++ )
                {
                    switch ( primitive.attributes[a].type )
                    {
                        case cgltf_attribute_type_position:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec3 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                Float3 position;
                                cgltf_accessor_read_float( primitive.attributes[a].data, i, &position.m_x, 3 );
                                geometrySection.m_vertices[i].m_position = ctx.ApplyUpAxisCorrection( Vector( position ) );
                                geometrySection.m_vertices[i].m_position.m_w = 1.0f;
                            }
                        }
                        break;

                        case cgltf_attribute_type_normal:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec3 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                Float3 normal;
                                cgltf_accessor_read_float( primitive.attributes[a].data, i, &normal.m_x, 3 );
                                geometrySection.m_vertices[i].m_normal = ctx.ApplyUpAxisCorrection( Vector( normal ) );
                                geometrySection.m_vertices[i].m_position.m_w = 0.0f;
                            }
                        }
                        break;

                        case cgltf_attribute_type_tangent:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec4 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                Float4 tangent;
                                cgltf_accessor_read_float( primitive.attributes[a].data, i, &tangent.m_x, 4 );
                                geometrySection.m_vertices[i].m_tangent = ctx.ApplyUpAxisCorrection( Vector( tangent ) );
                                geometrySection.m_vertices[i].m_position.m_w = tangent.m_w;
                            }
                        }
                        break;

                        case cgltf_attribute_type_texcoord:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec2 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                Float2 texcoord;
                                cgltf_accessor_read_float( primitive.attributes[a].data, i, &texcoord.m_x, 3 );
                                geometrySection.m_vertices[i].m_texCoords[primitive.attributes[a].index] = texcoord;
                            }
                        }
                        break;

                        case cgltf_attribute_type_color:
                        {
                            // Not implemented
                        }
                        break;

                        case cgltf_attribute_type_joints:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec4 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                uint32_t joints[4] = { 0, 0, 0, 0 };
                                cgltf_accessor_read_uint( primitive.attributes[a].data, i, joints, 4 );

                                for ( auto j = 0; j < 4; j++ )
                                {
                                    geometrySection.m_vertices[i].m_boneIndices.emplace_back( joints[j] );
                                }
                            }
                        }
                        break;

                        case cgltf_attribute_type_weights:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec4 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                float weights[4] = { 0, 0, 0, 0 };
                                // This should also support reading weights stored as normalized uints
                                cgltf_accessor_read_float( primitive.attributes[a].data, i, weights, 4 );
                                for ( auto w = 0; w < 4; w++ )
                                {
                                    geometrySection.m_vertices[i].m_boneWeights.emplace_back( weights[w] );
                                }
                            }
                        }
                        break;

                        default:
                        break;
                    }
                }

                // Read indices
                //-------------------------------------------------------------------------

                if ( primitive.indices->component_type == cgltf_component_type_r_16u )
                {
                    for ( auto i = 0; i < primitive.indices->count; i++ )
                    {
                        geometrySection.m_indices.emplace_back( (uint16_t) cgltf_accessor_read_index( primitive.indices, i ) );
                    }
                }
                else if ( primitive.indices->component_type == cgltf_component_type_r_32u )
                {
                    for ( auto i = 0; i < primitive.indices->count; i++ )
                    {
                        geometrySection.m_indices.emplace_back( (uint32_t) cgltf_accessor_read_index( primitive.indices, i ) );
                    }
                }
                else
                {
                    EE_HALT();
                }
            }
        }

        static TVector<SkinInfo> GetSkeletalMeshDefitions( gltf::SceneContext const& ctx )
        {
            TVector<SkinInfo> skeletalMeshes;

            auto pSceneData = ctx.GetSceneData();
            for ( auto n = 0; n < pSceneData->nodes_count; n++ )
            {
                cgltf_node const& node = pSceneData->nodes[n];
                if ( node.skin != nullptr && node.mesh != nullptr )
                {
                    bool foundSkin = false;
                    for ( auto& skin : skeletalMeshes )
                    {
                        if ( skin.m_pSkin == node.skin )
                        {
                            skin.m_meshes.emplace_back( node.mesh );
                            foundSkin = true;
                            break;
                        }
                    }

                    if ( !foundSkin )
                    {
                        auto& skin = skeletalMeshes.emplace_back( SkinInfo() );
                        skin.m_pSkin = node.skin;
                        skin.m_meshes.emplace_back( node.mesh );
                    }
                }
            }

            return skeletalMeshes;
        }

        //-------------------------------------------------------------------------

        static void ReadJointHierarchy( gltfImportedSkeleton& ImportedSkeleton, TVector<StringID> const& skinnedJoints, TVector<Transform> const& bindPose, cgltf_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr );

            StringID const currentBoneName( pNode->name );
            int32_t const bindPoseIdx = VectorFindIndex( skinnedJoints, currentBoneName );
            if ( bindPoseIdx == InvalidIndex )
            {
                return;
            }

            auto const boneIdx = (int32_t) ImportedSkeleton.m_bones.size();
            ImportedSkeleton.m_bones.push_back( ImportedSkeleton::Bone( pNode->name ) );
            ImportedSkeleton.m_bones[boneIdx].m_parentBoneName = ( parentIdx != InvalidIndex ) ? ImportedSkeleton.m_bones[parentIdx].m_name : StringID();
            ImportedSkeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Default Bone transform
            ImportedSkeleton.m_bones[boneIdx].m_globalTransform = bindPose[bindPoseIdx];

            // Read child bones
            for ( int i = 0; i < pNode->children_count; i++ )
            {
                cgltf_node* pChildNode = pNode->children[i];
                ReadJointHierarchy( ImportedSkeleton, skinnedJoints, bindPose, pChildNode, boneIdx );
            }
        }

        //-------------------------------------------------------------------------

        static TUniquePtr<ImportedMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<ImportedMesh> pMesh( EE::New<gltfImportedMesh>() );
            gltfImportedMesh* pImportedMesh = (gltfImportedMesh*) pMesh.get();
            pImportedMesh->m_sourcePath = sourceFilePath;

            //-------------------------------------------------------------------------

            gltf::SceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                auto pSceneData = sceneCtx.GetSceneData();
                if ( meshesToInclude.empty() )
                {
                    for ( auto m = 0; m < pSceneData->meshes_count; m++ )
                    {
                        if ( IsValidTriangleMesh( pSceneData->meshes[m] ) )
                        {
                            ReadMeshGeometry( sceneCtx, *pImportedMesh, pSceneData->meshes[m] );
                        }
                        else
                        {
                            pImportedMesh->LogError( "Non-triangle mesh skipped: %s", pSceneData->meshes[m].name );
                        }
                    }
                }
                else // Try to find the mesh to read
                {
                    bool meshFound = false;
                    for ( auto m = 0; m < pSceneData->meshes_count; m++ )
                    {
                        if ( VectorContains( meshesToInclude, pSceneData->meshes[m].name ) )
                        {
                            if ( IsValidTriangleMesh( pSceneData->meshes[m] ) )
                            {
                                ReadMeshGeometry( sceneCtx, *pImportedMesh, pSceneData->meshes[m] );
                            }
                            else
                            {
                                pImportedMesh->LogError( "Specified mesh is not a triangle mesh: %s", pSceneData->meshes[m].name );
                            }

                            meshFound = true;
                            break;
                        }
                    }

                    if ( !meshFound )
                    {
                        InlineString meshNames;
                        for ( auto const& meshName : meshesToInclude )
                        {
                            meshNames.append_sprintf( "%s, ", meshName.c_str() );
                        }
                        meshNames = meshNames.substr( 0, meshNames.length() - 2 );

                        pImportedMesh->LogError( "Failed to find any of the specified meshes in gltf file: %s", meshNames.c_str() );
                    }
                }
            }
            else
            {
                pImportedMesh->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pMesh;
        }

        static TUniquePtr<ImportedMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences = 4 )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<ImportedMesh> pMesh( EE::New<gltfImportedMesh>() );
            gltfImportedMesh* pImportedMesh = (gltfImportedMesh*) pMesh.get();
            pImportedMesh->m_sourcePath = sourceFilePath;
            pImportedMesh->m_isSkeletalMesh = true;

            //-------------------------------------------------------------------------

            gltf::SceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                TVector<SkinInfo> skins = GetSkeletalMeshDefitions( sceneCtx );

                if ( skins.size() == 0 )
                {
                    pImportedMesh->LogError( "No skins found in file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                }
                else if ( skins.size() > 1 )
                {
                    pImportedMesh->LogError( "More than one skin found in file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                }
                else // Read skeletal mesh data
                {
                    TVector<StringID> skinnedJoints;
                    for ( auto l = 0; l < skins[0].m_pSkin->joints_count; l++ )
                    {
                        if ( skins[0].m_pSkin->joints[l]->name == nullptr )
                        {
                            pImportedMesh->LogError( "Unnamed joint (%d) encountered, this is not supported by the EE importer: %s -> %s", l, sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                            return pMesh;
                        }

                        skinnedJoints.push_back( StringID( skins[0].m_pSkin->joints[l]->name ) );
                    }

                    // Read bind pose
                    TVector<Transform> bindPose;
                    for ( auto i = 0; i < skins[0].m_pSkin->joints_count; i++ )
                    {
                        float matrixValues[16];
                        cgltf_accessor_read_float( skins[0].m_pSkin->inverse_bind_matrices, i, matrixValues, 16 );
                        Matrix m( matrixValues );

                        Transform t( m.GetInverse() );
                        bindPose.emplace_back( t );
                    }

                    EE_ASSERT( skins[0].m_pSkin->joints_count > 0 );
                    gltfImportedSkeleton& ImportedSkeleton = (gltfImportedSkeleton&) pImportedMesh->m_skeleton;
                    ImportedSkeleton.m_name = StringID( (char const*) skins[0].m_pSkin->joints[0]->name );
                    ReadJointHierarchy( ImportedSkeleton, skinnedJoints, bindPose, skins[0].m_pSkin->joints[0], -1 );
                    if ( ImportedSkeleton.GetNumBones() > 0 )
                    {
                        ImportedSkeleton.CalculateLocalTransforms();
                        ImportedSkeleton.m_bones[0].m_localTransform = sceneCtx.ApplyUpAxisCorrection( ImportedSkeleton.m_bones[0].m_localTransform );
                        ImportedSkeleton.CalculateModelSpaceTransforms();
                    }

                    for ( auto pMeshData : skins[0].m_meshes )
                    {
                        if ( !meshesToInclude.empty() )
                        {
                            if ( !VectorContains( meshesToInclude, pMeshData->name ) )
                            {
                                continue;
                            }
                        }

                        ReadMeshGeometry( sceneCtx, *pImportedMesh, *pMeshData );
                    }
                }
            }
            else
            {
                pImportedMesh->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pMesh;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<ImportedMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
    {
        return gltfMeshFileReader::ReadStaticMesh( sourceFilePath, meshesToInclude );
    }

    TUniquePtr<ImportedMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences )
    {
        return gltfMeshFileReader::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
    }
}