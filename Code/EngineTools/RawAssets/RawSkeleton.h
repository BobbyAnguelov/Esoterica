#pragma once

#include "RawAsset.h"
#include "Base/Math/Transform.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class EE_ENGINETOOLS_API RawSkeleton : public RawAsset
    {

    public:

        struct BoneData
        {
            BoneData( const char* pName );

        public:

            StringID            m_name;
            Transform           m_localTransform = Transform::Identity;
            Transform           m_globalTransform = Transform::Identity;
            StringID            m_parentBoneName;
            int32_t             m_parentBoneIdx = InvalidIndex;
        };

    public:

        virtual bool IsValid() const override final { return !m_bones.empty(); }

        inline StringID const& GetName() const { return m_name; }

        inline BoneData const& GetRootBone() const { return m_bones[0]; }
        inline StringID const& GetRootBoneName() const { return m_bones[0].m_name; }

        inline TVector<BoneData> const& GetBoneData() const { return m_bones; }
        inline BoneData const& GetBoneData( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx]; }
        inline StringID const& GetBoneName( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_name; }
        inline uint32_t GetNumBones() const { return (uint32_t) m_bones.size(); }
        inline uint32_t GetNumBonesToSampleAtLowLOD() const { return m_numBonesToSampleAtLowLOD; }

        inline StringID GetBoneID( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_name; }
        int32_t GetBoneIndex( StringID const& boneName ) const;
        inline int32_t GetParentBoneIndex( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_parentBoneIdx; }
        inline Transform const& GetLocalTransform( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_localTransform; }
        inline Transform const& GetGlobalTransform( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_globalTransform; }

        bool IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const;

        void Finalize( TVector<StringID> const& highLODBones );

    protected:

        void CalculateLocalTransforms();
        void CalculateGlobalTransforms();

    protected:

        StringID                m_name;
        TVector<BoneData>       m_bones;
        uint32_t                m_numBonesToSampleAtLowLOD = 0;
    };
}