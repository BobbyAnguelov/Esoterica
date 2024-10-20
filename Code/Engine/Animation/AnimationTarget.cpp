#include "AnimationTarget.h"
#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool Target::TryGetTransform( Pose const* pPose, Transform& outTransform ) const
    {
        EE_ASSERT( m_isSet );

        //-------------------------------------------------------------------------

        bool const isBoneTarget = IsBoneTarget();
        if ( isBoneTarget )
        {
            EE_ASSERT( pPose != nullptr );
            auto pSkeleton = pPose->GetSkeleton();

            int32_t const boneIdx = pSkeleton->GetBoneIndex( m_boneID );
            if ( boneIdx != InvalidIndex )
            {
                if ( m_hasOffsets )
                {
                    Transform parentTransform = Transform::Identity;

                    // Get the local transform and the parent global transform
                    if ( m_isUsingBoneSpaceOffsets )
                    {
                        int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( boneIdx );
                        if ( parentBoneIdx != InvalidIndex )
                        {
                            parentTransform = pPose->GetModelSpaceTransform( parentBoneIdx );
                        }

                        outTransform = pPose->GetTransform( boneIdx );
                    }
                    else // Get the global transform for the target bone
                    {
                        outTransform = pPose->GetModelSpaceTransform( boneIdx );
                    }

                    //-------------------------------------------------------------------------

                    outTransform.SetRotation( outTransform.GetRotation() * m_transform.GetRotation() );
                    outTransform.SetTranslation( outTransform.GetTranslation() + m_transform.GetTranslation() );

                    if ( m_isUsingBoneSpaceOffsets && isBoneTarget )
                    {
                        outTransform *= parentTransform;
                    }
                }
                else
                {
                    outTransform = pPose->GetModelSpaceTransform( boneIdx );
                }
            }
            else
            {
                return false;
            }
        }
        else // Just use the internal transform
        {
            outTransform = m_transform;
        }

        return true;
    }
}