#include "Animation_Task_Blend.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "System/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    BlendTask::BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, PoseBlendMode blendMode, BoneMaskTaskList const* pBoneMaskTaskList )
        : Task( sourceID, TaskUpdateStage::Any, { sourceTaskIdx, targetTaskIdx } )
        , m_blendWeight( blendWeight )
        , m_blendMode( blendMode )
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
            Blender::Blend( m_blendMode, &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, result.m_pBoneMask, &pFinalBuffer->m_pose );
            
            if ( result.m_maskPoolIdx != InvalidIndex )
            {
                context.m_boneMaskPool.ReleaseMask( result.m_maskPoolIdx );
            }
        }
        else // Perform a simple blend
        {
            Blender::Blend( m_blendMode , &pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, nullptr, &pFinalBuffer->m_pose );
        }

        ReleaseDependencyPoseBuffer( context, 1 );
        MarkTaskComplete( context );
    }

    void BlendTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteDependencyIndex( m_dependencies[1] );
        serializer.WriteUInt( (uint32_t) m_blendMode, 2 );
        serializer.WriteNormalizedFloat( m_blendWeight );

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
        m_blendMode = ( PoseBlendMode ) serializer.ReadUInt( 2 );
        m_blendWeight = serializer.ReadNormalizedFloat();

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
            return String( String::CtorSprintf(), "Blend Task (Masked): %.2f%%", m_blendWeight * 100 );
        }
        else
        {
            return String( String::CtorSprintf(), "Blend Task: %.2f%%", m_blendWeight * 100 );
        }
    }
    #endif
}