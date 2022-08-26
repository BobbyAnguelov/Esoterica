#include "Animation_Task_Blend.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    BlendTask::BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, TBitFlags<PoseBlendOptions> blendOptions, BoneMask const* pBoneMask )
        : Task( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
        , m_blendWeight( blendWeight )
        , m_pBoneMask( pBoneMask )
        , m_blendOptions( blendOptions )
    {
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }
    }

    void BlendTask::Execute( TaskContext const& context )
    {
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        Blender::Blend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, m_blendOptions, m_pBoneMask, &pFinalBuffer->m_pose );

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    //-------------------------------------------------------------------------

    PivotBlendTask::PivotBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, StringID pivotBoneID, Vector* pPivotOffset, bool shouldCalculatePivotOffset )
        : Task( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
        , m_blendWeight( blendWeight )
        , m_pivotBoneID( pivotBoneID )
        , m_pPivotOffset( pPivotOffset )
        , m_shouldCalculatePivotOffset( shouldCalculatePivotOffset )
    {
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );
        EE_ASSERT( m_pivotBoneID.IsValid() && pPivotOffset != nullptr );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }
    }

    void PivotBlendTask::Execute( TaskContext const& context )
    {
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        // Blend pivot
        auto pSkeleton = pSourceBuffer->m_pose.GetSkeleton();
        int32_t const pivotIdx = ( true ) ? pSourceBuffer->m_pose.GetSkeleton()->GetBoneIndex( m_pivotBoneID ) : InvalidIndex;
        if ( pivotIdx != InvalidIndex )
        {
            EE_ASSERT( pSourceBuffer->m_pose.GetSkeleton() == pTargetBuffer->m_pose.GetSkeleton() ); // We dont support mixing skeletons atm

            // Set pivot update
            if ( m_shouldCalculatePivotOffset )
            {
                Transform const sourcePivotTransform = pSourceBuffer->m_pose.GetGlobalTransform( pivotIdx );
                Transform const targetPivotTransform = pTargetBuffer->m_pose.GetGlobalTransform( pivotIdx );
                *m_pPivotOffset = targetPivotTransform.GetTranslation() - sourcePivotTransform.GetTranslation();
            }
            else
            {
                // Get all direct children of the root bone
                TInlineVector<int32_t, 5> rootChildBoneIndices;
                for ( auto boneIdx = 1; boneIdx < pSkeleton->GetNumBones(); boneIdx++ )
                {
                    if ( pSkeleton->GetParentBoneIndex( boneIdx ) == 0 )
                    {
                        rootChildBoneIndices.emplace_back( boneIdx );
                    }
                }

                // Shift the target pose with the delta - we dont want to modify the root so we adjust all children of the root instead
                for ( auto boneIdx : rootChildBoneIndices )
                {
                    Transform shiftedSourceTransform = pSourceBuffer->m_pose.GetTransform( boneIdx );
                    shiftedSourceTransform.AddTranslation( *m_pPivotOffset );
                    pSourceBuffer->m_pose.SetTransform( boneIdx, shiftedSourceTransform );
                }
            }
        }

        // Perform Blend
        Blender::Blend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, m_blendOptions, m_pBoneMask, &pFinalBuffer->m_pose );

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }
}