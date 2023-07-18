#pragma once

#include "AnimationSkeleton.h"
#include "Base/Math/Math.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BoneMask;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Pose
    {
        friend class Blender;
        friend class AnimationClip;

    public:

        enum class Type : uint8_t
        {
            None = 0,
            ReferencePose,
            ZeroPose
        };

        enum class State : uint8_t
        {
            Unset = 0,
            Pose,
            ReferencePose,
            ZeroPose,
            AdditivePose
        };

    public:

        Pose( Skeleton const* pSkeleton, Type initialPoseType = Type::ReferencePose );

        Pose( Pose&& rhs );
        Pose( Pose const& rhs );
        Pose& operator=( Pose&& rhs );
        Pose& operator=( Pose const& rhs );

        void CopyFrom( Pose const& rhs );
        EE_FORCE_INLINE void CopyFrom( Pose const* pRhs ) { CopyFrom( *pRhs ); }

        //-------------------------------------------------------------------------

        inline int32_t GetNumBones() const { return m_pSkeleton->GetNumBones(); }
        inline Skeleton const* GetSkeleton() const { return m_pSkeleton; }

        // Pose state
        //-------------------------------------------------------------------------

        void Reset( Type initState = Type::None, bool calcGlobalPose = false );

        inline bool IsPoseSet() const { return m_state != State::Unset; }
        inline bool IsReferencePose() const { return m_state == State::ReferencePose; }
        inline bool IsZeroPose() const { return m_state == State::ZeroPose; }
        inline bool IsAdditivePose() const { return m_state == State::AdditivePose; }

        // Local Transforms
        //-------------------------------------------------------------------------

        TVector<Transform> const& GetTransforms() const { return m_localTransforms; }

        inline Transform const& GetTransform( int32_t boneIdx ) const
        {
            EE_ASSERT( boneIdx < GetNumBones() );
            return m_localTransforms[boneIdx];
        }

        inline void SetTransform( int32_t boneIdx, Transform const& transform )
        {
            EE_ASSERT( boneIdx < GetNumBones() && boneIdx >= 0 );
            m_localTransforms[boneIdx] = transform;
            MarkAsValidPose();
        }

        inline void SetRotation( int32_t boneIdx, Quaternion const& rotation )
        {
            EE_ASSERT( boneIdx < GetNumBones() && boneIdx >= 0 );
            m_localTransforms[boneIdx].SetRotation( rotation );
            MarkAsValidPose();
        }

        inline void SetTranslation( int32_t boneIdx, Float3 const& translation )
        {
            EE_ASSERT( boneIdx < GetNumBones() && boneIdx >= 0 );
            m_localTransforms[boneIdx].SetTranslation( translation );
            MarkAsValidPose();
        }

        // Set the scale for a given bone, note will change pose state to "Pose" if not already set
        inline void SetScale( int32_t boneIdx, float uniformScale )
        {
            EE_ASSERT( boneIdx < GetNumBones() && boneIdx >= 0 );
            m_localTransforms[boneIdx].SetScale( uniformScale );
            MarkAsValidPose();
        }

        // Global Transform Cache
        //-------------------------------------------------------------------------

        inline bool HasGlobalTransforms() const { return !m_globalTransforms.empty(); }
        inline void ClearGlobalTransforms() { m_globalTransforms.clear(); }
        inline TVector<Transform> const& GetGlobalTransforms() const { return m_globalTransforms; }
        void CalculateGlobalTransforms();
        Transform GetGlobalTransform( int32_t boneIdx ) const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform, Color color = Colors::HotPink, float lineThickness = 2.0f, BoneMask const* pBoneMask = nullptr, bool detailedView = false ) const;
        #endif

    private:

        Pose() = delete;

        void SetToReferencePose( bool setGlobalPose );
        void SetToZeroPose( bool setGlobalPose );

        EE_FORCE_INLINE void MarkAsValidPose()
        {
            if ( m_state != State::Pose && m_state != State::AdditivePose )
            {
                m_state = State::Pose;
            }
        }

    private:

        Skeleton const*             m_pSkeleton;                // The skeleton for this pose
        TVector<Transform>          m_localTransforms;          // Parent-space transforms
        TVector<Transform>          m_globalTransforms;         // Character-space transforms
        State                       m_state = State::Unset;     // Pose state
    };
}