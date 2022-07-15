#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Math/Transform.h"
#include "System/Types/String.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Types/StringID.h"
#include <fbxsdk.h>

//-------------------------------------------------------------------------

using namespace fbxsdk;

//-------------------------------------------------------------------------
// An FBX scene context + helpers
//-------------------------------------------------------------------------

// This helper allows us to import an FBX file and perform all necessary coordinate conversion and scale adjustments
// NB! This helper need all implementations to be inlined, as the FBXSDK doesnt work out of the box across DLL boundaries.

// Implements axis system and unit system conversions from any FBX system to the EE system
// EE Coordinate system is ( +Z up, -Y forward, Right Handed i.e. the same as Max/Maya Z-Up/Blender )
// EE units are meters

// Unit conversion is handled when we convert from FBX to EE types, since FBX bakes the conversion into the FBX transforms and we have to remove it afterwards

//-------------------------------------------------------------------------

namespace EE::Fbx
{
    class EE_ENGINETOOLS_API FbxSceneContext
    {

    public:

        FbxSceneContext( FileSystem::Path const& filePath );
        ~FbxSceneContext();

        inline bool IsValid() const { return m_pManager != nullptr && m_pScene != nullptr && m_error.empty(); }
        inline String const& GetErrorMessage() const { return m_error; }

        inline Axis GetOriginalUpAxis() const { return m_originalUpAxis; }

        // Scene helpers
        //-------------------------------------------------------------------------

        void FindAllNodesOfType( FbxNodeAttribute::EType nodeType, TVector<FbxNode*>& results ) const;
        FbxNode* FindNodeOfTypeByName( FbxNodeAttribute::EType nodeType, char const* pNodeName ) const;

        void FindAllAnimStacks( TVector<FbxAnimStack*>& results ) const;
        FbxAnimStack* FindAnimStack( char const* pTakeName ) const;

        inline FbxAMatrix GetNodeGlobalTransform( FbxNode* pNode ) const
        {
            EE_ASSERT( pNode != nullptr );

            FbxVector4 const gT = pNode->GetGeometricTranslation( FbxNode::eSourcePivot );
            FbxVector4 const gR = pNode->GetGeometricRotation( FbxNode::eSourcePivot );
            FbxVector4 const gS = pNode->GetGeometricScaling( FbxNode::eSourcePivot );

            FbxAMatrix geometricTransform( gT, gR, gS );
            geometricTransform = pNode->EvaluateGlobalTransform() * geometricTransform;
            return geometricTransform;
        }

        // Conversion Functions
        //-------------------------------------------------------------------------

        inline Transform ConvertMatrixToTransform( FbxAMatrix const& fbxMatrix ) const
        {
            auto const Q = fbxMatrix.GetQ();
            auto const T = fbxMatrix.GetT();
            auto const S = fbxMatrix.GetS();

            auto const rotation = Quaternion( (float) Q[0], (float) Q[1], (float) Q[2], (float) Q[3] ).GetNormalized();
            auto const translation = Vector( (float) T[0], (float) T[1], (float) T[2] );
            auto const scale = Vector( (float) S[0], (float) S[1], (float) S[2] );

            Transform transform( rotation, translation, scale );
            transform.SanitizeScaleValues();
            return transform;
        }

        inline Transform ConvertMatrixToTransformAndFixScale( FbxAMatrix const& fbxMatrix ) const
        {
            auto const Q = fbxMatrix.GetQ();
            auto const T = fbxMatrix.GetT();
            auto const S = fbxMatrix.GetS();

            auto const rotation = Quaternion( (float) Q[0], (float) Q[1], (float) Q[2], (float) Q[3] ).GetNormalized();
            auto const translation = Vector( (float) T[0] * m_scaleConversionMultiplier, (float) T[1] * m_scaleConversionMultiplier, (float) T[2] * m_scaleConversionMultiplier );
            auto const scale = Vector( (float) S[0], (float) S[1], (float) S[2] );

            Transform transform( rotation, translation, scale );
            transform.SanitizeScaleValues();
            return transform;
        }

        inline Vector ConvertVector3( FbxVector4 const& fbxVec ) const
        {
            return  Vector( (float) fbxVec[0], (float) fbxVec[1], (float) fbxVec[2] );
        }

        inline Vector ConvertVector3AndFixScale( FbxVector4 const& fbxVec ) const
        {
            return Vector( (float) fbxVec[0] * m_scaleConversionMultiplier, (float) fbxVec[1] * m_scaleConversionMultiplier, (float) fbxVec[2] * m_scaleConversionMultiplier );
        }

        inline Vector ConvertVector4( FbxVector4 const& fbxVec ) const
        {
            return  Vector( (float) fbxVec[0], (float) fbxVec[1], (float) fbxVec[2], (float) fbxVec[3] );
        }

        inline Vector ConvertVectorAndFixScale4( FbxVector4 const& fbxVec ) const
        {
            return Vector( (float) fbxVec[0] * m_scaleConversionMultiplier, (float) fbxVec[1] * m_scaleConversionMultiplier, (float) fbxVec[2] * m_scaleConversionMultiplier, (float) fbxVec[3] * m_scaleConversionMultiplier );
        }

        // Scale correction
        //-------------------------------------------------------------------------

        inline float GetScaleConversionMultiplier() const { return m_scaleConversionMultiplier; }

    private:

        FbxSceneContext() = delete;
        FbxSceneContext( FbxSceneContext const& ) = delete;
        FbxSceneContext& operator=( FbxSceneContext const& ) = delete;

    public:

        FbxManager*                 m_pManager = nullptr;
        FbxScene*                   m_pScene = nullptr;

    private:

        String                      m_error;
        Axis                        m_originalUpAxis = Axis::Z;
        float                       m_scaleConversionMultiplier = 1.0f;
    };
}