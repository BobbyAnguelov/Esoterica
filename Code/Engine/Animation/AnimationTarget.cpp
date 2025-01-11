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
            if ( boneIdx == InvalidIndex )
            {
                return false;
            }

            if ( m_hasOffsets )
            {
                // Get the local transform and the parent global transform
                if ( m_isUsingBoneSpaceOffsets )
                {
                    outTransform = pPose->GetTransform( boneIdx );
                    outTransform.SetRotation( m_transform.GetRotation() * outTransform.GetRotation() );
                    outTransform.SetTranslation( outTransform.GetTranslation() + m_transform.GetTranslation() );

                    int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( boneIdx );
                    if ( parentBoneIdx != InvalidIndex )
                    {
                        outTransform = outTransform * pPose->GetModelSpaceTransform( parentBoneIdx );
                    }
                }
                else // Get the model space transform for the target bone
                {
                    outTransform = pPose->GetModelSpaceTransform( boneIdx );
                    outTransform.SetRotation( m_transform.GetRotation() * outTransform.GetRotation() );
                    outTransform.SetTranslation( outTransform.GetTranslation() + m_transform.GetTranslation() );
                }
            }
            else
            {
                outTransform = pPose->GetModelSpaceTransform( boneIdx );
            }
        }
        else // Just use the internal transform
        {
            outTransform = m_transform;
        }

        return true;
    }
}