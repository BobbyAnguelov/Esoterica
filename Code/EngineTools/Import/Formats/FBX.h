#pragma once

#include "FormatFile.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "EngineTools/Import/ImportedMaterial.h"
#include "EngineTools/ThirdParty/ufbx/ufbx.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

struct ufbx_scene;

//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    class FbxImportedSkeleton;
    class FbxImportedAnimation;
    class FbxImportedMesh;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API File : public SceneFile
    {
    public:

        static Quaternion const s_correctiveYtoZ;

    public:

        File( Source const& source, FileSystem::Path const& sourceDataDirectoryPath );
        ~File();

        // File Status
        //-------------------------------------------------------------------------

        // Did the source file load correctly?
        virtual bool IsValid() const override { return SceneFile::IsValid() && m_pScene != nullptr; }

        // Are we allowed to import animations or skinned meshes from this file (based on source coordinate system)
        inline bool IsBoneOrSkinImportAllowed() const { return m_isBoneOrSkinImportAllowed; }

        ufbx_scene* GetScene() const { return m_pScene; }

        // Conversion
        //-------------------------------------------------------------------------

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

        // Import Functions
        //-------------------------------------------------------------------------

        virtual void GetFileContents( Contents& outContents ) const override;
        virtual TUniquePtr<Skeleton> ReadSkeleton( String const& skeletonRootBoneName ) const override;
        virtual TUniquePtr<Animation> ReadAnimation( Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate ) const override;
        virtual TUniquePtr<Mesh> ReadStaticMesh( TVector<String> const& meshesToInclude ) const override;
        virtual TUniquePtr<Mesh> ReadSkeletalMesh( TVector<String> const& meshesToInclude ) const override;
        virtual TUniquePtr<Materials> ReadMaterials() const override;

    protected:

        // Helpers
        //-------------------------------------------------------------------------

        // This finds all the nodes of this type that are roots of a branch
        inline void FindAllRootNodes( ufbx_node* pCurrentNode, ufbx_element_type nodeType, TVector<ufbx_node*>& results ) const
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

        // Skeleton
        //-------------------------------------------------------------------------

        bool ReadSkeleton( String const& skeletonRootBoneName, FbxImportedSkeleton& skeleton ) const;
        bool ReadBoneHierarchy( FbxImportedSkeleton& skeleton, ufbx_node* pNode, int32_t parentIdx ) const;

        // Animation
        //-------------------------------------------------------------------------

        bool ReadAnimationTrackData( FbxImportedAnimation& animation, AnimationClip& animClip, ufbx_anim_stack *pAnimStack ) const;

        // Mesh
        //-------------------------------------------------------------------------

        void ReadAllSubmeshes( FbxImportedMesh& importedMesh, TVector<String> const& meshesToInclude ) const;
        bool ReadMeshGeometry( FbxImportedMesh& importedMesh, ufbx_mesh* pMesh, ufbx_mesh_part const& meshPart, Mesh::Geometry& geo ) const;

        // Materials
        //-------------------------------------------------------------------------

        void ReadAllMaterials( Materials& materials ) const;
        void ReadMaterialProperty( Material::Property& property, ufbx_material const& material, ufbx_material_pbr_map pbrMap, ufbx_material_fbx_map fbxMap, bool isColor ) const;

    private:

        ufbx_scene*                 m_pScene = nullptr;
        bool                        m_isBoneOrSkinImportAllowed = true;
    };
}