#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Import/ImporterSource.h"
#include "EngineTools/ThirdParty/ufbx/ufbx.h"
#include "Base/Types/String.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Memory/UniquePtr.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

struct ufbx_scene;

namespace EE::Import
{
    class Mesh;
    class Animation;
    class Skeleton;
}

//-------------------------------------------------------------------------

namespace EE::Import::UFbx
{
    //-------------------------------------------------------------------------
    // A FBX scene context + helpers
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API SceneContext
    {
    public:

        static Quaternion const s_correctiveYtoZ;

    public:

        SceneContext() = default;
        SceneContext( Source const& source );
        ~SceneContext();

        // Did the source file load correctly?
        inline bool IsValid() const { return m_pScene != nullptr; }

        // Are we allowed to import animations or skinned meshes from this file (based on source coordinate system)
        inline bool IsBoneOrSkinImportAllowed() const { return m_isBoneOrSkinImportAllowed; }

        ufbx_scene* GetScene() const { return m_pScene; }

        inline bool HasErrorOccurred() const { return !m_error.empty(); }
        inline String const& GetErrorMessage() const { return m_error; }

        inline bool HasWarningOccurred() const { return !m_warning.empty(); }
        inline String const& GetWarningMessage() const { return m_warning; }

    private:

        String                      m_error;
        String                      m_warning;
        ufbx_scene*                 m_pScene = nullptr;
        bool                        m_isBoneOrSkinImportAllowed = true;
    };

    //-------------------------------------------------------------------------
    // Conversion Functions
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

    //-------------------------------------------------------------------------
    // Helpers
    //-------------------------------------------------------------------------

     // This finds all the nodes of this type that are roots of a branch
    inline void FindAllRootNodes( ufbx_node* pCurrentNode, ufbx_element_type nodeType, TVector<ufbx_node*>& results )
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

    //-------------------------------------------------------------------------
    // Import Functions
    //-------------------------------------------------------------------------

    EE_ENGINETOOLS_API TUniquePtr<Skeleton> ReadSkeleton( Source const& source, String const& skeletonRootBoneName );
    EE_ENGINETOOLS_API TUniquePtr<Animation> ReadAnimation( Source const& source, Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate );
    EE_ENGINETOOLS_API TUniquePtr<Mesh> ReadStaticMesh( Source const& source, TVector<String> const& meshesToInclude );
    EE_ENGINETOOLS_API TUniquePtr<Mesh> ReadSkeletalMesh( Source const& source, TVector<String> const& meshesToInclude );
}