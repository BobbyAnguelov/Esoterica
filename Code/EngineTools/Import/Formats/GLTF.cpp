#include "GLTF.h"
#include "EngineTools/Import/ImportedSkeleton.h"
#include "EngineTools/Import/ImportedAnimation.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "Base/Types/HashMap.h"

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

    SceneContext::SceneContext( Source const& source, float additionalScalingFactor )
    {
        Initialize( source, additionalScalingFactor );
    }

    SceneContext::~SceneContext()
    {
        Shutdown();
    }

    //-------------------------------------------------------------------------

    void SceneContext::LoadFile( Source const& source, float additionalScalingFactor )
    {
        Shutdown();
        Initialize( source, additionalScalingFactor );
    }

    //-------------------------------------------------------------------------

    void SceneContext::Initialize( Source const& source, float additionalScalingFactor )
    {
        EE_ASSERT( source.IsValid() );

        cgltf_options options = { cgltf_file_type_invalid, 0 };

        if ( source.m_pFileData != nullptr )
        {
            // Parse gltf/glb files
            //-------------------------------------------------------------------------

            cgltf_result const parseResult = cgltf_parse( &options, source.m_pFileData->data(), source.m_pFileData->size(), &m_pSceneData );
            if ( parseResult != cgltf_result_success )
            {
                m_error.sprintf( "Failed to load specified gltf file ( %s ) : %s", source.m_path.c_str(), g_errorStrings[parseResult] );
                return;
            }
        }
        else // Load file from disk
        {
            // Parse gltf/glb files
            //-------------------------------------------------------------------------

            cgltf_result const parseResult = cgltf_parse_file( &options, source.m_path.c_str(), &m_pSceneData );
            if ( parseResult != cgltf_result_success )
            {
                m_error.sprintf( "Failed to load specified gltf file ( %s ) : %s", source.m_path.c_str(), g_errorStrings[parseResult] );
                return;
            }
        }

        // Load all data buffers
        //-------------------------------------------------------------------------

        cgltf_result bufferLoadResult = cgltf_load_buffers( &options, m_pSceneData, source.m_path.c_str() );
        if ( bufferLoadResult != cgltf_result_success )
        {
            m_error.sprintf( "Failed to load gltf file buffers ( %s ) : %s", source.m_path.c_str(), g_errorStrings[bufferLoadResult] );
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
    class gltfImportedSkeleton : public Skeleton
    {
        friend class gltfSkeletonFileReader;
        friend class gltfMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class gltfSkeletonFileReader
    {

    public:

        static TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName )
        {
            EE_ASSERT( source.IsValid() );

            TUniquePtr<Skeleton> pSkeleton( EE::New<gltfImportedSkeleton>() );
            gltfImportedSkeleton* pImportedSkeleton = (gltfImportedSkeleton*) pSkeleton.get();
            pImportedSkeleton->m_sourcePath = source.m_path;

            //-------------------------------------------------------------------------

            gltf::SceneContext sceneCtx( source );
            if ( sceneCtx.IsValid() )
            {
                ReadSkeleton( sceneCtx, skeletonRootBoneName, *pImportedSkeleton );
            }
            else
            {
                pImportedSkeleton->LogError( "Failed to read gltf file: %s -> %s", source.m_path.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pSkeleton;
        }

        static void ReadSkeleton( gltf::SceneContext const& sceneCtx, String const& skeletonRootBoneName, gltfImportedSkeleton& skeleton )
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
                    skeleton.LogError( "Failed to find any skeletons" );
                }
                else
                {
                    skeleton.LogError( "Failed to find specified skeleton: %s", skeletonRootBoneName.c_str() );
                }
                return;
            }

            skeleton.m_name = StringID( (char const*) pSkeletonToRead->name );
            ReadBoneHierarchy( skeleton, sceneCtx, pSkeletonToRead, -1 );

            if ( skeleton.GetNumBones() > 0 )
            {
                skeleton.m_bones[0].m_parentSpaceTransform = sceneCtx.ApplyUpAxisCorrection( skeleton.m_bones[0].m_parentSpaceTransform );
                skeleton.CalculateModelSpaceTransforms();
            }
        }

        static void ReadBoneHierarchy( gltfImportedSkeleton& skeleton, gltf::SceneContext const& sceneCtx, cgltf_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr );

            auto const boneIdx = (int32_t) skeleton.m_bones.size();
            skeleton.m_bones.push_back( Skeleton::Bone( pNode->name ) );
            skeleton.m_bones[boneIdx].m_parentBoneName = ( parentIdx != InvalidIndex ) ? skeleton.m_bones[parentIdx].m_name : StringID();
            skeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Default Bone transform
            skeleton.m_bones[boneIdx].m_parentSpaceTransform = sceneCtx.GetNodeTransform( pNode );

            // Read child bones
            for ( int i = 0; i < pNode->children_count; i++ )
            {
                cgltf_node* pChildNode = pNode->children[i];
                ReadBoneHierarchy( skeleton, sceneCtx, pChildNode, boneIdx );
            }
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName )
    {
        return gltfSkeletonFileReader::ReadSkeleton( source, skeletonRootBoneName );
    }
}

//-------------------------------------------------------------------------
// Animation
//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    class gltfImportedAnimation : public Animation
    {
        friend class gltfAnimationFileReader;

    public:

        using Animation::Animation;
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

        static TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName )
        {
            EE_ASSERT( source.IsValid() && pPrimarySkeleton != nullptr && pPrimarySkeleton->IsValid() );

            if ( !secondarySkeletons.empty() )
            {
                // TODO!!!!!
                EE_UNIMPLEMENTED_FUNCTION();
            }

            TUniquePtr<Animation> pAnimation( EE::New<gltfImportedAnimation>( *pPrimarySkeleton ) );
            gltfImportedAnimation* pImportedAnimation = (gltfImportedAnimation*) pAnimation.get();
            pImportedAnimation->m_sourcePath = source.m_path;

            gltf::SceneContext sceneCtx( source );
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

                ReadAnimationData( sceneCtx, *pImportedAnimation, pImportedAnimation->GetPrimaryClip(), pAnimationNode );
            }
            else
            {
                pImportedAnimation->LogError( "Failed to read gltf file: %s -> %s", source.m_path.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pAnimation;
        }

        static void ReadAnimationData( gltf::SceneContext const& ctx, gltfImportedAnimation& animation, AnimationClip& animClip, cgltf_animation const* pAnimation )
        {
            EE_ASSERT( pAnimation != nullptr );

            // Create temporary transform storage
            size_t const numBones = animClip.GetNumBones();
            TVector<TVector<TransformData>> trackData;
            trackData.resize( numBones );
            for ( auto& track : trackData )
            {
                track.resize( animation.m_numFrames );
            }

            // Read raw transform data
            //-------------------------------------------------------------------------

            size_t const numFrames = animation.m_numFrames;
            for ( auto c = 0; c < pAnimation->channels_count; c++ )
            {
                cgltf_animation_channel const& channel = pAnimation->channels[c];

                if ( channel.target_node->name == nullptr )
                {
                    animation.LogError( "Invalid setup in gltf file, bones have no names" );
                    return;
                }

                int32_t const boneIdx = animClip.m_skeleton.GetBoneIndex( StringID( channel.target_node->name ) );
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

            animClip.m_tracks.resize( animClip.GetNumBones() );
            for ( auto boneIdx = 0u; boneIdx < numBones; boneIdx++ )
            {
                animClip.m_tracks[boneIdx].m_parentSpaceTransforms.resize( animation.m_numFrames );
                for ( auto f = 0; f < animation.m_numFrames; f++ )
                {
                    TransformData const& transformData = trackData[boneIdx][f];
                    animClip.m_tracks[boneIdx].m_parentSpaceTransforms[f] = Transform( transformData.m_rotation, transformData.m_translation, transformData.m_scale );

                    // Correct the up-axis for all root transform
                    if ( boneIdx == 0 )
                    {
                        animClip.m_tracks[boneIdx].m_parentSpaceTransforms[f] = ctx.ApplyUpAxisCorrection( animClip.m_tracks[boneIdx].m_parentSpaceTransforms[f] );
                    }
                }
            }
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& takeName )
    {
        return gltfAnimationFileReader::ReadAnimation( source, pPrimarySkeleton, secondarySkeletons, takeName );
    }
}

//-------------------------------------------------------------------------
// Mesh
//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    class gltfImportedMesh : public Mesh
    {
        friend class gltfMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class gltfMeshFileReader
    {
        struct MeshEntry
        {
            cgltf_mesh const*   m_pMesh = nullptr;
            uint32_t            m_firstGeometryIdx = 0;
            uint32_t            m_numGeometries = 0;
        };

        struct SkinInstanceInfo
        {
            cgltf_skin*             m_pSkin = nullptr;
            TVector<cgltf_node*>    m_nodes;
        };

    public:

        static bool IsValidTriangleMesh( cgltf_mesh const& mesh )
        {
            for ( auto p = 0; p < mesh.primitives_count; p++ )
            {
                if ( mesh.primitives[p].type != cgltf_primitive_type_triangles )
                {
                    return false;
                }
            }

            return true;
        }

        static void ReadMeshGeometry( gltf::SceneContext const& ctx, gltfImportedMesh& importedMesh, cgltf_mesh const& mesh )
        {
            EE_ASSERT( IsValidTriangleMesh( mesh ) );

            for ( auto p = 0; p < mesh.primitives_count; p++ )
            {
                cgltf_primitive const& primitive = mesh.primitives[p];

                // Create new sub mesh and allocate vertex memory
                //-------------------------------------------------------------------------

                auto& geo = importedMesh.m_geometries.emplace_back();
                geo.m_ID = StringID( mesh.name );
                geo.m_clockwiseWinding = false;

                EE_ASSERT( primitive.attributes_count > 0 );
                size_t const numVertices = primitive.attributes[0].data->count;
                geo.m_vertices.resize( numVertices );

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

                geo.m_numUVChannels = Math::Min( Import::Mesh::s_maxNumOfTextureCoords, numTexcoordAttributes );

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
                                geo.m_vertices[i].m_position = ctx.ApplyUpAxisCorrection( Vector( position ) );
                                geo.m_vertices[i].m_position.m_w = 1.0f;
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
                                geo.m_vertices[i].m_normal = ctx.ApplyUpAxisCorrection( Vector( normal ) );
                                geo.m_vertices[i].m_normal.m_w = 0.0f;
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
                                geo.m_vertices[i].m_tangent = ctx.ApplyUpAxisCorrection( Vector( tangent ) );
                                geo.m_vertices[i].m_tangent.m_w = tangent.m_w;
                            }
                        }
                        break;

                        case cgltf_attribute_type_texcoord:
                        {
                            EE_ASSERT( primitive.attributes[a].data->type == cgltf_type_vec2 );

                            for ( auto i = 0; i < numVertices; i++ )
                            {
                                Float2 texcoord;
                                cgltf_accessor_read_float( primitive.attributes[a].data, i, &texcoord.m_x, 2 );
                                geo.m_vertices[i].m_texCoords[primitive.attributes[a].index] = texcoord;
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
                                    geo.m_vertices[i].m_boneIndices[j] = (int32_t) joints[j];
                                }
                            }

                            geo.m_numBoneInfluences = 4;
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
                                    geo.m_vertices[i].m_boneWeights[w] = weights[w];
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
                        geo.m_indices.emplace_back( (uint16_t) cgltf_accessor_read_index( primitive.indices, i ) );
                    }
                }
                else if ( primitive.indices->component_type == cgltf_component_type_r_32u )
                {
                    for ( auto i = 0; i < primitive.indices->count; i++ )
                    {
                        geo.m_indices.emplace_back( (uint32_t) cgltf_accessor_read_index( primitive.indices, i ) );
                    }
                }
                else
                {
                    EE_HALT();
                }
            }
        }

        // Create Submeshes
        //-------------------------------------------------------------------------

        static void CreateInstancesFromNodes( gltf::SceneContext const& ctx, gltfImportedMesh& importedMesh, cgltf_data const* pSceneData, THashMap<cgltf_mesh const*, MeshEntry> const& meshMap )
        {
            for ( auto n = 0; n < pSceneData->nodes_count; n++ )
            {
                cgltf_node const& node = pSceneData->nodes[n];
                if ( node.mesh == nullptr )
                {
                    continue;
                }

                auto const foundIter = meshMap.find( node.mesh );
                if ( foundIter == meshMap.end() )
                {
                    continue;
                }

                MeshEntry const& entry = foundIter->second;

                Transform const nodeTransform = ctx.GetNodeTransform( const_cast<cgltf_node*>( &node ), true );

                for ( uint32_t geoIdx = 0; geoIdx < entry.m_numGeometries; geoIdx++ )
                {
                    Mesh::Submesh& submesh = importedMesh.m_submeshes.emplace_back();
                    submesh.m_ID = StringID( node.name );
                    submesh.m_transform = nodeTransform.ToMatrix();
                    submesh.m_geometryIdx = entry.m_firstGeometryIdx + geoIdx;

                    cgltf_primitive const& primitive = entry.m_pMesh->primitives[geoIdx];
                    char const* pMaterialName = ( primitive.material != nullptr && primitive.material->name != nullptr ) ? primitive.material->name : "Default";
                    submesh.m_materialID = StringID( pMaterialName );
                }
            }
        }

        // Skeletal Mesh Helpers
        //-------------------------------------------------------------------------

        static TVector<SkinInstanceInfo> GetSkeletalMeshDefinitions( gltf::SceneContext const& ctx )
        {
            TVector<SkinInstanceInfo> skeletalMeshes;

            auto pSceneData = ctx.GetSceneData();
            for ( auto n = 0; n < pSceneData->nodes_count; n++ )
            {
                cgltf_node const& node = pSceneData->nodes[n];
                if ( node.skin != nullptr && node.mesh != nullptr )
                {
                    bool foundSkin = false;
                    for ( auto& skinInfo : skeletalMeshes )
                    {
                        if ( skinInfo.m_pSkin == node.skin )
                        {
                            skinInfo.m_nodes.emplace_back( const_cast<cgltf_node*>( &node ) );
                            foundSkin = true;
                            break;
                        }
                    }

                    if ( !foundSkin )
                    {
                        auto& skinInfo = skeletalMeshes.emplace_back( SkinInstanceInfo() );
                        skinInfo.m_pSkin = node.skin;
                        skinInfo.m_nodes.emplace_back( const_cast<cgltf_node*>( &node ) );
                    }
                }
            }

            return skeletalMeshes;
        }

        //-------------------------------------------------------------------------

        static void ReadJointHierarchy( gltfImportedSkeleton& skeleton, TVector<StringID> const& skinnedJoints, TVector<Transform> const& bindPose, cgltf_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr );

            StringID const currentBoneName( pNode->name );
            int32_t const bindPoseIdx = VectorFindIndex( skinnedJoints, currentBoneName );
            if ( bindPoseIdx == InvalidIndex )
            {
                return;
            }

            auto const boneIdx = (int32_t) skeleton.m_bones.size();
            skeleton.m_bones.push_back( Skeleton::Bone( pNode->name ) );
            skeleton.m_bones[boneIdx].m_parentBoneName = ( parentIdx != InvalidIndex ) ? skeleton.m_bones[parentIdx].m_name : StringID();
            skeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Default Bone transform
            skeleton.m_bones[boneIdx].m_modelSpaceTransform = bindPose[bindPoseIdx];

            // Read child bones
            for ( int i = 0; i < pNode->children_count; i++ )
            {
                cgltf_node* pChildNode = pNode->children[i];
                ReadJointHierarchy( skeleton, skinnedJoints, bindPose, pChildNode, boneIdx );
            }
        }

        // Static Mesh Import
        //-------------------------------------------------------------------------

        static TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude )
        {
            EE_ASSERT( source.IsValid() );

            TUniquePtr<Mesh> pMesh( EE::New<gltfImportedMesh>() );
            gltfImportedMesh* pImportedMesh = (gltfImportedMesh*) pMesh.get();
            pImportedMesh->m_sourcePath = source.m_path;

            //-------------------------------------------------------------------------

            gltf::SceneContext sceneCtx( source );
            if ( !sceneCtx.IsValid() )
            {
                pImportedMesh->LogError( "Failed to read gltf file: %s -> %s", source.m_path.c_str(), sceneCtx.GetErrorMessage().c_str() );
                return pMesh;
            }

            auto pSceneData = sceneCtx.GetSceneData();

            // Build map of unique meshes
            //-------------------------------------------------------------------------

            THashMap<cgltf_mesh const*, MeshEntry> meshMap;
            bool meshFilterActive = !meshesToInclude.empty();

            for ( auto n = 0; n < pSceneData->nodes_count; n++ )
            {
                cgltf_node const& node = pSceneData->nodes[n];
                if ( node.mesh == nullptr )
                {
                    continue;
                }

                // Filter by name if requested
                if ( meshFilterActive && node.mesh->name != nullptr )
                {
                    if ( !VectorContains( meshesToInclude, String( node.mesh->name ) ) )
                    {
                        continue;
                    }
                }

                if ( meshMap.find( node.mesh ) != meshMap.end() )
                {
                    continue;
                }

                if ( !IsValidTriangleMesh( *node.mesh ) )
                {
                    pImportedMesh->LogError( "Non-triangle mesh skipped: %s", node.mesh->name );
                    continue;
                }

                MeshEntry entry;
                entry.m_pMesh = node.mesh;
                entry.m_firstGeometryIdx = (uint32_t) pImportedMesh->m_geometries.size();
                entry.m_numGeometries = (uint32_t) node.mesh->primitives_count;

                ReadMeshGeometry( sceneCtx, *pImportedMesh, *node.mesh );
                meshMap[node.mesh] = entry;
            }

            // Report missing meshes if filtering
            //-------------------------------------------------------------------------

            if ( meshFilterActive )
            {
                for ( auto const& meshName : meshesToInclude )
                {
                    bool found = false;
                    for ( auto const& pair : meshMap )
                    {
                        if ( pair.first->name != nullptr && meshName == pair.first->name )
                        {
                            found = true;
                            break;
                        }
                    }

                    if ( !found )
                    {
                        pImportedMesh->LogError( "Couldn't find specified mesh: %s", meshName.c_str() );
                    }
                }
            }

            // Create instances from nodes
            //-------------------------------------------------------------------------

            if ( meshMap.empty() )
            {
                pImportedMesh->LogError( "No valid meshes found in gltf file" );
                return pMesh;
            }

            if ( meshFilterActive )
            {
                // Only create instances for nodes whose mesh is in the filtered set
                for ( auto n = 0; n < pSceneData->nodes_count; n++ )
                {
                    cgltf_node const& node = pSceneData->nodes[n];
                    if ( node.mesh == nullptr )
                    {
                        continue;
                    }

                    auto const foundIt = meshMap.find( node.mesh );
                    if ( foundIt == meshMap.end() )
                    {
                        continue;
                    }

                    MeshEntry const& entry = foundIt->second;
                    Transform const nodeTransform = sceneCtx.GetNodeTransform( const_cast<cgltf_node*>( &node ), true );

                    for ( uint32_t geoIdx = 0; geoIdx < entry.m_numGeometries; ++geoIdx )
                    {
                        Mesh::Submesh& submesh = pImportedMesh->m_submeshes.emplace_back();
                        submesh.m_ID = StringID( node.name );
                        submesh.m_transform = nodeTransform.ToMatrix();
                        submesh.m_geometryIdx = entry.m_firstGeometryIdx + geoIdx;

                        cgltf_primitive const& primitive = entry.m_pMesh->primitives[geoIdx];
                        char const* pMaterialName = ( primitive.material != nullptr && primitive.material->name != nullptr ) ? primitive.material->name : "Default";
                        submesh.m_materialID = StringID( pMaterialName );
                    }
                }
            }
            else
            {
                CreateInstancesFromNodes( sceneCtx, *pImportedMesh, pSceneData, meshMap );
            }

            return pMesh;
        }

        // Skeletal Mesh Import
        //-------------------------------------------------------------------------

        static TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences = 4 )
        {
            EE_ASSERT( source.IsValid() );

            TUniquePtr<Mesh> pMesh( EE::New<gltfImportedMesh>() );
            gltfImportedMesh* pImportedMesh = (gltfImportedMesh*) pMesh.get();
            pImportedMesh->m_sourcePath = source.m_path;
            pImportedMesh->m_isSkeletalMesh = true;

            //-------------------------------------------------------------------------

            gltf::SceneContext sceneCtx( source );
            if ( !sceneCtx.IsValid() )
            {
                pImportedMesh->LogError( "Failed to read gltf file: %s -> %s", source.m_path.c_str(), sceneCtx.GetErrorMessage().c_str() );
                return pMesh;
            }

            TVector<SkinInstanceInfo> skins = GetSkeletalMeshDefinitions( sceneCtx );

            if ( skins.size() == 0 )
            {
                pImportedMesh->LogError( "No skins found in file: %s", source.m_path.c_str() );
                return pMesh;
            }
            else if ( skins.size() > 1 )
            {
                pImportedMesh->LogError( "More than one skin found in file: %s", source.m_path.c_str() );
                return pMesh;
            }

            SkinInstanceInfo const& skinInfo = skins[0];

            // Read joint names
            //-------------------------------------------------------------------------

            TVector<StringID> skinnedJoints;
            for ( auto l = 0; l < skinInfo.m_pSkin->joints_count; l++ )
            {
                if ( skinInfo.m_pSkin->joints[l]->name == nullptr )
                {
                    pImportedMesh->LogError( "Unnamed joint (%d) encountered, this is not supported by the EE importer: %s", l, source.m_path.c_str() );
                    return pMesh;
                }

                skinnedJoints.push_back( StringID( skinInfo.m_pSkin->joints[l]->name ) );
            }

            // Read bind pose
            //-------------------------------------------------------------------------

            TVector<Transform> bindPose;
            for ( auto i = 0; i < skinInfo.m_pSkin->joints_count; i++ )
            {
                float matrixValues[16];
                cgltf_accessor_read_float( skinInfo.m_pSkin->inverse_bind_matrices, i, matrixValues, 16 );
                Matrix m( matrixValues );

                Transform t( m.GetInverse() );
                bindPose.emplace_back( t );
            }

            EE_ASSERT( skinInfo.m_pSkin->joints_count > 0 );
            gltfImportedSkeleton& skeleton = (gltfImportedSkeleton&) pImportedMesh->m_skeleton;
            skeleton.m_name = StringID( (char const*) skinInfo.m_pSkin->joints[0]->name );
            ReadJointHierarchy( skeleton, skinnedJoints, bindPose, skinInfo.m_pSkin->joints[0], -1 );
            if ( skeleton.GetNumBones() > 0 )
            {
                skeleton.CalculateParentSpaceTransforms();
                skeleton.m_bones[0].m_parentSpaceTransform = sceneCtx.ApplyUpAxisCorrection( skeleton.m_bones[0].m_parentSpaceTransform );
                skeleton.CalculateModelSpaceTransforms();
            }

            // Build mesh map from skin instance nodes
            //-------------------------------------------------------------------------

            THashMap<cgltf_mesh const*, MeshEntry> meshMap;
            bool meshFilterActive = !meshesToInclude.empty();

            for ( cgltf_node* pNode : skinInfo.m_nodes )
            {
                if ( pNode->mesh == nullptr )
                {
                    continue;
                }

                if ( meshFilterActive && pNode->mesh->name != nullptr )
                {
                    if ( !VectorContains( meshesToInclude, String( pNode->mesh->name ) ) )
                    {
                        continue;
                    }
                }

                if ( meshMap.find( pNode->mesh ) != meshMap.end() )
                {
                    continue;
                }

                if ( !IsValidTriangleMesh( *pNode->mesh ) )
                {
                    pImportedMesh->LogError( "Non-triangle mesh skipped: %s", pNode->mesh->name );
                    continue;
                }

                MeshEntry entry;
                entry.m_pMesh = pNode->mesh;
                entry.m_firstGeometryIdx = (uint32_t) pImportedMesh->m_geometries.size();
                entry.m_numGeometries = (uint32_t) pNode->mesh->primitives_count;

                ReadMeshGeometry( sceneCtx, *pImportedMesh, *pNode->mesh );
                meshMap[pNode->mesh] = entry;
            }

            if ( meshMap.empty() )
            {
                pImportedMesh->LogError( "No valid meshes found for skin" );
                return pMesh;
            }

            // Create instances from skin nodes
            //-------------------------------------------------------------------------

            for ( cgltf_node* pNode : skinInfo.m_nodes )
            {
                if ( pNode->mesh == nullptr )
                {
                    continue;
                }

                if ( meshFilterActive && pNode->mesh->name != nullptr )
                {
                    if ( !VectorContains( meshesToInclude, String( pNode->mesh->name ) ) )
                    {
                        continue;
                    }
                }

                auto const foundIt = meshMap.find( pNode->mesh );
                if ( foundIt == meshMap.end() )
                {
                    continue;
                }

                MeshEntry const& entry = foundIt->second;
                Transform const nodeTransform = sceneCtx.GetNodeTransform( pNode, true );

                for ( uint32_t geoIdx = 0; geoIdx < entry.m_numGeometries; ++geoIdx )
                {
                    Mesh::Submesh& submesh = pImportedMesh->m_submeshes.emplace_back();
                    submesh.m_ID = StringID( pNode->name );
                    submesh.m_transform = nodeTransform.ToMatrix();
                    submesh.m_geometryIdx = entry.m_firstGeometryIdx + geoIdx;

                    cgltf_primitive const& primitive = entry.m_pMesh->primitives[geoIdx];
                    char const* pMaterialName = ( primitive.material != nullptr && primitive.material->name != nullptr ) ? primitive.material->name : "Default";
                    submesh.m_materialID = StringID( pMaterialName );
                }
            }

            return pMesh;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude )
    {
        return gltfMeshFileReader::ReadStaticMesh( source, meshesToInclude );
    }

    TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude )
    {
        return gltfMeshFileReader::ReadSkeletalMesh( source, meshesToInclude );
    }
}