#include "FBX.h"
#include "EngineTools/Import/importedSkeleton.h"
#include "EngineTools/Import/ImportedAnimation.h"
#include "EngineTools/Import/ImportedMesh.h"

//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    Quaternion const File::s_correctiveYtoZ = Quaternion( EulerAngles( -90, 0, 0 ) );

    constinit static char const * const g_boneSkinImportDisallowedErrorMessage = "Invalid source file coordinate system, only right-handed y-up and z-up are supported for skeletal meshes, skeletons and animations!";

    //-------------------------------------------------------------------------

    File::File( Source const& source, FileSystem::Path const& sourceDataDirectoryPath )
        : SceneFile( source, sourceDataDirectoryPath )
    {
        if ( !Import::File::IsValid() )
        {
            return;
        }

        ufbx_load_opts opts = { 0 };
        opts.target_axes = ufbx_axes_right_handed_z_up;
        opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
        opts.target_unit_meters = 1.0f;
        opts.normalize_normals = true;
        opts.normalize_tangents = true;
        opts.generate_missing_normals = true;

        ufbx_error error;
        m_pScene = ufbx_load_memory( m_fileData.data(), m_fileData.size(), &opts, &error );

        if ( m_pScene == nullptr )
        {
            m_error.sprintf( "Failed to load FBX scene: %s", error.description.data );
        }

        bool const validSourceCoordinateSystem = CompareAxes( m_pScene->settings.axes, ufbx_axes_right_handed_y_up ) || CompareAxes( m_pScene->settings.axes, ufbx_axes_right_handed_z_up );
        if ( !validSourceCoordinateSystem )
        {
            m_isBoneOrSkinImportAllowed = false;
        }
    }

    File::~File()
    {
        if ( m_pScene != nullptr )
        {
            ufbx_free_scene( m_pScene );
            m_pScene = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void File::GetFileContents( Contents& outContents ) const
    {
        outContents.Clear();

        //-------------------------------------------------------------------------
        // Materials
        //-------------------------------------------------------------------------

        for ( ufbx_material* pMaterial : m_pScene->materials )
        {
            outContents.m_materials.emplace_back( StringID( pMaterial->name.data ) );
        }

        //-------------------------------------------------------------------------
        // Meshes
        //-------------------------------------------------------------------------

        for ( ufbx_mesh* pMesh : m_pScene->meshes )
        {
            for ( ufbx_node* pMeshNode : pMesh->instances )
            {
                if ( pMesh->skin_deformers.count > 0 )
                {
                    outContents.m_skeletalMeshes.emplace_back( StringID( pMeshNode->name.data ) );
                }
                else
                {
                    outContents.m_staticMeshes.emplace_back( StringID( pMeshNode->name.data ) );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Skeletons
        //-------------------------------------------------------------------------

        TVector<ufbx_node*> skeletonRootNodes;
        TVector<ufbx_node*> nullRoots;
        FindAllRootNodes( m_pScene->root_node, ufbx_element_type::UFBX_ELEMENT_BONE, skeletonRootNodes );

        for ( auto& pSkeletonNode : skeletonRootNodes )
        {
            ufbx_node* pParentNode = pSkeletonNode->parent;
            bool const hasNullOrLocatorParent = ( pParentNode != nullptr ) ? ( pParentNode->attrib_type == ufbx_element_type::UFBX_ELEMENT_EMPTY ) : false;
            if ( hasNullOrLocatorParent )
            {
                // Try to find existing null root
                bool rootAlreadyAdded = false;
                for ( auto& pRoot : nullRoots )
                {
                    if ( pRoot == pParentNode )
                    {
                        rootAlreadyAdded = true;
                        break;
                    }
                }

                if ( rootAlreadyAdded )
                {
                    continue;
                }

                outContents.m_skeletons.emplace_back( StringID( pParentNode->name.data ) );
                nullRoots.emplace_back( pParentNode );
            }
            else // No parent so just add it
            {
                outContents.m_skeletons.emplace_back( StringID( pSkeletonNode->name.data ) );
            }
        }

        //-------------------------------------------------------------------------
        // Animations
        //-------------------------------------------------------------------------

        for ( ufbx_anim_stack *pStack : m_pScene->anim_stacks )
        {
            outContents.m_animations.emplace_back( StringID( pStack->name.data ) );
        }
    }
}

//-------------------------------------------------------------------------
// Custom Imported Types
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    class FbxImportedSkeleton : public Skeleton
    {
        friend class File;
    };

    //-------------------------------------------------------------------------

    class FbxImportedAnimation : public Animation
    {
        friend class File;

        using Animation::Animation;
    };

    //-------------------------------------------------------------------------

    class FbxImportedMesh : public Mesh
    {
        friend class File;

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
}

//-------------------------------------------------------------------------
// Skeleton
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    TUniquePtr<Skeleton> File::ReadSkeleton( String const& skeletonRootBoneName ) const
    {
        EE_ASSERT( IsValid() );

        TUniquePtr<Skeleton> pSkeleton( EE::New<FbxImportedSkeleton>() );
        FbxImportedSkeleton& importedSkeleton = *(FbxImportedSkeleton*) pSkeleton.get();

        if ( !IsBoneOrSkinImportAllowed() )
        {
            pSkeleton->LogError( g_boneSkinImportDisallowedErrorMessage );
            return pSkeleton;
        }

        //-------------------------------------------------------------------------

        ReadSkeleton( skeletonRootBoneName, importedSkeleton );

        return pSkeleton;
    }

    bool File::ReadSkeleton( String const& skeletonRootBoneName, FbxImportedSkeleton& skeleton ) const
    {
        TVector<ufbx_node*> skeletonRootNodes;
        FindAllRootNodes( m_pScene->root_node, ufbx_element_type::UFBX_ELEMENT_BONE, skeletonRootNodes );

        auto const numSkeletons = skeletonRootNodes.size();
        if ( numSkeletons == 0 )
        {
            skeleton.LogError( "No Skeletons found in FBX scene" );
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
                skeleton.LogError( "Couldn't find specified skeleton root: %s", skeletonRootBoneName.c_str() );
                return false;
            }
        }
        else
        {
            pSkeletonToUse = skeletonRootNodes[0];
        }

        // Read model space transforms
        //-------------------------------------------------------------------------

        skeleton.m_name = StringID( pSkeletonToUse->name.data );
        if ( !ReadBoneHierarchy( skeleton, pSkeletonToUse, -1 ) )
        {
            return false;
        }

        // Fix up bone rotations
        //-------------------------------------------------------------------------

        // TODO: support other coordinate systems if we ever get a file that is left-handed and y-up
        if ( CompareAxes( m_pScene->settings.axes, ufbx_axes_right_handed_y_up ) )
        {
            for ( auto& bone : skeleton.m_bones )
            {
                Quaternion rotation = bone.m_modelSpaceTransform.GetRotation();
                rotation = File::s_correctiveYtoZ * rotation;
                bone.m_modelSpaceTransform.SetRotation( rotation );
            }
        }

        // Complete conversion
        //-------------------------------------------------------------------------

        Vector const rootOffset = skeleton.m_bones[0].m_modelSpaceTransform.GetTranslation().GetNegated();

        // Shift all bones so that the root is at the origin
        if ( !rootOffset.IsNearZero3() )
        {
            for ( size_t i = 1; i < skeleton.m_bones.size(); i++ )
            {
                skeleton.m_bones[i].m_modelSpaceTransform.AddTranslation( rootOffset );
            }
        }

        // Calculate parent space transforms
        //-------------------------------------------------------------------------

        skeleton.CalculateParentSpaceTransforms();

        return true;
    }

    bool File::ReadBoneHierarchy( FbxImportedSkeleton& skeleton, ufbx_node* pNode, int32_t parentIdx ) const
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
                if ( !ReadBoneHierarchy( skeleton, pChildNode, boneIdx ) )
                {
                    return false;
                }
            }
        }

        return true;
    }
}

//-------------------------------------------------------------------------
// Animation
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    TUniquePtr<Animation> File::ReadAnimation( Skeleton const* pPrimarySkeleton, TVector<Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( pPrimarySkeleton != nullptr && pPrimarySkeleton->IsValid() );

        TUniquePtr<Animation> pAnimation( EE::New<FbxImportedAnimation>( *pPrimarySkeleton ) );
        FbxImportedAnimation* pImportedAnimation = (FbxImportedAnimation*) pAnimation.get();

        if ( !IsBoneOrSkinImportAllowed() )
        {
            pAnimation->LogError( g_boneSkinImportDisallowedErrorMessage );
            return pAnimation;
        }

        //-------------------------------------------------------------------------

        pImportedAnimation->m_sourcePath = m_sourcePath;
        pImportedAnimation->m_samplingFrameRate = samplingFrameRate;

        // Find the required anim stack
        //-------------------------------------------------------------------------

        if ( m_pScene->anim_stacks.count == 0 )
        {
            pImportedAnimation->LogError( "There are no animations in the FBX scene" );
            return pAnimation;
        }

        ufbx_anim_stack *pAnimStack = nullptr;
        if ( !animationName.empty() )
        {
            for ( ufbx_anim_stack *pStack : m_pScene->anim_stacks )
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
            pAnimStack = m_pScene->anim_stacks[0];
        }

        EE_ASSERT( pAnimStack != nullptr );

        // Read animation data
        //-------------------------------------------------------------------------

        if ( !ReadAnimationTrackData( *pImportedAnimation, pImportedAnimation->GetPrimaryClip(), pAnimStack ) )
        {
            return pAnimation;
        }

        for ( auto pSecondarySkeleton : secondarySkeletons )
        {
            AnimationClip& secondaryClip = pImportedAnimation->m_secondaryClips.emplace_back( *pSecondarySkeleton );
            if ( !ReadAnimationTrackData( *pImportedAnimation, secondaryClip, pAnimStack ) )
            {
                return pAnimation;
            }
        }

        return pAnimation;
    }

    bool File::ReadAnimationTrackData( FbxImportedAnimation& animation, AnimationClip& animClip, ufbx_anim_stack *pAnimStack ) const
    {
        EE_ASSERT( pAnimStack != nullptr );

        // Generate bone to node mappings
        //-------------------------------------------------------------------------

        struct NodeInfo
        {
            NodeInfo() = default;
            NodeInfo( ufbx_node* pNode ) : m_pNode( pNode ) {}

            ufbx_node* m_pNode = nullptr;
        };

        TVector<ufbx_node*> boneToNodeMapping;

        //-------------------------------------------------------------------------

        String missingBonesStr;

        TVector<ufbx_node*> nodes;
        nodes.reserve( m_pScene->nodes.count );
        for ( int32_t n = 0; n < m_pScene->nodes.count; n++ )
        {
            if ( m_pScene->nodes[n]->attrib_type == UFBX_ELEMENT_EMPTY || m_pScene->nodes[n]->attrib_type == UFBX_ELEMENT_BONE )
            {
                nodes.emplace_back( m_pScene->nodes[n] );
            }
        }

        //-------------------------------------------------------------------------

        int32_t const numBones = animClip.m_skeleton.GetNumBones();
        boneToNodeMapping.resize( numBones, nullptr );

        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            StringID const& boneName = animClip.m_skeleton.GetBoneName( boneIdx );
            InlineString boneNameStr( boneName.c_str() );

            for ( ufbx_node* pNode : nodes )
            {
                if ( boneNameStr.comparei( pNode->name.data ) == 0 )
                {
                    boneToNodeMapping[boneIdx] = pNode;
                    break;
                }
            }

            if ( boneToNodeMapping[boneIdx] == nullptr )
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

        // Read Track Data
        //-------------------------------------------------------------------------

        float const samplingTimeStep = 1.0f / animation.m_samplingFrameRate;

        animation.m_duration = (float) pAnimStack->time_end - (float) pAnimStack->time_begin;
        animation.m_numFrames = Math::RoundToInt32( animation.m_duration / samplingTimeStep ) + 1;

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

        if ( CompareAxes( m_pScene->settings.axes, ufbx_axes_right_handed_y_up ) )
        {
            animClip.GenerateModelSpaceTransforms();

            for ( auto& trackData : animClip.m_tracks )
            {
                for ( auto& transform : trackData.m_modelSpaceTransforms )
                {
                    Quaternion const rotation = File::s_correctiveYtoZ * transform.GetRotation();
                    transform.SetRotation( rotation );
                }
            }

            animClip.RecreateParentSpaceTransformsFromModelSpaceTransform();
        }

        return true;
    }
}

//-------------------------------------------------------------------------
// Mesh
//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    TUniquePtr<Mesh> File::ReadStaticMesh( TVector<String> const& meshesToInclude ) const
    {
        EE_ASSERT( IsValid() );

        TUniquePtr<Mesh> pMesh( EE::New<FbxImportedMesh>() );
        FbxImportedMesh* pImportedMesh = (FbxImportedMesh*) pMesh.get();
        pImportedMesh->m_sourcePath = m_sourcePath;

        //-------------------------------------------------------------------------

        ReadAllSubmeshes( *pImportedMesh, meshesToInclude );

        if ( !pImportedMesh->HasErrors() )
        {
            for ( auto& mp : pImportedMesh->m_geometries )
            {
                FbxImportedMesh::DeduplicateSubMeshVertices( mp );
            }
        }

        //-------------------------------------------------------------------------

        ReadAllMaterials( pImportedMesh->m_materials );

        return pMesh;
    }

    TUniquePtr<Mesh> File::ReadSkeletalMesh( TVector<String> const& meshesToInclude ) const
    {
        EE_ASSERT( IsValid() );

        TUniquePtr<Mesh> pMesh( EE::New<FbxImportedMesh>() );

        if ( !IsBoneOrSkinImportAllowed() )
        {
            pMesh->LogError( g_boneSkinImportDisallowedErrorMessage );
            return pMesh;
        }

        //-------------------------------------------------------------------------

        FbxImportedMesh* pImportedMesh = (FbxImportedMesh*) pMesh.get();
        pImportedMesh->m_sourcePath = m_sourcePath;
        pImportedMesh->m_isSkeletalMesh = true;

        //-------------------------------------------------------------------------

        ReadAllSubmeshes( *pImportedMesh, meshesToInclude );

        if ( !pImportedMesh->HasErrors() )
        {
            // Since the bind pose is in global space, calculate the local space transforms for the skeleton
            FbxImportedSkeleton& importedSkeleton = (FbxImportedSkeleton&) pImportedMesh->m_skeleton;
            importedSkeleton.CalculateParentSpaceTransforms();
        }

        if ( !pImportedMesh->HasErrors() )
        {
            for ( auto& mp : pImportedMesh->m_geometries )
            {
                FbxImportedMesh::DeduplicateSubMeshVertices( mp );
            }
        }

        //-------------------------------------------------------------------------

        ReadAllMaterials( pImportedMesh->m_materials );

        return pMesh;
    }

    //-------------------------------------------------------------------------

    void File::ReadAllSubmeshes( FbxImportedMesh& importedMesh, TVector<String> const& meshesToInclude ) const
    {
        // Check that the file contains skinned meshes if we are imported a skeletal mesh
        if ( importedMesh.m_isSkeletalMesh )
        {
            bool hasSkeletalMeshes = false;
            for ( ufbx_mesh* pFoundMesh : m_pScene->meshes )
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

        for ( ufbx_mesh* pMesh : m_pScene->meshes )
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

                ReadMeshGeometry( importedMesh, pMesh, meshPart, geo );
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

    bool File::ReadMeshGeometry( FbxImportedMesh& importedMesh, ufbx_mesh* pMesh, ufbx_mesh_part const& meshPart, FbxImportedMesh::Geometry& geo ) const
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
                importedMesh.LogError( "Couldn't find skeleton for mesh (%s)!", pMesh->name.data );
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
                FbxImportedSkeleton tempSkeleton;
                if ( !ReadSkeleton( String( pSkeletonNode->name.data ), tempSkeleton ) )
                {
                    importedMesh.LogError( "Failed to read skeleton for mesh (%s)", pMesh->name.data );
                    return false;
                }
                importedMesh.m_skeleton = tempSkeleton;
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

    //-------------------------------------------------------------------------

    TUniquePtr<Materials> File::ReadMaterials() const
    {
        EE_ASSERT( IsValid() );

        TUniquePtr<Materials> pMaterials( EE::New<Materials>() );
        ReadAllMaterials( *pMaterials.get() );
        return pMaterials;
    }

    void File::ReadMaterialProperty( Material::Property& property, ufbx_material const& material, ufbx_material_pbr_map pbrMap, ufbx_material_fbx_map fbxMap, bool isColor ) const
    {
        auto CreatePropertyFromFBXMap = [] ( Material::Property& importedProperty, ufbx_material_map const& map, bool isColor )
        {
            importedProperty.m_isValueSet = map.has_value;
            importedProperty.m_value = Color( isColor ? ToFloat4( map.value_vec4 ) : Float4( float( map.value_real ) ) );

            ufbx_texture* pTexture = map.texture;
            if ( pTexture != nullptr )
            {
                char const* pFilename = ( pTexture->relative_filename.length > 0 ) ? pTexture->relative_filename.data : pTexture->filename.data;
                importedProperty.m_rawTexturePathStr = pFilename;
            }
        };

        //-------------------------------------------------------------------------

        ufbx_material_map const& pbr = material.pbr.maps[pbrMap];
        if ( pbr.has_value || pbr.texture_enabled || pbr.texture != nullptr )
        {
            CreatePropertyFromFBXMap( property, pbr, isColor );
            return;
        }

        ufbx_material_map const& fbx = material.fbx.maps[fbxMap];
        if ( fbx.has_value || fbx.texture_enabled || fbx.texture != nullptr )
        {
            CreatePropertyFromFBXMap( property, fbx, isColor );
        }
    }

    void File::ReadAllMaterials( Materials& materials ) const
    {
        auto ReadMaterial = [this] ( ufbx_material const& material, Material& importedMaterial )
        {
            ReadMaterialProperty( importedMaterial.m_albedo, material, UFBX_MATERIAL_PBR_BASE_COLOR, UFBX_MATERIAL_FBX_DIFFUSE_COLOR, true );
            ReadMaterialProperty( importedMaterial.m_metalness, material, UFBX_MATERIAL_PBR_METALNESS, UFBX_MATERIAL_FBX_SPECULAR_FACTOR, false );
            ReadMaterialProperty( importedMaterial.m_roughness, material, UFBX_MATERIAL_PBR_ROUGHNESS, UFBX_MATERIAL_FBX_SPECULAR_FACTOR, false );
            ReadMaterialProperty( importedMaterial.m_occlusion, material, UFBX_MATERIAL_PBR_AMBIENT_OCCLUSION, UFBX_MATERIAL_FBX_AMBIENT_COLOR, false );

            ReadMaterialProperty( importedMaterial.m_displacement, material, UFBX_MATERIAL_PBR_DISPLACEMENT_MAP, UFBX_MATERIAL_FBX_VECTOR_DISPLACEMENT, false );
            ReadMaterialProperty( importedMaterial.m_emissive, material, UFBX_MATERIAL_PBR_EMISSION_COLOR, UFBX_MATERIAL_FBX_EMISSION_COLOR, true );
            ReadMaterialProperty( importedMaterial.m_normal, material, UFBX_MATERIAL_PBR_NORMAL_MAP, UFBX_MATERIAL_FBX_NORMAL_MAP, false );

            //-------------------------------------------------------------------------

            ufbx_material_map const& emissionFactor = material.pbr.maps[UFBX_MATERIAL_PBR_EMISSION_FACTOR];
            if ( emissionFactor.has_value )
            {
                importedMaterial.m_emissiveStrength = (float) emissionFactor.value_real;
            }

            ufbx_material_map const& opacity = material.pbr.maps[UFBX_MATERIAL_PBR_OPACITY];
            if ( opacity.has_value )
            {
                importedMaterial.m_opacity = (float) opacity.value_real;
            }

            importedMaterial.m_isTwoSided = material.features.double_sided.enabled;
            importedMaterial.m_hasAlpha = ( importedMaterial.m_opacity < 1.0F ) || material.features.opacity.enabled;

            ufbx_material_map const& metalnessMap = material.pbr.maps[UFBX_MATERIAL_PBR_METALNESS];
            ufbx_material_map const& roughnessMap = material.pbr.maps[UFBX_MATERIAL_PBR_ROUGHNESS];
            bool const hasMetalnessRoughness = ( metalnessMap.texture_enabled && metalnessMap.texture != nullptr ) || ( roughnessMap.texture_enabled && roughnessMap.texture != nullptr );

            importedMaterial.m_isSpecularGlossiness = !hasMetalnessRoughness && material.fbx.maps[UFBX_MATERIAL_FBX_SPECULAR_COLOR].texture_enabled;
        };

        //-------------------------------------------------------------------------

        materials.SetSourcePath( m_sourcePath );

        for ( ufbx_material* pMaterial : m_pScene->materials )
        {
            Material& newMeshMaterial = materials.emplace_back();
            newMeshMaterial.m_ID = StringID( pMaterial->name.data );
            ReadMaterial( *pMaterial, newMeshMaterial );
        }
    }
}