#include "FBX.h"
#include "EngineTools/Import/importedSkeleton.h"
#include "EngineTools/Import/ImportedAnimation.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "EngineTools/ThirdParty/ufbx/ufbx.h"

//-------------------------------------------------------------------------
// UFbx Helpers
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    inline static Matrix ToMatrix( ufbx_matrix const& f )
    {
        Matrix m
        (
            (float) f.cols[0].x, (float) f.cols[0].y, (float) f.cols[0].z, 0.0f,
            (float) f.cols[1].x, (float) f.cols[1].y, (float) f.cols[1].z, 0.0f,
            (float) f.cols[2].x, (float) f.cols[2].y, (float) f.cols[2].z, 0.0f,
            (float) f.cols[3].x, (float) f.cols[3].y, (float) f.cols[3].z, 1.0f
        );

        return m;
    }

    inline static bool ToTransform( ufbx_matrix const& f, Transform& outTransform )
    {
        ufbx_transform const ft = ufbx_matrix_to_transform( &f );

        Quaternion const Q( (float) ft.rotation.x, (float) ft.rotation.y, (float) ft.rotation.z, (float) ft.rotation.w );
        Vector const T( (float) ft.translation.x, (float) ft.translation.y, (float) ft.translation.z, 0.0f );
        Vector const S( (float) ft.scale.x, (float) ft.scale.y, (float) ft.scale.z, 1.0f );

        bool const hasUniformScale = ( Math::IsNearEqual( S[0], S[1], Math::LargeEpsilon ) && Math::IsNearEqual( S[1], S[2], Math::LargeEpsilon ) );

        outTransform = Transform( Q, T, (float) S[0] );
        outTransform.SanitizeScaleValue();

        return hasUniformScale;
    }

    inline static Float2 ToFloat2( ufbx_vec2 v ) { return Float2( (float) v.x, (float) v.y ); }
    inline static Float2 ToFloat2( ufbx_vec3 v ) { return Float2( (float) v.x, (float) v.y ); }
    inline static Float2 ToFloat2( ufbx_vec4 v ) { return Float2( (float) v.x, (float) v.y ); }

    inline static Float3 ToFloat3( ufbx_vec2 v ) { return Float3( (float) v.x, (float) v.y, 0.0f ); }
    inline static Float3 ToFloat3( ufbx_vec3 v ) { return Float3( (float) v.x, (float) v.y, (float) v.z ); }
    inline static Float3 ToFloat3( ufbx_vec4 v ) { return Float3( (float) v.x, (float) v.y, (float) v.z ); }

    inline static Float4 ToFloat4( ufbx_vec2 v ) { return Float4( (float) v.x, (float) v.y, 0.0f, 0.0f ); }
    inline static Float4 ToFloat4( ufbx_vec3 v ) { return Float4( (float) v.x, (float) v.y, (float) v.z, 0.0f ); }
    inline static Float4 ToFloat4( ufbx_vec4 v ) { return Float4( (float) v.x, (float) v.y, (float) v.z, (float) v.w ); }

    inline static Quaternion ToQuat( ufbx_quat v ) { return Quaternion( (float) v.x, (float) v.y, (float) v.z, (float) v.w ); }

    inline static bool CompareAxes( ufbx_coordinate_axes a, ufbx_coordinate_axes b )
    {
        return a.front == b.front && a.right == b.right && a.up == b.up;
    };

    static Quaternion const g_correctiveYtoZ( EulerAngles( -90, 0, 0 ) );

    //-------------------------------------------------------------------------

    static ufbx_scene* CreateSceneFromSource( Source const& source, Log& log, bool sceneContainsBoneTransforms )
    {
        ufbx_load_opts opts = { 0 };
        opts.target_axes = ufbx_axes_right_handed_z_up;
        opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
        opts.target_unit_meters = 1.0f;
        opts.normalize_normals = true;
        opts.normalize_tangents = true;
        opts.generate_missing_normals = true;

        ufbx_error error;
        ufbx_scene* pScene = nullptr;

        if ( source.m_pFileData )
        {
            pScene = ufbx_load_memory( source.m_pFileData->data(), source.m_pFileData->size(), &opts, &error );
        }
        else // Load from file
        {
            pScene = ufbx_load_file( source.m_path.c_str(), &opts, &error );
        }

        if ( pScene == nullptr )
        {
            log.LogError( "Failed to load FBX scene: %s", error.description.data );
        }

        if ( sceneContainsBoneTransforms )
        {
            bool const validSourceCoordinateSystem = CompareAxes( pScene->settings.axes, ufbx_axes_right_handed_y_up ) || CompareAxes( pScene->settings.axes, ufbx_axes_right_handed_z_up );
            if ( !validSourceCoordinateSystem )
            {
                log.LogError( "Invalid source file coordinate system, only right-handed y-up and z-up are supported for skeletal meshes, skeletons and animations!" );
                ufbx_free_scene( pScene );
                pScene = nullptr;
            }
        }

        return pScene;
    }

    static void DestroyScene( ufbx_scene*& pScene )
    {
        if ( pScene != nullptr )
        {
            ufbx_free_scene( pScene );
            pScene = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    // This finds all the nodes of this type that are roots of a branch
    static void FindAllRootNodes( ufbx_node* pCurrentNode, ufbx_element_type nodeType, TVector<ufbx_node*>& results )
    {
        EE_ASSERT( pCurrentNode != nullptr );

        // Return node or continue search
        //-------------------------------------------------------------------------

        if ( pCurrentNode->attrib_type == nodeType )
        {
            results.emplace_back( pCurrentNode );
        }
        else // Search children
        {
            for ( ufbx_node* pChildNode : pCurrentNode->children )
            {
                FindAllRootNodes( pChildNode, nodeType, results );
            }
        }
    }
}

//-------------------------------------------------------------------------
// Skeleton
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    class FbxImportedSkeleton : public Skeleton
    {
        friend class FbxSkeletonFileReader;
        friend class FbxAnimationFileReader;
        friend class FbxMeshFileReader;
    };

    //-------------------------------------------------------------------------

    class FbxSkeletonFileReader
    {
    public:

        static TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName )
        {
            EE_ASSERT( source.IsValid() );

            TUniquePtr<Skeleton> pSkeleton( EE::New<FbxImportedSkeleton>() );

            FbxImportedSkeleton* pImportedSkeleton = (FbxImportedSkeleton*) pSkeleton.get();
            pImportedSkeleton->m_sourcePath = source.m_path;

            //-------------------------------------------------------------------------

            ufbx_scene* pScene = CreateSceneFromSource( source, *pImportedSkeleton, true );
            if ( pScene == nullptr )
            {
                return pSkeleton;
            }

            ReadSkeleton( pScene, skeletonRootBoneName, *pImportedSkeleton );
            DestroyScene( pScene );

            return pSkeleton;
        }

        static bool ReadSkeleton( ufbx_scene* pScene, String const& skeletonRootBoneName, FbxImportedSkeleton& importedSkeleton )
        {
            TVector<ufbx_node*> skeletonRootNodes;
            FindAllRootNodes( pScene->root_node, ufbx_element_type::UFBX_ELEMENT_BONE, skeletonRootNodes );

            auto const numSkeletons = skeletonRootNodes.size();
            if ( numSkeletons == 0 )
            {
                importedSkeleton.LogError( "No Skeletons found in FBX scene" );
                return false;
            }

            ufbx_node* pSkeletonToUse = nullptr;
            if ( !skeletonRootBoneName.empty() )
            {
                for ( auto& pSkeletonNode : skeletonRootNodes )
                {
                    // Check skeleton node name
                    if ( skeletonRootBoneName.comparei( pSkeletonNode->name.data ) == 0 )
                    {
                        pSkeletonToUse = pSkeletonNode;
                        break;
                    }

                    // Check null parents
                    if ( auto pParentNode = pSkeletonNode->parent )
                    {
                        if ( pParentNode->attrib_type == ufbx_element_type::UFBX_ELEMENT_EMPTY && skeletonRootBoneName.comparei( pParentNode->name.data ) == 0 )
                        {
                            pSkeletonToUse = pParentNode;
                            break;
                        }
                    }
                }

                if ( pSkeletonToUse == nullptr )
                {
                    importedSkeleton.LogError( "Couldn't find specified skeleton root: %s", skeletonRootBoneName.c_str() );
                    return false;
                }
            }
            else
            {
                pSkeletonToUse = skeletonRootNodes[0];
            }

            // Read model space transforms
            //-------------------------------------------------------------------------

            importedSkeleton.m_name = StringID( pSkeletonToUse->name.data );
            if ( !ReadBoneHierarchy( importedSkeleton, pScene, pSkeletonToUse, -1 ) )
            {
                return false;
            }

            // Fix up bone rotations
            //-------------------------------------------------------------------------

            // TODO: support other coordinate systems if we ever get a file that is left-handed and y-up
            if ( CompareAxes( pScene->settings.axes, ufbx_axes_right_handed_y_up ) )
            {
                for ( auto& bone : importedSkeleton.m_bones )
                {
                    Quaternion rotation = bone.m_modelSpaceTransform.GetRotation();
                    rotation = g_correctiveYtoZ * rotation;
                    bone.m_modelSpaceTransform.SetRotation( rotation );
                }
            }

            // Complete conversion
            //-------------------------------------------------------------------------

            Vector const rootOffset = importedSkeleton.m_bones[0].m_modelSpaceTransform.GetTranslation().GetNegated();

            // Shift all bones so that the root is at the origin
            if ( !rootOffset.IsNearZero3() )
            {
                for ( size_t i = 1; i < importedSkeleton.m_bones.size(); i++ )
                {
                    importedSkeleton.m_bones[i].m_modelSpaceTransform.AddTranslation( rootOffset );
                }
            }

            // Calculate parent space transforms
            //-------------------------------------------------------------------------

            importedSkeleton.CalculateParentSpaceTransforms();

            return true;
        }

        static bool ReadBoneHierarchy( FbxImportedSkeleton& skeleton, ufbx_scene* pScene, ufbx_node* pNode, int32_t parentIdx )
        {
            EE_ASSERT( pNode != nullptr && ( pNode->attrib_type == UFBX_ELEMENT_BONE || pNode->attrib_type == UFBX_ELEMENT_EMPTY ) );

            auto const boneIdx = (int32_t) skeleton.m_bones.size();
            skeleton.m_bones.push_back( Skeleton::Bone( pNode->name.data ) );
            skeleton.m_bones[boneIdx].m_parentBoneName = ( parentIdx != InvalidIndex ) ? skeleton.m_bones[parentIdx].m_name : StringID();
            skeleton.m_bones[boneIdx].m_parentBoneIdx = parentIdx;

            // Read bone transform
            if ( !ToTransform( pNode->node_to_world, skeleton.m_bones[boneIdx].m_modelSpaceTransform ) )
            {
                skeleton.LogError( "Non-uniform scale detected on bone: %s", pNode->name.data );
                return false;
            }

            // Read child bones
            size_t const numChildren = pNode->children.count;
            for ( size_t i = 0; i < numChildren; i++ )
            {
                ufbx_node* pChildNode = pNode->children.data[i];
                if ( pChildNode->attrib_type == UFBX_ELEMENT_BONE ) // We only support a null root node, all children need to be bones
                {
                    if ( !ReadBoneHierarchy( skeleton, pScene, pChildNode, boneIdx ) )
                    {
                        return false;
                    }
                }
            }

            return true;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName )
    {
        return FbxSkeletonFileReader::ReadSkeleton( source, skeletonRootBoneName );
    }
}

//-------------------------------------------------------------------------
// Animation
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    class FbxImportedAnimation : public Animation
    {
        friend class FbxAnimationFileReader;

        using Animation::Animation;
    };

    //-------------------------------------------------------------------------

    class FbxAnimationFileReader
    {

    public:

        static TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate )
        {
            EE_ASSERT( source.IsValid() && pPrimarySkeleton != nullptr && pPrimarySkeleton->IsValid() );

            TUniquePtr<Animation> pAnimation( EE::New<FbxImportedAnimation>( *pPrimarySkeleton ) );
            FbxImportedAnimation* pImportedAnimation = (FbxImportedAnimation*) pAnimation.get();
            pImportedAnimation->m_sourcePath = source.m_path;
            pImportedAnimation->m_samplingFrameRate = samplingFrameRate;

            ufbx_scene* pScene = CreateSceneFromSource( source, *pImportedAnimation, true );
            if ( pScene == nullptr )
            {
                return pAnimation;
            }

            // Find the required anim stack
            //-------------------------------------------------------------------------

            if ( pScene->anim_stacks.count == 0 )
            {
                pImportedAnimation->LogError( "There are no animations in the FBX scene" );
                return pAnimation;
            }

            ufbx_anim_stack *pAnimStack = nullptr;
            if ( !animationName.empty() )
            {
                for ( ufbx_anim_stack *pStack : pScene->anim_stacks )
                {
                    if ( animationName.comparei( pStack->name.data ) == 0 )
                    {
                        pAnimStack = pStack;
                        break;
                    }
                }

                if ( pAnimStack == nullptr )
                {
                    pImportedAnimation->LogError( "Could not find requested animation (%s) in FBX scene", animationName.c_str() );
                    return pAnimation;
                }
            }
            else // Take the first anim stack present
            {
                pAnimStack = pScene->anim_stacks[0];
            }

            EE_ASSERT( pAnimStack != nullptr );

            // Read animation data
            //-------------------------------------------------------------------------

            TVector<ufbx_node*> boneToNodeMapping;
            if ( GenerateNodeMappings( pScene, *pImportedAnimation, pImportedAnimation->GetPrimaryClip(), boneToNodeMapping ) )
            {
                ReadTrackData( pScene, *pImportedAnimation, pImportedAnimation->GetPrimaryClip(), boneToNodeMapping, pAnimStack );
            }

            for ( auto pSecondarySkeleton : secondarySkeletons )
            {
                boneToNodeMapping.clear();

                AnimationClip& secondaryClip = pImportedAnimation->m_secondaryClips.emplace_back( *pSecondarySkeleton );
                if ( GenerateNodeMappings( pScene, *pImportedAnimation, secondaryClip, boneToNodeMapping ) )
                {
                    ReadTrackData( pScene, *pImportedAnimation, secondaryClip, boneToNodeMapping, pAnimStack );
                }
            }

            DestroyScene( pScene );
            return pAnimation;
        }

        static bool GenerateNodeMappings( ufbx_scene* pScene, FbxImportedAnimation& animation, AnimationClip& animationClip, TVector<ufbx_node*>& outBoneToNodeMapping )
        {
            struct NodeInfo
            {
                NodeInfo() = default;
                NodeInfo( ufbx_node* pNode ) : m_pNode( pNode ) {}

                ufbx_node* m_pNode = nullptr;
            };

            //-------------------------------------------------------------------------

            String missingBonesStr;

            TVector<ufbx_node*> nodes;
            nodes.reserve( pScene->nodes.count );
            for ( int32_t n = 0; n < pScene->nodes.count; n++ )
            {
                if ( pScene->nodes[n]->attrib_type == UFBX_ELEMENT_EMPTY || pScene->nodes[n]->attrib_type == UFBX_ELEMENT_BONE )
                {
                    nodes.emplace_back( pScene->nodes[n] );
                }
            }

            //-------------------------------------------------------------------------

            uint32_t const numBones = animationClip.m_skeleton.GetNumBones();
            outBoneToNodeMapping.resize( numBones, nullptr );

            for ( auto boneIdx = 0u; boneIdx < numBones; boneIdx++ )
            {
                StringID const& boneName = animationClip.m_skeleton.GetBoneName( boneIdx );
                InlineString boneNameStr( boneName.c_str() );

                for ( ufbx_node* pNode : nodes )
                {
                    if ( boneNameStr.comparei( pNode->name.data ) == 0 )
                    {
                        outBoneToNodeMapping[boneIdx] = pNode;
                        break;
                    }
                }

                if ( outBoneToNodeMapping[boneIdx] == nullptr )
                {
                    missingBonesStr.append_sprintf( "%s, ", boneName.c_str() );
                }
            }

            //-------------------------------------------------------------------------

            if ( !missingBonesStr.empty() )
            {
                missingBonesStr = missingBonesStr.substr( 0, missingBonesStr.length() - 2 );
                animation.LogError( "Failed to find tracks for bones: %s", missingBonesStr.c_str() );
                return false;
            }

            return true;
        }

        static bool ReadTrackData( ufbx_scene* pScene, FbxImportedAnimation& animation, AnimationClip& animClip, TVector<ufbx_node*> const& boneToNodeMapping, ufbx_anim_stack *pAnimStack )
        {
            EE_ASSERT( pScene != nullptr && pAnimStack != nullptr );

            float const samplingTimeStep = 1.0f / animation.m_samplingFrameRate;

            animation.m_duration = (float) pAnimStack->time_end - (float) pAnimStack->time_begin;
            animation.m_numFrames = Math::RoundToInt32( animation.m_duration / samplingTimeStep ) + 1;

            int32_t const numBones = (int32_t) animClip.m_skeleton.GetNumBones();
            animClip.m_tracks.resize( numBones );

            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                AnimationClip::TrackData& animTrack = animClip.m_tracks[boneIdx];
                ufbx_node* pBoneNode = boneToNodeMapping[boneIdx];

                bool hasUniformScale = true;

                if ( pBoneNode == nullptr )
                {
                    // Store skeleton reference pose for bone
                    for ( auto i = 0; i < animation.m_numFrames; i++ )
                    {
                        animTrack.m_parentSpaceTransforms.emplace_back( animClip.m_skeleton.GetParentSpaceTransform( boneIdx ) );
                    }
                }
                else // Sample the node's transform evenly for the whole animation stack duration
                {
                    for ( size_t i = 0; i < animation.m_numFrames; i++ )
                    {
                        double const time = pAnimStack->time_begin + ( i * samplingTimeStep );
                        EE_ASSERT( time <= ( pAnimStack->time_end + Math::Epsilon ) );
                        ufbx_transform transform = ufbx_evaluate_transform( pAnimStack->anim, pBoneNode, time );

                        Quaternion const rotation = ToQuat( transform.rotation );
                        Float3 const translation = ToFloat3( transform.translation );
                        Float3 const scale = ToFloat3( transform.scale );
                        animTrack.m_parentSpaceTransforms.emplace_back( rotation, translation, scale[0] );

                        if ( !( Math::IsNearEqual( scale[0], scale[1], Math::LargeEpsilon ) && Math::IsNearEqual( scale[1], scale[2], Math::LargeEpsilon ) ) )
                        {
                            hasUniformScale = false;
                        }

                        if ( i > 0 )
                        {
                            // Negated quaternions are equivalent, but interpolating between ones of different polarity takes a the longer path, so flip the quaternion if necessary.
                            if ( animTrack.m_parentSpaceTransforms[i].GetRotation().GetDot( animTrack.m_parentSpaceTransforms[i - 1].GetRotation() ) < 0 )
                            {
                                animTrack.m_parentSpaceTransforms[i].SetRotation( animTrack.m_parentSpaceTransforms[i].GetRotation().GetNegated() );
                            }
                        }
                    }
                }

                if ( !hasUniformScale )
                {
                    animation.LogWarning( "Non-uniform scale detected for bone: %s", pBoneNode->name.data );
                }
            }

            //-------------------------------------------------------------------------

            if ( CompareAxes( pScene->settings.axes, ufbx_axes_right_handed_y_up ) )
            {
                animClip.GenerateModelSpaceTransforms();

                for ( auto& trackData : animClip.m_tracks )
                {
                    for ( auto& transform : trackData.m_modelSpaceTransforms )
                    {
                        Quaternion const rotation = g_correctiveYtoZ * transform.GetRotation();
                        transform.SetRotation( rotation );
                    }
                }

                animClip.RecreateParentSpaceTransformsFromModelSpaceTransform();
            }

            return true;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Skeleton const*> const& secondarySkeletons, String const& takeName, float samplingFrameRate )
    {
        return FbxAnimationFileReader::ReadAnimation( source, pPrimarySkeleton, secondarySkeletons, takeName, samplingFrameRate );
    }
}

//-------------------------------------------------------------------------
// Mesh
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    class FbxImportedMesh : public Mesh
    {
        friend class FbxMeshFileReader;

        static void DeduplicateSubMeshVertices( Mesh::Geometry& geo )
        {
            geo.m_indices.resize( geo.m_vertices.size() );

            // Generate the index buffer.
            ufbx_vertex_stream streams[1] =
            {
                { geo.m_vertices.data(), geo.m_vertices.size(), sizeof( Mesh::VertexData ) },
            };

            // This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
            // indices are written in `indices[]` and the number of unique vertices is returned.
            size_t const numVertices = ufbx_generate_indices( streams, 1, geo.m_indices.data(), geo.m_indices.size(), NULL, NULL );
            geo.m_vertices.resize( numVertices );
        }
    };

    //-------------------------------------------------------------------------

    class FbxMeshFileReader
    {
    public:

        static TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude )
        {
            EE_ASSERT( source.IsValid() );

            TUniquePtr<Mesh> pMesh( EE::New<FbxImportedMesh>() );
            FbxImportedMesh* pImportedMesh = (FbxImportedMesh*) pMesh.get();
            pImportedMesh->m_sourcePath = source.m_path;

            //-------------------------------------------------------------------------

            ufbx_scene* pScene = CreateSceneFromSource( source, *pImportedMesh, false );
            if ( pScene == nullptr )
            {
                return pMesh;
            }

            ReadAllSubmeshes( *pImportedMesh, pScene, meshesToInclude );
            DestroyScene( pScene );

            if ( !pImportedMesh->HasErrors() )
            {
                for ( auto& mp : pImportedMesh->m_geometries )
                {
                    FbxImportedMesh::DeduplicateSubMeshVertices( mp );
                }
            }

            return pMesh;
        }

        static TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude )
        {
            EE_ASSERT( source.IsValid() );

            TUniquePtr<Mesh> pMesh( EE::New<FbxImportedMesh>() );
            FbxImportedMesh* pImportedMesh = (FbxImportedMesh*) pMesh.get();
            pImportedMesh->m_sourcePath = source.m_path;
            pImportedMesh->m_isSkeletalMesh = true;

            //-------------------------------------------------------------------------

            ufbx_scene* pScene = CreateSceneFromSource( source, *pImportedMesh, true );
            if ( pScene == nullptr )
            {
                return pMesh;
            }

            ReadAllSubmeshes( *pImportedMesh, pScene, meshesToInclude );

            if ( !pImportedMesh->HasErrors() )
            {
                // Since the bind pose is in global space, calculate the local space transforms for the skeleton
                FbxImportedSkeleton& importedSkeleton = (FbxImportedSkeleton&) pImportedMesh->m_skeleton;
                importedSkeleton.CalculateParentSpaceTransforms();
            }

            DestroyScene( pScene );

            if ( !pImportedMesh->HasErrors() )
            {
                for ( auto& mp : pImportedMesh->m_geometries )
                {
                    FbxImportedMesh::DeduplicateSubMeshVertices( mp );
                }
            }

            return pMesh;
        }

        //-------------------------------------------------------------------------

        static void ReadAllSubmeshes( FbxImportedMesh& importedMesh, ufbx_scene* pScene, TVector<String> const& meshesToInclude )
        {
            // Check that the file contains skinned meshes if we are imported a skeletal mesh
            if ( importedMesh.m_isSkeletalMesh )
            {
                bool hasSkeletalMeshes = false;
                for ( ufbx_mesh* pFoundMesh : pScene->meshes )
                {
                    if ( pFoundMesh->skin_deformers.count > 0 )
                    {
                        hasSkeletalMeshes = true;
                        break;
                    }
                }

                if ( !hasSkeletalMeshes )
                {
                    importedMesh.LogError( "No skinned meshes detected in the source file" );
                    return;
                }
            }

            bool meshFound = false;
            bool numUVSetsWarningEmitted = false;

            for ( ufbx_mesh* pMesh : pScene->meshes )
            {
                if ( importedMesh.m_isSkeletalMesh && pMesh->skin_deformers.count == 0 )
                {
                    continue;
                }

                if ( pMesh->num_triangles == 0 ) // How does this happen?
                {
                    continue;
                }

                // Only process specified meshes
                if ( !meshesToInclude.empty() )
                {
                    if ( !VectorContains( meshesToInclude, pMesh->name.data ) )
                    {
                        continue;
                    }
                }

                meshFound = true;

                // Split into sub-meshes based on material
                //-------------------------------------------------------------------------

                size_t firstGeometryIdx = importedMesh.m_geometries.size();
                for ( ufbx_mesh_part const& meshPart : pMesh->material_parts )
                {
                    Mesh::Geometry& geo = importedMesh.m_geometries.emplace_back();
                    geo.m_ID = StringID( pMesh->name.data );
                    geo.m_numUVChannels = Math::Min( Import::Mesh::s_maxNumOfTextureCoords, (uint32_t) pMesh->uv_sets.count );

                    if ( !numUVSetsWarningEmitted && pMesh->uv_sets.count > Import::Mesh::s_maxNumOfTextureCoords )
                    {
                        importedMesh.LogWarning( "Exceeded the maximum number of uv sets allowed, mesh requires %d uv sets per vertex, we only allow a max of %d", pMesh->uv_sets.count, Import::Mesh::s_maxNumOfTextureCoords );
                        numUVSetsWarningEmitted = true;
                    }

                    ReadMeshGeometry( pScene, importedMesh, pMesh, meshPart, geo );
                }

                size_t numGeometries = importedMesh.m_geometries.size() - firstGeometryIdx;

                // Import instances
                //-------------------------------------------------------------------------

                for ( ufbx_node* pMeshNode : pMesh->instances )
                {
                    for ( size_t geoIdx = 0; geoIdx < numGeometries; ++geoIdx )
                    {
                        Mesh::Submesh& submesh = importedMesh.m_submeshes.emplace_back();
                        submesh.m_ID = StringID( pMeshNode->name.data );
                        submesh.m_transform = ToMatrix( pMeshNode->geometry_to_world );
                        submesh.m_geometryIdx = uint32_t( firstGeometryIdx + geoIdx );
                        submesh.m_materialID = StringID( pMeshNode->materials[geoIdx]->name.data );
                    }
                }
            }
            //-------------------------------------------------------------------------

            if ( !meshFound )
            {
                InlineString meshNames;
                for ( auto const& meshName : meshesToInclude )
                {
                    meshNames.append_sprintf( "%s, ", meshName.c_str() );
                }
                meshNames = meshNames.substr( 0, meshNames.length() - 2 );

                importedMesh.LogError( "Couldn't find any matching specified meshes: %s", meshNames.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        static ufbx_node* FindSkeletonForSkin( ufbx_skin_deformer* pSkin )
        {
            ufbx_node* pSkeletonRootNode = nullptr;

            //-------------------------------------------------------------------------

            EE_ASSERT( pSkin->clusters.count > 0 );
            ufbx_skin_cluster* pCluster = pSkin->clusters.data[0];

            EE_ASSERT( pCluster->bone_node != nullptr );
            ufbx_node* pBoneNode = pCluster->bone_node;

            // Traverse hierarchy to find the root of the skeleton
            while ( pBoneNode != nullptr )
            {
                if ( pBoneNode->attrib_type == UFBX_ELEMENT_BONE )
                {
                    pSkeletonRootNode = pBoneNode;
                    pBoneNode = pBoneNode->parent;
                }
                else
                {
                    pBoneNode = nullptr;
                }
            }

            // If the root of the skeleton has a null root, make that the root as this is a common practice
            // TODO: should we make this a setting in the descriptor?
            if ( auto pParentNode = pSkeletonRootNode->parent )
            {
                if ( pParentNode->attrib_type == UFBX_ELEMENT_EMPTY )
                {
                    pSkeletonRootNode = pParentNode;
                }
            }

            return pSkeletonRootNode;
        }

        static bool ReadMeshGeometry( ufbx_scene* pScene, FbxImportedMesh& importedMesh, ufbx_mesh* pMesh, ufbx_mesh_part const& meshPart, FbxImportedMesh::Geometry& geo )
        {
            EE_ASSERT( pMesh != nullptr );

            // Read skeleton
            //-------------------------------------------------------------------------

            ufbx_skin_deformer* pSkin = nullptr;
            FbxImportedSkeleton* pImportedSkeleton = nullptr;
            if ( importedMesh.m_isSkeletalMesh )
            {
                // Check whether skinning data exists for this mesh
                size_t const numSkins = pMesh->skin_deformers.count;
                if ( numSkins == 0 )
                {
                    importedMesh.LogError( "No skin data found for mesh (%s)!", pMesh->name.data );
                    return false;
                }

                if ( numSkins > 1 )
                {
                    importedMesh.LogWarning( "More than one skin found for mesh (%s), compiling only the first skin found!", pMesh->name.data );
                }

                pSkin = pMesh->skin_deformers.data[0];
                if ( pSkin->skinning_method != UFBX_SKINNING_METHOD_LINEAR && pSkin->skinning_method != UFBX_SKINNING_METHOD_RIGID )
                {
                    importedMesh.LogError( "Unsupported skinning type for mesh (%s), only rigid and linear skinning supported!", pMesh->name.data );
                    return false;
                }

                //-------------------------------------------------------------------------

                auto pSkeletonNode = FindSkeletonForSkin( pSkin );
                if ( pSkeletonNode == nullptr )
                {
                    importedMesh.LogError( "Couldn't find skeleton for mesh!", geo.m_ID.c_str() );
                    return false;
                }

                // Check if we have already read a skeleton for this mesh and if the skeleton matches
                pImportedSkeleton = static_cast<FbxImportedSkeleton*>( &importedMesh.m_skeleton );
                if ( pImportedSkeleton->IsValid() )
                {
                    StringID const skeletonRootName( pSkeletonNode->name.data );
                    if ( importedMesh.GetSkeleton().GetRootBoneName() != skeletonRootName )
                    {
                        importedMesh.LogError( "Different skeletons detected for the various sub-meshes. Expected: %s, Got: %s", importedMesh.GetSkeleton().GetRootBoneName().c_str(), skeletonRootName.c_str() );
                        return false;
                    }
                }
                else // Try to read the skeleton
                {
                    FbxSkeletonFileReader::ReadSkeleton( pScene, String( pSkeletonNode->name.data ), *pImportedSkeleton );
                    if ( !pImportedSkeleton->IsValid() )
                    {
                        importedMesh.LogError( "Failed to read mesh skeleton", geo.m_ID.c_str() );
                        return false;
                    }
                }
            }

            //-------------------------------------------------------------------------

            size_t const numTriangleIndices = pMesh->max_face_triangles * 3;
            TVector<uint32_t> triangleIndices( numTriangleIndices );

            bool numBoneInfluencesWarningEmitted = false;

            // Iterate over each face using the specific material.
            for ( uint32_t faceIdx : meshPart.face_indices )
            {
                ufbx_face const& face = pMesh->faces[faceIdx];

                // Iterate over each triangle corner contiguously.
                size_t const numTriangles = ufbx_triangulate_face( triangleIndices.data(), triangleIndices.size(), pMesh, face );
                for ( size_t vertexIdx = 0; vertexIdx < numTriangles * 3; vertexIdx++ )
                {
                    uint32_t const index = triangleIndices[vertexIdx];

                    Mesh::VertexData v;
                    v.m_position = ToFloat4( pMesh->vertex_position[index] );
                    v.m_normal = ToFloat4( pMesh->vertex_normal[index] );

                    if ( pMesh->vertex_tangent.exists )
                    {
                        v.m_tangent = ToFloat4( pMesh->vertex_tangent[index] );
                    }

                    if ( pMesh->vertex_bitangent.exists )
                    {
                        v.m_binormal = ToFloat4( pMesh->vertex_bitangent[index] );
                    }

                    if ( pMesh->vertex_color.exists )
                    {
                        v.m_color = ToFloat4( pMesh->vertex_color[index] );
                    }

                    for ( uint32_t uvSetIdx = 0; uvSetIdx < geo.m_numUVChannels; uvSetIdx++ )
                    {
                        if ( pMesh->uv_sets.data[uvSetIdx].vertex_uv.exists )
                        {
                            ufbx_vec2 const uv = pMesh->uv_sets.data[uvSetIdx].vertex_uv[index];
                            v.m_texCoords[uvSetIdx] = Float2( (float) uv.x, 1.0f - (float) uv.y );
                        }
                    }

                    //-------------------------------------------------------------------------

                    if ( pSkin != nullptr )
                    {
                        // NOTE: This calculation below is the same for each `vertex`, we could pre-calculate these up to `mesh->num_vertices`, and just load the results.
                        uint32_t vertex = pMesh->vertex_indices[index];
                        ufbx_skin_vertex skin_vertex = pSkin->vertices[vertex];

                        if ( !numBoneInfluencesWarningEmitted && skin_vertex.num_weights > Import::Mesh::s_maxNumOfBoneInfluences )
                        {
                            importedMesh.LogWarning( "Exceeded the maximum number of bone influences allowed, mesh requires %d influences per vertex, we only allow a max of %d", skin_vertex.num_weights, Import::Mesh::s_maxNumOfBoneInfluences );
                            numBoneInfluencesWarningEmitted = true;
                        }

                        geo.m_numBoneInfluences = Math::Max( geo.m_numBoneInfluences, Math::ClosestUpperBoundMultiple( skin_vertex.num_weights, 4u ) );
                        geo.m_numBoneInfluences = Math::Min( geo.m_numBoneInfluences, Mesh::s_maxNumOfBoneInfluences );

                        v.m_numBoneWeights = Math::Min( skin_vertex.num_weights, geo.m_numBoneInfluences );
                        EE_ASSERT( v.m_numBoneWeights <= Mesh::s_maxNumOfBoneInfluences );

                        float totalWeight = 0.0f;
                        for ( size_t i = 0; i < v.m_numBoneWeights; i++ )
                        {
                            ufbx_skin_weight skinWeight = pSkin->weights[skin_vertex.weight_begin + i];

                            // Find bone in skeleton that matches this cluster name
                            ufbx_skin_cluster* pCluster = pSkin->clusters[skinWeight.cluster_index];
                            ufbx_node* pBoneNode = pCluster->bone_node;
                            int32_t const boneIdx = importedMesh.m_skeleton.GetBoneIndex( StringID( pBoneNode->name.data ) );
                            if ( boneIdx == InvalidIndex )
                            {
                                importedMesh.LogError( "Unknown bone link node encountered in FBX scene (%s)", pBoneNode->name.data );
                                return false;
                            }

                            v.m_boneIndices[i] = boneIdx;
                            v.m_boneWeights[i] = (float) skinWeight.weight;
                            totalWeight += (float) skinWeight.weight;
                        }

                        // FBX does not guarantee that skin weights are normalized, and we may even be dropping some, so we must renormalize them.
                        for ( size_t i = 0; i < v.m_numBoneWeights; i++ )
                        {
                            v.m_boneWeights[i] /= totalWeight;
                        }
                    }

                    //-------------------------------------------------------------------------

                    uint32_t const subMeshVertexIdx = (uint32_t) geo.m_vertices.size();
                    geo.m_vertices.push_back( v );
                    geo.m_indices.push_back( subMeshVertexIdx );
                }
            }

            return true;
        }
    };

    //-------------------------------------------------------------------------

    TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude )
    {
        return FbxMeshFileReader::ReadStaticMesh( source, meshesToInclude );
    }

    TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude )
    {
        return FbxMeshFileReader::ReadSkeletalMesh( source, meshesToInclude );
    }
}