#include "gltfMesh.h"
#include "gltfSceneContext.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class gltfRawSkeleton : public RawSkeleton
    {
        friend class gltfMeshFileReader;
    };

    class gltfRawMesh : public RawMesh
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

        static void ReadMeshGeometry( gltf::gltfSceneContext const& ctx, gltfRawMesh& rawMesh, cgltf_mesh const& meshData )
        {
            EE_ASSERT( IsValidTriangleMesh( meshData ) );

            for ( auto p = 0; p < meshData.primitives_count; p++ )
            {
                cgltf_primitive const& primitive = meshData.primitives[p];

                // Create new geometry section and allocate vertex memory
                //-------------------------------------------------------------------------

                auto& geometrySection = rawMesh.m_geometrySections.emplace_back( RawMesh::GeometrySection() );
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

        static TVector<SkinInfo> GetSkeletalMeshDefitions( gltf::gltfSceneContext const& ctx )
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

        static void ReadJointHierarchy( gltfRawSkeleton& rawSkeleton, TVector<StringID> const& skinnedJoints, TVector<Transform> const& bindPose, cgltf_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr );

            StringID const currentBoneName( pNode->name );
            int32_t const bindPoseIdx = VectorFindIndex( skinnedJoints, currentBoneName );
            if ( bindPoseIdx == InvalidIndex )
            {
                return;
            }

            auto const boneIdx = (int32_t) rawSkeleton.m_bones.size();
            rawSkeleton.m_bones.push_back( RawSkeleton::BoneData( pNode->name ) );
            rawSkeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Default Bone transform
            rawSkeleton.m_bones[boneIdx].m_globalTransform = bindPose[bindPoseIdx];

            // Read child bones
            for ( int i = 0; i < pNode->children_count; i++ )
            {
                cgltf_node* pChildNode = pNode->children[i];
                ReadJointHierarchy( rawSkeleton, skinnedJoints, bindPose, pChildNode, boneIdx );
            }
        }

        //-------------------------------------------------------------------------

        static TUniquePtr<RawMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, String const& nameOfMeshToCompile )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawMesh> pMesh( EE::New<RawMesh>() );
            gltfRawMesh* pRawMesh = (gltfRawMesh*) pMesh.get();

            //-------------------------------------------------------------------------

            gltf::gltfSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                auto pSceneData = sceneCtx.GetSceneData();
                if ( nameOfMeshToCompile.empty() )
                {
                    for ( auto m = 0; m < pSceneData->meshes_count; m++ )
                    {
                        if ( IsValidTriangleMesh( pSceneData->meshes[m] ) )
                        {
                            ReadMeshGeometry( sceneCtx, *pRawMesh, pSceneData->meshes[m] );
                        }
                        else
                        {
                            pRawMesh->LogError( "Non-triangle mesh skipped: %s", pSceneData->meshes[m].name );
                        }
                    }
                }
                else // Try to find the mesh to read
                {
                    bool meshFound = false;
                    for ( auto m = 0; m < pSceneData->meshes_count; m++ )
                    {
                        if ( nameOfMeshToCompile == pSceneData->meshes[m].name )
                        {
                            if ( IsValidTriangleMesh( pSceneData->meshes[m] ) )
                            {
                                ReadMeshGeometry( sceneCtx, *pRawMesh, pSceneData->meshes[m] );
                            }
                            else
                            {
                                pRawMesh->LogError( "Specified mesh is not a triangle mesh: %s", nameOfMeshToCompile.c_str() );
                            }
                            meshFound = true;
                            break;
                        }
                    }

                    if ( !meshFound )
                    {
                        pRawMesh->LogError( "Failed to find specified mesh in gltf file: %s", nameOfMeshToCompile.c_str() );
                    }
                }
            }
            else
            {
                pRawMesh->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pMesh;
        }

        static TUniquePtr<RawMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, int32_t maxBoneInfluences = 4 )
        {
            EE_ASSERT( sourceFilePath.IsValid() );

            TUniquePtr<RawMesh> pMesh( EE::New<RawMesh>() );
            gltfRawMesh* pRawMesh = (gltfRawMesh*) pMesh.get();
            pRawMesh->m_isSkeletalMesh = true;

            //-------------------------------------------------------------------------

            gltf::gltfSceneContext sceneCtx( sourceFilePath );
            if ( sceneCtx.IsValid() )
            {
                TVector<SkinInfo> skins = GetSkeletalMeshDefitions( sceneCtx );

                if ( skins.size() == 0 )
                {
                    pRawMesh->LogError( "No skins found in file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                }
                else if ( skins.size() > 1 )
                {
                    pRawMesh->LogError( "More than one skin found in file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                }
                else // Read skeletal mesh data
                {
                    TVector<StringID> skinnedJoints;
                    for ( auto l = 0; l < skins[0].m_pSkin->joints_count; l++ )
                    {
                        if ( skins[0].m_pSkin->joints[l]->name == nullptr )
                        {
                            pRawMesh->LogError( "Unnamed joint (%d) encountered, this is not supported by the EE importer: %s -> %s", l, sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                            return pMesh;
                        }

                        skinnedJoints.push_back( StringID( skins[0].m_pSkin->joints[l]->name ) );
                    }

                    // Read bind pose
                    TVector<Transform> bindPose;
                    for ( auto i = 0; i < skins[0].m_pSkin->joints_count; i++ )
                    {
                        Matrix m;
                        cgltf_accessor_read_float( skins[0].m_pSkin->inverse_bind_matrices, i, &m[0][0], 16 );

                        Transform t( m.GetInverse() );
                        bindPose.emplace_back( t );
                    }

                    EE_ASSERT( skins[0].m_pSkin->joints_count > 0 );
                    gltfRawSkeleton& rawSkeleton = (gltfRawSkeleton&) pRawMesh->m_skeleton;
                    rawSkeleton.m_name = StringID( (char const*) skins[0].m_pSkin->joints[0]->name );
                    ReadJointHierarchy( rawSkeleton, skinnedJoints, bindPose, skins[0].m_pSkin->joints[0], -1 );
                    if ( rawSkeleton.GetNumBones() > 0 )
                    {
                        rawSkeleton.CalculateLocalTransforms();
                        rawSkeleton.m_bones[0].m_localTransform = sceneCtx.ApplyUpAxisCorrection( rawSkeleton.m_bones[0].m_localTransform );
                        rawSkeleton.CalculateGlobalTransforms();
                    }

                    for ( auto pMeshData : skins[0].m_meshes )
                    {
                        ReadMeshGeometry( sceneCtx, *pRawMesh, *pMeshData );
                    }
                }
            }
            else
            {
                pRawMesh->LogError( "Failed to read gltf file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
            }

            return pMesh;
        }
    };
}

//-------------------------------------------------------------------------

namespace EE::gltf
{
    TUniquePtr<RawAssets::RawMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, String const& nameOfMeshToCompile )
        {
            return RawAssets::gltfMeshFileReader::ReadStaticMesh( sourceFilePath, nameOfMeshToCompile );
        }

    TUniquePtr<RawAssets::RawMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, int32_t maxBoneInfluences )
        {
            return RawAssets::gltfMeshFileReader::ReadSkeletalMesh( sourceFilePath, maxBoneInfluences );
        }
}