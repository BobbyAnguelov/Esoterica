#include "Animation_BoneMaskTask.h"
#include "Animation_BoneMaskPool.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    BoneMaskTaskList const BoneMaskTaskList::s_defaultMaskList( BoneMaskTask( 1.0f ) );

    //-------------------------------------------------------------------------

    BoneMaskTaskList::BoneMaskTaskList( BoneMaskTaskList const& sourceTaskList, BoneMaskTaskList const& targetTaskList, float blendWeight )
    {
        EE_ASSERT( &sourceTaskList != this && &targetTaskList != this );

        if ( blendWeight == 0 )
        {
            m_tasks = sourceTaskList.m_tasks;
        }
        else if ( blendWeight == 1 )
        {
            m_tasks = targetTaskList.m_tasks;
        }
        else // Blend
        {
            m_tasks = sourceTaskList.m_tasks;
            BlendTo( targetTaskList, blendWeight );
        }
    }

    int8_t BoneMaskTaskList::SetToBlendBetweenTaskLists( BoneMaskTaskList const& sourceTaskList, BoneMaskTaskList const& targetTaskList, float blendWeight )
    {
        EE_ASSERT( &sourceTaskList != this && &targetTaskList != this );

        if ( blendWeight == 0 )
        {
            m_tasks = sourceTaskList.m_tasks;
        }
        else if ( blendWeight == 1 )
        {
            m_tasks = targetTaskList.m_tasks;
        }
        else // Blend
        {
            m_tasks = sourceTaskList.m_tasks;
            BlendTo( targetTaskList, blendWeight );
        }

        return GetLastTaskIdx();
    }

    int8_t BoneMaskTaskList::AppendTaskListAndFixDependencies( BoneMaskTaskList const& taskList )
    {
        int8_t const dependencyOffset = GetLastTaskIdx() + 1;

        m_tasks.reserve( m_tasks.size() + taskList.m_tasks.size() );
        for ( BoneMaskTask const& task : taskList.m_tasks )
        {
            BoneMaskTask& addedTask = m_tasks.emplace_back( task );
            if ( addedTask.m_type == BoneMaskTask::Type::Blend || addedTask.m_type == BoneMaskTask::Type::Combine )
            {
                addedTask.m_sourceTaskIdx += dependencyOffset;
                addedTask.m_targetTaskIdx += dependencyOffset;
            }
        }

        EE_ASSERT( m_tasks.size() < 128 );
        return GetLastTaskIdx();
    }

    int8_t BoneMaskTaskList::BlendToGeneratedMask( float maskWeight, float blendWeight )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );

        // If no weight, do nothing
        if ( blendWeight == 0 )
        {
            // Do Nothing
        }
        // If full weight, replace the task with the generated mask
        else if ( blendWeight == 1 )
        {
            m_tasks.clear();
            m_tasks.emplace_back( maskWeight );
        }
        else // Blend
        {
            int8_t const sourceTaskIdx = GetLastTaskIdx();
            m_tasks.emplace_back( maskWeight );
            int8_t const targetTaskIdx = GetLastTaskIdx();

            //-------------------------------------------------------------------------

            m_tasks.emplace_back( sourceTaskIdx, targetTaskIdx, blendWeight );
            EE_ASSERT( m_tasks.size() < s_maxTasks );
        }

        return GetLastTaskIdx();
    }

    int8_t BoneMaskTaskList::GenerateBoneMask( Skeleton const* pSkeleton, BoneMaskPool& pool ) const
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        // If we only have a single task, just get the mask from it
        int32_t const numTasks = (int32_t) m_tasks.size();
        EE_ASSERT( numTasks < s_maxTasks );

        TInlineVector<int8_t, 128> maskBufferIndices; // Temp array to avoid writing into the constant tasks

        //-------------------------------------------------------------------------

        for ( auto i = 0; i < numTasks; i++ )
        {
            EE_ASSERT( m_tasks[i].IsValid() );

            // Skip reference tasks
            //-------------------------------------------------------------------------

            if ( m_tasks[i].IsMaskSet() )
            {
                maskBufferIndices.emplace_back( pool.AcquireMaskBuffer() );
                BoneMaskSet const* pMaskSet = pSkeleton->GetBoneMaskSet( m_tasks[i].m_maskSetIdx );
                EE_ASSERT( pMaskSet != nullptr );
                pool.GetBuffer( maskBufferIndices[i] )->CopyFrom( *pMaskSet );
            }

            // Generate tasks
            //-------------------------------------------------------------------------

            else if ( m_tasks[i].m_type == BoneMaskTask::Type::GenerateMask )
            {
                maskBufferIndices.emplace_back( pool.AcquireMaskBuffer() );
                pool.GetBuffer( maskBufferIndices[i] )->ResetWeights( m_tasks[i].m_weight );
            }

            // Scale
            //-------------------------------------------------------------------------

            else if ( m_tasks[i].m_type == BoneMaskTask::Type::Scale )
            {
                int8_t const sourceMaskBufferIdx = maskBufferIndices[m_tasks[i].m_sourceTaskIdx];
                maskBufferIndices.emplace_back( sourceMaskBufferIdx );
                pool.GetBuffer( sourceMaskBufferIdx )->ScaleWeights( m_tasks[i].m_weight );
            }

            // Combine/Blend tasks
            //-------------------------------------------------------------------------

            else
            {
                int8_t const sourceMaskBufferIdx = maskBufferIndices[m_tasks[i].m_sourceTaskIdx];
                BoneMaskBuffer* pSourceBuffer = pool.GetBuffer( sourceMaskBufferIdx );

                int8_t const targetMaskBufferIdx = maskBufferIndices[m_tasks[i].m_targetTaskIdx];
                BoneMaskBuffer* pTargetBuffer = pool.GetBuffer( targetMaskBufferIdx );

                maskBufferIndices.emplace_back( targetMaskBufferIdx );

                if ( m_tasks[i].m_type == BoneMaskTask::Type::Combine )
                {
                    pTargetBuffer->CombineWith( pSourceBuffer );
                }
                else
                {
                    pTargetBuffer->BlendFrom( pSourceBuffer, m_tasks[i].m_weight );
                }

                pool.ReleaseMaskBuffer( sourceMaskBufferIdx );
            }
        }

        // Return the result buffer index, the user is expected to release the return pool mask idx
        int8_t const lastTaskIdx = (int8_t) numTasks - 1;
        return maskBufferIndices[lastTaskIdx];
    }

    void BoneMaskTaskList::Serialize( Serialization::BitArchive& archive, uint32_t maxBitsForMaskIndex ) const
    {
        uint8_t const numTasks = (uint8_t) m_tasks.size();
        uint32_t const numBitsToUseForTaskIndices = Math::GetMaxNumberOfBitsForValue( numTasks );

        archive.WriteUInt( numTasks, 5 );

        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            auto const& task = m_tasks[i];

            archive.WriteUInt( (uint8_t) task.m_type, 3 );

            switch ( task.m_type )
            {
                case BoneMaskTask::Type::Mask:
                {
                    archive.WriteUInt( task.m_maskSetIdx, maxBitsForMaskIndex );
                }
                break;

                case BoneMaskTask::Type::GenerateMask:
                {
                    archive.WriteNormalizedFloat8Bit( task.m_weight );
                }
                break;

                case BoneMaskTask::Type::Blend:
                {
                    archive.WriteUInt( task.m_sourceTaskIdx, numBitsToUseForTaskIndices );
                    archive.WriteUInt( task.m_targetTaskIdx, numBitsToUseForTaskIndices );
                    archive.WriteNormalizedFloat8Bit( task.m_weight );
                }
                break;

                case BoneMaskTask::Type::Scale:
                {
                    archive.WriteUInt( task.m_sourceTaskIdx, numBitsToUseForTaskIndices );
                    archive.WriteNormalizedFloat8Bit( task.m_weight );
                }
                break;

                case BoneMaskTask::Type::Combine:
                {
                    archive.WriteUInt( task.m_sourceTaskIdx, numBitsToUseForTaskIndices );
                    archive.WriteUInt( task.m_targetTaskIdx, numBitsToUseForTaskIndices );
                }
                break;
            }
        }
    }

    void BoneMaskTaskList::Deserialize( Serialization::BitArchive& archive, uint32_t maxBitsForMaskIndex )
    {
        uint8_t const numTasks = (uint8_t) archive.ReadUInt( 5 );
        uint32_t const numBitsToUseForTaskIndices = Math::GetMaxNumberOfBitsForValue( numTasks );

        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            auto type = (BoneMaskTask::Type) archive.ReadUInt( 3 );

            switch ( type )
            {
                case BoneMaskTask::Type::Mask:
                {
                    uint8_t const maskIdx = (uint8_t) archive.ReadUInt( maxBitsForMaskIndex );
                    m_tasks.emplace_back( maskIdx );
                }
                break;

                case BoneMaskTask::Type::GenerateMask:
                {
                    float const weight = archive.ReadNormalizedFloat8Bit();
                    m_tasks.emplace_back( weight );
                }
                break;

                case BoneMaskTask::Type::Blend:
                {
                    auto& task = m_tasks.emplace_back();
                    task.m_type = type;
                    task.m_sourceTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_targetTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_weight = archive.ReadNormalizedFloat8Bit();
                }
                break;

                case BoneMaskTask::Type::Scale:
                {
                    auto& task = m_tasks.emplace_back();
                    task.m_type = type;
                    task.m_sourceTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_targetTaskIdx = InvalidIndex;
                    task.m_weight = archive.ReadNormalizedFloat8Bit();
                }
                break;

                case BoneMaskTask::Type::Combine:
                {
                    auto& task = m_tasks.emplace_back();
                    task.m_type = type;
                    task.m_sourceTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_targetTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_weight = -1.0f;
                }
                break;
            }
        }
    }
}