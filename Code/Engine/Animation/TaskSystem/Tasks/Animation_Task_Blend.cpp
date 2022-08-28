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
}