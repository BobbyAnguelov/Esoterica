#include "ProceduralBoneConstraint_Twist.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void TwistConstraint::Settings::CreateConstraint( SkeletalMesh const* pMesh, ProceduralBoneConstraint* pMemory ) const
    {
        EE_ASSERT( pMesh != nullptr && pMemory != nullptr );
        TwistConstraint* pTwistConstraint = new ( pMemory ) TwistConstraint();

        pTwistConstraint->m_sourceBoneIdx = pMesh->GetBoneIndex( m_sourceBone );
        pTwistConstraint->m_targetBoneIdx = pMesh->GetBoneIndex( m_targetBone );
        EE_ASSERT( pTwistConstraint->m_sourceBoneIdx != InvalidIndex && pTwistConstraint->m_targetBoneIdx != InvalidIndex );

        pTwistConstraint->m_sourceAxis = m_sourceAxis;
        pTwistConstraint->m_targetAxis = m_targetAxis;
        pTwistConstraint->m_weight = m_weight;
    }

    void TwistConstraint::Solve( SkeletalMesh const* pMesh, TVector<Transform>& parentSpaceTransforms )
    {
        Quaternion sourceRefRotation = pMesh->GetParentSpaceBindPoseTransform( m_sourceBoneIdx ).GetRotation();
        Quaternion sourceRotation = parentSpaceTransforms[m_sourceBoneIdx].GetRotation();
        Quaternion deltaSourceQuat = Quaternion::Delta( sourceRefRotation, sourceRotation );
        Radians sourceTwistAngle = deltaSourceQuat.GetTwistAngle( m_sourceAxis );

        Radians targetTwistAngle = sourceTwistAngle * m_weight;
        Quaternion targetRefQuat = pMesh->GetParentSpaceBindPoseTransform( m_targetBoneIdx ).GetRotation();
        Quaternion deltaTargetQuat = Quaternion( m_targetAxis, targetTwistAngle );
        Quaternion finalRotation = deltaTargetQuat * targetRefQuat;
        finalRotation.Normalize();

        parentSpaceTransforms[m_targetBoneIdx].SetRotation( finalRotation );
    }
}