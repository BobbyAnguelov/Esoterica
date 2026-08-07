#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/ThirdParty/cgltf/cgltf.h"
#include "EngineTools/Import/ImporterSource.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Math/Transform.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class Mesh;
    class Animation;
    class Skeleton;
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
        SceneContext( Source const& source );
        ~SceneContext();

        inline bool IsValid() const { return m_pSceneData != nullptr; }

        inline bool HasErrorOccurred() const { return !m_error.empty(); }
        inline String const& GetErrorMessage() const { return m_error; }

        inline bool HasWarningOccurred() const { return !m_warning.empty(); }
        inline String const& GetWarningMessage() const { return m_warning; }

        cgltf_data const* GetScene() const { return m_pSceneData; }

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

    private:

        cgltf_data*                 m_pSceneData = nullptr;
        String                      m_error;
        String                      m_warning;
        Transform                   m_upAxisCorrectionTransform = Transform( AxisAngle( Float3::UnitX, Degrees( 90 ) ) );
    };

    //-------------------------------------------------------------------------
    // Conversion Functions
    //-------------------------------------------------------------------------

    inline Transform ToTransform( cgltf_float const gltfMatrix[16] )
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

    //-------------------------------------------------------------------------
    // Import Functions
    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName );
    EE_ENGINETOOLS_API TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName = String() );
    EE_ENGINETOOLS_API TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude );
    EE_ENGINETOOLS_API TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude );
}