#include "Animation_Task_Blend.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    BlendTask::BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : Task( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
        , m_blendWeight( blendWeight )
    {
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }

        if ( pBoneMaskTaskList != nullptr )
        {
            m_boneMaskTaskList.CopyFrom( *pBoneMaskTaskList );
        }
    }

    void BlendTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        // If we have a bone mask task list, execute it
        if ( m_boneMaskTaskList.HasTasks() )
        {
            auto const result = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );
            Blender::LocalBlend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, result.m_pBoneMask, &pFinalBuffer->m_pose, true );

            #if EE_DEVELOPMENT_TOOLS
            if ( context.m_posePool.IsRecordingEnabled() )
            {
                m_debugBoneMask = *result.m_pBoneMask;
            }
            #endif

            if ( result.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( result.m_maskPoolIdx );
            }
        }
        else // Perform a simple blend
        {
            Blender::LocalBlend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, nullptr, &pFinalBuffer->m_pose, true );
        }

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    void BlendTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteDependencyIndex( m_dependencies[1] );
        serializer.WriteNormalizedFloat8Bit( m_blendWeight );

        // Bone Mask Task List
        bool const hasBoneMaskTasks = m_boneMaskTaskList.HasTasks();
        serializer.WriteBool( hasBoneMaskTasks );
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Serialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    void BlendTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 2 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_dependencies[1] = serializer.ReadDependencyIndex();
        m_blendWeight = serializer.ReadNormalizedFloat8Bit();

        // Bone Mask Task List
        bool const hasBoneMaskTasks = serializer.ReadBool();
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Deserialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    String BlendTask::GetDebugText() const
    {
        if ( m_boneMaskTaskList.HasTasks() )
        {
            return String( String::CtorSprintf(), "Blend (Masked): %.2f%%", m_blendWeight * 100 );
        }
        else
        {
            return String( String::CtorSprintf(), "Blend: %.2f%%", m_blendWeight * 100 );
        }
    }

    void BlendTask::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const
    {
        TBitFlags<Pose::DrawFlags> drawFlags;
        if ( isDetailedViewEnabled )
        {
            if ( m_debugBoneMask.IsValid() )
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneWeights );
            }
            else
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneNames );
            }
        }

        pRecordedPose->DrawDebug( drawingContext, worldTransform, GetDebugColor(), 3.0f, m_debugBoneMask.IsValid() ? &m_debugBoneMask : nullptr, drawFlags );
    }
    #endif

    //-------------------------------------------------------------------------

    AdditiveBlendTask::AdditiveBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList )
        : Task( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
        , m_blendWeight( blendWeight )
    {
        EE_ASSERT( m_blendWeight >= 0.0f && m_blendWeight <= 1.0f );

        // Ensure that blend weights very close to 1.0f are set to 1.0f since the blend code will optimize away unnecessary blend operations
        if ( Math::IsNearEqual( m_blendWeight, 1.0f ) )
        {
            m_blendWeight = 1.0f;
        }

        if ( pBoneMaskTaskList != nullptr )
        {
            m_boneMaskTaskList.CopyFrom( *pBoneMaskTaskList );
        }
    }

    void AdditiveBlendTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = AccessDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pSourceBuffer;

        // If we have a bone mask task list, execute it
        if ( m_boneMaskTaskList.HasTasks() )
        {
            auto const result = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );
            Blender::AdditiveBlend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, result.m_pBoneMask, &pFinalBuffer->m_pose );

            #if EE_DEVELOPMENT_TOOLS
            if ( context.m_posePool.IsRecordingEnabled() )
            {
                m_debugBoneMask = *result.m_pBoneMask;
            }
            #endif

            if ( result.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( result.m_maskPoolIdx );
            }
        }
        else // Perform a simple blend
        {
            Blender::AdditiveBlend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, nullptr, &pFinalBuffer->m_pose );
        }

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    void AdditiveBlendTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteDependencyIndex( m_dependencies[1] );
        serializer.WriteNormalizedFloat8Bit( m_blendWeight );

        // Bone Mask Task List
        bool const hasBoneMaskTasks = m_boneMaskTaskList.HasTasks();
        serializer.WriteBool( hasBoneMaskTasks );
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Serialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    void AdditiveBlendTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 2 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_dependencies[1] = serializer.ReadDependencyIndex();
        m_blendWeight = serializer.ReadNormalizedFloat8Bit();

        // Bone Mask Task List
        bool const hasBoneMaskTasks = serializer.ReadBool();
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Deserialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    String AdditiveBlendTask::GetDebugText() const
    {
        if ( m_boneMaskTaskList.HasTasks() )
        {
            return String( String::CtorSprintf(), "Additive Blend (Masked): %.2f%%", m_blendWeight * 100 );
        }
        else
        {
            return String( String::CtorSprintf(), "Additive Blend: %.2f%%", m_blendWeight * 100 );
        }
    }

    void AdditiveBlendTask::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const
    {
        TBitFlags<Pose::DrawFlags> drawFlags;
        if ( isDetailedViewEnabled )
        {
            if ( m_debugBoneMask.IsValid() )
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneWeights );
            }
            else
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneNames );
            }
        }

        pRecordedPose->DrawDebug( drawingContext, worldTransform, GetDebugColor(), 3.0f, m_debugBoneMask.IsValid() ? &m_debugBoneMask : nullptr, drawFlags );
    }
    #endif

    //-------------------------------------------------------------------------

    GlobalBlendTask::GlobalBlendTask( TaskSourceID sourceID, TaskIndex baseTaskIdx, TaskIndex layerTaskIdx, float const layerWeight, BoneMaskTaskList const& boneMaskTaskList )
        : Task( sourceID, TaskUpdateStage::Any, { baseTaskIdx, layerTaskIdx } )
        , m_layerWeight( layerWeight )
    {
        EE_ASSERT( m_layerWeight >= 0.0f && m_layerWeight <= 1.0f );
        EE_ASSERT( boneMaskTaskList.HasTasks() );
        m_boneMaskTaskList.CopyFrom( boneMaskTaskList );
    }

    void GlobalBlendTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        auto pSourceBuffer = AccessDependencyPoseBuffer( context, 0 );
        auto pTargetBuffer = TransferDependencyPoseBuffer( context, 1 );
        auto pFinalBuffer = pTargetBuffer;

        // If we have a bone mask task list, execute it
        if ( m_boneMaskTaskList.HasTasks() )
        {
            auto const result = m_boneMaskTaskList.GenerateBoneMask( context.m_boneMaskPool );

            Blender::GlobalBlend( &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_layerWeight, result.m_pBoneMask, &pFinalBuffer->m_pose );

            #if EE_DEVELOPMENT_TOOLS
            if ( context.m_posePool.IsRecordingEnabled() )
            {
                m_debugBoneMask = *result.m_pBoneMask;
            }
            #endif

            if ( result.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( result.m_maskPoolIdx );
            }
        }

        ReleaseDependencyPoseBuffer( context, 0 );
        MarkTaskComplete( context );
    }

    void GlobalBlendTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteDependencyIndex( m_dependencies[1] );
        serializer.WriteNormalizedFloat8Bit( m_layerWeight );

        // Bone Mask Task List
        bool const hasBoneMaskTasks = m_boneMaskTaskList.HasTasks();
        serializer.WriteBool( hasBoneMaskTasks );
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Serialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    void GlobalBlendTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 2 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_dependencies[1] = serializer.ReadDependencyIndex();
        m_layerWeight = serializer.ReadNormalizedFloat8Bit();

        // Bone Mask Task List
        bool const hasBoneMaskTasks = serializer.ReadBool();
        if ( hasBoneMaskTasks )
        {
            m_boneMaskTaskList.Deserialize( serializer, serializer.GetMaxBitsForBoneMaskIndex() );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void GlobalBlendTask::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const
    {
        TBitFlags<Pose::DrawFlags> drawFlags;
        if ( isDetailedViewEnabled )
        {
            if ( m_debugBoneMask.IsValid() )
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneWeights );
            }
            else
            {
                drawFlags.SetFlag( Pose::DrawFlags::DrawBoneNames );
            }
        }

        pRecordedPose->DrawDebug( drawingContext, worldTransform, GetDebugColor(), 3.0f, m_debugBoneMask.IsValid() ? &m_debugBoneMask : nullptr, drawFlags );
    }
    #endif
}