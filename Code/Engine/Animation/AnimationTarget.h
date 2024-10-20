#pragma once

#include "Base/Math/Transform.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Pose;

    //-------------------------------------------------------------------------

    class Target
    {
        EE_SERIALIZE( m_transform, m_boneID, m_isUsingBoneSpaceOffsets, m_hasOffsets, m_isSet );

    public:

        Target() = default;

        explicit Target( Transform const& T )
            : m_transform( T.GetRotation(), T.GetTranslation() ) // Ignore scale for targets
            , m_isSet( true )
        {}

        explicit Target( Quaternion const& R, Vector const& T )
            : m_transform( R, T )
            , m_isSet( true )
        {}

        explicit Target( StringID boneID )
            : m_boneID( boneID )
            , m_isBoneTarget( true )
            , m_isSet( true )
        {}

        inline void Reset() { m_isSet = false; }
        inline bool IsTargetSet() const { return m_isSet; }

        inline bool IsBoneTarget() const { return m_isBoneTarget; }
        inline StringID GetBoneID() const { EE_ASSERT( IsBoneTarget() ); return m_boneID; }

        // Offsets
        //-------------------------------------------------------------------------
        // Only make sense for bone targets

        inline bool HasOffsets() const { EE_ASSERT( m_isBoneTarget ); return m_hasOffsets; }
        inline Quaternion GetRotationOffset() const { EE_ASSERT( m_isSet && m_isBoneTarget && m_hasOffsets ); return m_transform.GetRotation(); }
        inline Vector GetTranslationOffset() const { EE_ASSERT( m_isSet && m_isBoneTarget && m_hasOffsets ); return m_transform.GetTranslation(); }
        inline bool IsUsingBoneSpaceOffsets() const { EE_ASSERT( m_isBoneTarget ); return m_isUsingBoneSpaceOffsets; }
        inline bool IsUsingWorldSpaceOffsets() const { EE_ASSERT( m_isBoneTarget ); return !m_isUsingBoneSpaceOffsets; }

        // Set an offset for a bone target - user specifies if the offsets are in world space or bone space
        inline void SetOffsets( Quaternion const& rotationOffset, Vector const& translationOffset, bool useBoneSpaceOffsets = true )
        {
            EE_ASSERT( m_isSet && m_isBoneTarget ); // Offsets only make sense for bone targets
            m_transform = Transform( rotationOffset, translationOffset );
            m_isUsingBoneSpaceOffsets = useBoneSpaceOffsets;
            m_hasOffsets = true;
        }

        void ClearOffsets()
        {
            EE_ASSERT( m_isSet && m_isBoneTarget ); // Offsets only make sense for bone targets
            m_hasOffsets = false;
        }

        // Transform
        //-------------------------------------------------------------------------

        inline Transform const& GetTransform() const
        {
            EE_ASSERT( m_isSet && !m_isBoneTarget );
            return m_transform;
        }

        bool TryGetTransform( Pose const* pPose, Transform& outTransform ) const;

    private:

        Transform           m_transform = Transform::Identity;
        StringID            m_boneID;
        bool                m_isBoneTarget = false;
        bool                m_isUsingBoneSpaceOffsets = true;
        bool                m_hasOffsets = false;
        bool                m_isSet = false;
    };
}