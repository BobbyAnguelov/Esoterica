#pragma once
#include "ProceduralBoneConstraint.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API TwistConstraint : public ProceduralBoneConstraint
    {

    public:

        class EE_ENGINE_API Settings : public ProceduralBoneConstraint::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_PROCEDURAL_BONE_SETTINGS( TwistConstraint );

            virtual void CreateConstraint( SkeletalMesh const* pMesh, ProceduralBoneConstraint* pMemory ) const override;

        public:

            EE_REFLECT();
            StringID m_sourceBone;

            EE_REFLECT();
            StringID m_targetBone;

            EE_REFLECT();
            Float3 m_sourceAxis = Float3::UnitX;

            EE_REFLECT();
            Float3 m_targetAxis = Float3::UnitX;

            EE_REFLECT();
            float m_weight = 0.0f;
        };

    public:

        virtual void Solve( SkeletalMesh const* pMesh, TVector<Transform>& parentSpaceTransforms ) override;

    private:

        int32_t     m_sourceBoneIdx = InvalidIndex;
        int32_t     m_targetBoneIdx = InvalidIndex;
        Vector      m_sourceAxis = Vector::UnitX;
        Vector      m_targetAxis = Vector::UnitX;
        float       m_weight = 0.0f;
    };
}