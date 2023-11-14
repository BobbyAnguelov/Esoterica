#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Math/Transform.h"
#include "Base/Types/String.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/StringID.h"
#include "Base/Memory/Pointers.h"
#include <fbxsdk.h>

//-------------------------------------------------------------------------

namespace EE::Import
{
    class ImportedMesh;
    class ImportedAnimation;
    class ImportedSkeleton;
}

//-------------------------------------------------------------------------

namespace EE::Import::Fbx
{
    //-------------------------------------------------------------------------
    // An FBX scene context + helpers
    //-------------------------------------------------------------------------

    // This helper allows us to import an FBX file and perform all necessary coordinate conversion and scale adjustments
    // NB! This helper need all implementations to be inlined, as the FBXSDK doesnt work out of the box across DLL boundaries.

    // Implements axis system and unit system conversions from any FBX system to the EE system
    // EE Coordinate system is ( +Z up, -Y forward, Right Handed i.e. the same as Max/Maya Z-Up/Blender )
    // EE units are meters

    // Unit conversion is handled when we convert from FBX to EE types, since FBX bakes the conversion into the FBX transforms and we have to remove it afterwards

    class EE_ENGINETOOLS_API SceneContext
    {

    public:

        SceneContext() = default;
        SceneContext( FileSystem::Path const& filePath, float additionalScalingFactor = 1.0f );
        ~SceneContext();

        void LoadFile( FileSystem::Path const& filePath, float additionalScalingFactor = 1.0f );

        inline bool IsValid() const { return m_pManager != nullptr && m_pScene != nullptr && m_error.empty(); }

        inline bool HasErrorOccurred() const { return !m_error.empty(); }
        inline String const& GetErrorMessage() const { return m_error; }

        inline bool HasWarningOccurred() const { return !m_warning.empty(); }
        inline String const& GetWarningMessage() const { return m_warning; }

        inline Axis GetOriginalUpAxis() const { return m_originalUpAxis; }

        // Scene helpers
        //-------------------------------------------------------------------------

        void FindAllNodesOfType( fbxsdk::FbxNodeAttribute::EType nodeType, TVector<fbxsdk::FbxNode*>& results ) const;
        fbxsdk::FbxNode* FindNodeOfTypeByName( fbxsdk::FbxNodeAttribute::EType nodeType, char const* pNodeName ) const;

        void FindAllAnimStacks( TVector<fbxsdk::FbxAnimStack*>& results ) const;
        fbxsdk::FbxAnimStack* FindAnimStack( char const* pTakeName ) const;

        inline FbxAMatrix GetNodeGlobalTransform( fbxsdk::FbxNode* pNode ) const
        {
            EE_ASSERT( pNode != nullptr );

            fbxsdk::FbxVector4 const gT = pNode->GetGeometricTranslation( fbxsdk::FbxNode::eSourcePivot );
            fbxsdk::FbxVector4 const gR = pNode->GetGeometricRotation( fbxsdk::FbxNode::eSourcePivot );
            fbxsdk::FbxVector4 const gS = pNode->GetGeometricScaling( fbxsdk::FbxNode::eSourcePivot );

            fbxsdk::FbxAMatrix geometricTransform( gT, gR, gS );
            geometricTransform = pNode->EvaluateGlobalTransform() * geometricTransform;
            return geometricTransform;
        }

        // Conversion Functions
        //-------------------------------------------------------------------------

        inline Transform ConvertMatrixToTransform( fbxsdk::FbxAMatrix const& fbxMatrix ) const
        {
            auto const Q = fbxMatrix.GetQ();
            auto const T = fbxMatrix.GetT();
            auto const S = fbxMatrix.GetS();

            EE_ASSERT( Math::IsNearEqual( S[0], S[1], (double) Math::LargeEpsilon ) && Math::IsNearEqual( S[1], S[2], (double) Math::LargeEpsilon ) );

            auto const rotation = Quaternion( (float) Q[0], (float) Q[1], (float) Q[2], (float) Q[3] ).GetNormalized();
            auto const translation = Vector( (float) T[0], (float) T[1], (float) T[2] );

            Transform transform( rotation, translation, (float) S[0] );
            transform.SanitizeScaleValue();
            return transform;
        }

        inline Transform ConvertMatrixToTransformAndFixScale( fbxsdk::FbxAMatrix const& fbxMatrix ) const
        {
            auto const Q = fbxMatrix.GetQ();
            auto const T = fbxMatrix.GetT();
            auto const S = fbxMatrix.GetS();

            EE_ASSERT( S[0] == S[1] && S[1] == S[2] );

            auto const rotation = Quaternion( (float) Q[0], (float) Q[1], (float) Q[2], (float) Q[3] ).GetNormalized();
            auto const translation = Vector( (float) T[0] * m_scaleConversionMultiplier, (float) T[1] * m_scaleConversionMultiplier, (float) T[2] * m_scaleConversionMultiplier );

            Transform transform( rotation, translation, (float) S[0] );
            transform.SanitizeScaleValue();
            return transform;
        }

        inline Vector ConvertVector3( fbxsdk::FbxVector4 const& fbxVec ) const
        {
            return  Vector( (float) fbxVec[0], (float) fbxVec[1], (float) fbxVec[2] );
        }

        inline Vector ConvertVector3AndFixScale( fbxsdk::FbxVector4 const& fbxVec ) const
        {
            return Vector( (float) fbxVec[0] * m_scaleConversionMultiplier, (float) fbxVec[1] * m_scaleConversionMultiplier, (float) fbxVec[2] * m_scaleConversionMultiplier );
        }

        inline Vector ConvertVector4( fbxsdk::FbxVector4 const& fbxVec ) const
        {
            return  Vector( (float) fbxVec[0], (float) fbxVec[1], (float) fbxVec[2], (float) fbxVec[3] );
        }

        inline Vector ConvertVectorAndFixScale4( fbxsdk::FbxVector4 const& fbxVec ) const
        {
            return Vector( (float) fbxVec[0] * m_scaleConversionMultiplier, (float) fbxVec[1] * m_scaleConversionMultiplier, (float) fbxVec[2] * m_scaleConversionMultiplier, (float) fbxVec[3] * m_scaleConversionMultiplier );
        }

        // Scale correction
        //-------------------------------------------------------------------------

        inline float GetScaleConversionMultiplier() const { return m_scaleConversionMultiplier; }

    private:

        void Initialize( FileSystem::Path const& filePath, float additionalScalingFactor );
        void Shutdown();

        SceneContext( SceneContext const& ) = delete;
        SceneContext& operator=( SceneContext const& ) = delete;

    public:

        fbxsdk::FbxManager*                 m_pManager = nullptr;
        fbxsdk::FbxScene*                   m_pScene = nullptr;
        fbxsdk::FbxGeometryConverter*       m_pGeometryConvertor = nullptr;

    private:

        FileSystem::Path                    m_filePath;
        String                              m_error;
        String                              m_warning;
        Axis                                m_originalUpAxis = Axis::Z;
        float                               m_scaleConversionMultiplier = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------

    // This is needed to deal with multiple nested namespace as the FBX SDK default function only remove the first namespace
    EE_FORCE_INLINE char const* GetNameWithoutNamespace( fbxsdk::FbxObject* pObject )
    {
        return FbxNode::RemovePrefix( pObject->GetNameOnly() );
    }

    //-------------------------------------------------------------------------
    // Import Functions
    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<ImportedSkeleton> ReadSkeleton( FileSystem::Path const& sourceFilePath, String const& skeletonRootBoneName );
    EE_ENGINETOOLS_API TUniquePtr<ImportedAnimation> ReadAnimation( FileSystem::Path const& animationFilePath, ImportedSkeleton const& importedSkeleton, String const& animationName );
    EE_ENGINETOOLS_API TUniquePtr<ImportedMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude );
    EE_ENGINETOOLS_API TUniquePtr<ImportedMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences );
}