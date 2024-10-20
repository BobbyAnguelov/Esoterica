#pragma once

#include "ImportedData.h"
#include "Base/Math/Transform.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API ImportedSkeleton : public ImportedData
    {

    public:

        struct Bone
        {
            Bone( const char* pName );

        public:

            StringID            m_name;
            Transform           m_parentSpaceTransform = Transform::Identity;
            Transform           m_modelSpaceTransform = Transform::Identity;
            StringID            m_parentBoneName;
            int32_t             m_parentBoneIdx = InvalidIndex;
        };

    public:

        virtual bool IsValid() const override final { return !m_bones.empty(); }

        inline StringID const& GetName() const { return m_name; }

        inline Bone const& GetRootBone() const { return m_bones[0]; }
        inline StringID const& GetRootBoneName() const { return m_bones[0].m_name; }

        inline TVector<Bone> const& GetBoneData() const { return m_bones; }
        inline Bone const& GetBoneData( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx]; }
        inline StringID const& GetBoneName( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_name; }
        inline uint32_t GetNumBones() const { return (uint32_t) m_bones.size(); }
        inline uint32_t GetNumBonesToSampleAtLowLOD() const { return m_numBonesToSampleAtLowLOD; }

        inline StringID GetBoneID( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_name; }
        int32_t GetBoneIndex( StringID const& boneName ) const;
        inline int32_t GetParentBoneIndex( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_parentBoneIdx; }
        inline Transform const& GetParentSpaceTransform( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_parentSpaceTransform; }
        inline Transform const& GetModelSpaceTransform( int32_t boneIdx ) const { EE_ASSERT( boneIdx >= 0 && boneIdx < m_bones.size() ); return m_bones[boneIdx].m_modelSpaceTransform; }

        bool IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const;

        void Finalize( TVector<StringID> const& highLODBones );

    protected:

        void CalculateLocalTransforms();
        void CalculateModelSpaceTransforms();

    protected:

        StringID                m_name;
        TVector<Bone>           m_bones;
        uint32_t                m_numBonesToSampleAtLowLOD = 0;
    };
}