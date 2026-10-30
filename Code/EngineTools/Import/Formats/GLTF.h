#pragma once

#include "FormatFile.h"
#include "EngineTools/ThirdParty/cgltf/cgltf.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    class gltfImportedSkeleton;
    class gltfImportedAnimation;
    class gltfImportedMesh;

    //-------------------------------------------------------------------------
    // A GLTF scene context + helpers
    //-------------------------------------------------------------------------

    // Implements axis system and unit system conversions from the gltf coordinate system ( Right-Handed, Y-Up, -Z as forward ) to the EE system
    // EE Coordinate system is ( +Z up, -Y forward, Right-Handed i.e. the same as Max/Maya Z-Up/Blender )
    // EE units are meters

    // Luckily all that's need to convert gltf scenes to EE is a -90 rotation on X

    class EE_ENGINETOOLS_API File : public SceneFile
    {
        constexpr static char const * const s_defaultName = "Default";

    public:

        File( Source const& source, FileSystem::Path const& sourceDataDirectoryPath );
        ~File();

        // File Status
        //-------------------------------------------------------------------------

        // Did the source file load correctly?
        virtual bool IsValid() const override { return SceneFile::IsValid() && m_pSceneData != nullptr; }

        cgltf_data const* GetScene() const { return m_pSceneData; }

        // Conversion
        //-------------------------------------------------------------------------

        inline Transform ToTransform( cgltf_float const gltfMatrix[16] ) const
        {
            Matrix engineMatrix
            (
                (float) gltfMatrix[0], (float) gltfMatrix[1], (float) gltfMatrix[2], (float) gltfMatrix[3],
                (float) gltfMatrix[4], (float) gltfMatrix[5], (float) gltfMatrix[6], (float) gltfMatrix[7],
                (float) gltfMatrix[8], (float) gltfMatrix[9], (float) gltfMatrix[10], (float) gltfMatrix[11],
                (float) gltfMatrix[12], (float) gltfMatrix[13], (float) gltfMatrix[14], (float) gltfMatrix[15]
            );

            Transform convertedTransform = Transform( engineMatrix );
            convertedTransform.SanitizeScaleValue();
            return convertedTransform;
        }

        // Up Axis Correction
        //-------------------------------------------------------------------------

        inline Transform GetUpAxisCorrectionTransform() const { return m_upAxisCorrectionTransform; }

        inline Transform ApplyUpAxisCorrection( Transform const& matrix ) const
        {
            Transform correctedMatrix = matrix * m_upAxisCorrectionTransform;
            return correctedMatrix;
        }

        inline Vector ApplyUpAxisCorrection( Vector const& point ) const
        {
            Vector correctedVector = m_upAxisCorrectionTransform.TransformPoint( point );
            return correctedVector;
        }

        // Helpers
        //-------------------------------------------------------------------------

        Transform GetNodeTransform( cgltf_node* pNode, bool includeParentTransform = false ) const;

        // Import Functions
        //-------------------------------------------------------------------------

        virtual void GetFileContents( Contents& outContents ) const override;
        virtual TUniquePtr<Skeleton> ReadSkeleton( String const& skeletonRootBoneName ) const override;
        virtual TUniquePtr<Animation> ReadAnimation( Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate ) const override;
        virtual TUniquePtr<Mesh> ReadStaticMesh( TVector<String> const& meshesToInclude ) const override;
        virtual TUniquePtr<Mesh> ReadSkeletalMesh( TVector<String> const& meshesToInclude ) const override;
        virtual TUniquePtr<Materials> ReadMaterials() const override;

    private:

        // Skeleton
        //-------------------------------------------------------------------------

        void ReadBoneHierarchy( gltfImportedSkeleton& skeleton, cgltf_node* pNode, int32_t parentIdx ) const;

        // Animation
        //-------------------------------------------------------------------------

        void ReadAnimationData( gltfImportedAnimation& animation, AnimationClip& animClip, cgltf_animation const* pAnimation ) const;

        // Mesh
        //-------------------------------------------------------------------------

        void ReadMeshGeometry( gltfImportedMesh& importedMesh, cgltf_mesh const& mesh ) const;
        void CreateSubmeshesFromNodes( gltfImportedMesh& importedMesh, THashMap<cgltf_mesh const*, struct MeshEntry> const& meshMap ) const;
        void ReadBindPoseHierarchy( gltfImportedSkeleton& skeleton, TVector<StringID> const& skinnedJoints, TVector<Transform> const& bindPose, cgltf_node* pNode, int32_t parentIdx ) const;

        // Materials
        //-------------------------------------------------------------------------

        void ReadAllMaterials( Materials& materials ) const;

    private:

        cgltf_data*                 m_pSceneData = nullptr;
        Transform                   m_upAxisCorrectionTransform = Transform( AxisAngle( Float3::UnitX, Degrees( 90 ) ) );
    };
}