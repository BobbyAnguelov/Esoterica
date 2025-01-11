#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/ThirdParty/cgltf/cgltf.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Math/Transform.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class ImportedMesh;
    class ImportedAnimation;
    class ImportedSkeleton;
}

//-------------------------------------------------------------------------

namespace EE::Import::gltf
{
    //-------------------------------------------------------------------------
    // A GLTF scene context + helpers
    //-------------------------------------------------------------------------

    // Implements axis system and unit system conversions from the gltf coordinate system ( Right-Handed, Y-Up, -Z as forward ) to the EE system
    // EE Coordinate system is ( +Z up, -Y forward, Right-Handed i.e. the same as Max/Maya Z-Up/Blender )
    // EE units are meters

    // Luckily all that's need to convert gltf scenes to EE is a -90 rotation on X

    class EE_ENGINETOOLS_API SceneContext
    {
    public:

        SceneContext() = default;
        SceneContext( FileSystem::Path const& filePath, float additionalScalingFactor = 1.0f );
        ~SceneContext();

        void LoadFile( FileSystem::Path const& filePath, float additionalScalingFactor = 1.0f );

        inline bool IsValid() const { return m_pSceneData != nullptr; }

        inline bool HasErrorOccurred() const { return !m_error.empty(); }
        inline String const& GetErrorMessage() const { return m_error; }

        inline bool HasWarningOccurred() const { return !m_warning.empty(); }
        inline String const& GetWarningMessage() const { return m_warning; }

        cgltf_data const* GetSceneData() const { return m_pSceneData; }

        // Conversion Functions
        //-------------------------------------------------------------------------

        inline Transform ConvertMatrix( cgltf_float const gltfMatrix[16] ) const
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

        inline Transform GetNodeTransform( cgltf_node* pNode, bool includeParentTransform = false ) const
        {
            EE_ASSERT( pNode != nullptr );

            Transform t;

            if ( pNode->has_matrix )
            {
                t = ConvertMatrix( pNode->matrix );
            }
            else
            {
                Quaternion rotation = Quaternion::Identity;
                if ( pNode->has_rotation )
                {
                    rotation = Quaternion( pNode->rotation[0], pNode->rotation[1], pNode->rotation[2], pNode->rotation[3] );
                }

                Vector translation = Vector::Zero;
                if ( pNode->has_translation )
                {
                    translation = Vector( pNode->translation[0], pNode->translation[1], pNode->translation[2] );
                }

                float scale = 1.0f;
                if ( pNode->has_scale )
                {
                    // TODO: log warning
                    EE_ASSERT( pNode->scale[0] != pNode->scale[1] || pNode->scale[1] != pNode->scale[2] );
                    scale = pNode->scale[0];
                }

                t = Transform( rotation, translation, scale );
            }

            if ( includeParentTransform && pNode->parent != nullptr )
            {
                return t * GetNodeTransform( pNode->parent, includeParentTransform );
            }

            return t;
        }

    private:

        void Initialize( FileSystem::Path const& filePath, float additionalScalingFactor );
        void Shutdown();

    private:

        cgltf_data*                 m_pSceneData = nullptr;
        String                      m_error;
        String                      m_warning;
        Transform                   m_upAxisCorrectionTransform = Transform( AxisAngle( Float3::UnitX, Degrees( 90 ) ) );
        float                       m_scaleConversionMultiplier = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Import Functions
    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<ImportedSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName );
    EE_ENGINETOOLS_API TUniquePtr<ImportedAnimation> ReadAnimation( FileSystem::Path const& animationFilePath, ImportedSkeleton const& ImportedSkeleton, String const& animationName = String() );
    EE_ENGINETOOLS_API TUniquePtr<ImportedMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude );
    EE_ENGINETOOLS_API TUniquePtr<ImportedMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences );
}