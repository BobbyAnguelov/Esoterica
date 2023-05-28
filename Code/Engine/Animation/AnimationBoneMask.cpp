#include "AnimationBoneMask.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    BoneMask::BoneMask( Skeleton const* pSkeleton )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );
        m_weights.resize( pSkeleton->GetNumBones(), 0.0f );
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, float fixedWeight )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );

        EE_ASSERT( fixedWeight >= 0.0f && fixedWeight <= 1.0f );
        m_weights.resize( pSkeleton->GetNumBones(), fixedWeight );
    }

    BoneMask::BoneMask( BoneMask const& rhs )
    {
        EE_ASSERT( rhs.IsValid() );
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights = rhs.m_weights;
    }

    BoneMask::BoneMask( BoneMask&& rhs )
    {
        EE_ASSERT( rhs.IsValid() );
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights.swap( rhs.m_weights );
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, BoneMaskDefinition const& definition )
        : m_ID( definition.m_ID )
        , m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );
        m_weights.resize( pSkeleton->GetNumBones() );
        ResetWeights( definition );
    }

    //-------------------------------------------------------------------------

    bool BoneMask::IsValid() const
    {
        return m_pSkeleton != nullptr && !m_weights.empty() && m_weights.size() == m_pSkeleton->GetNumBones();
    }

    BoneMask& BoneMask::operator=( BoneMask const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights = rhs.m_weights;
        return *this;
    }

    BoneMask& BoneMask::operator=( BoneMask&& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights.swap( rhs.m_weights );
        return *this;
    }

    //-------------------------------------------------------------------------

    void BoneMask::ResetWeights( float fixedWeight )
    {
        EE_ASSERT( fixedWeight >= 0.0f && fixedWeight <= 1.0f );
        for ( auto& weight : m_weights )
        {
            weight = fixedWeight;
        }
    }

    void BoneMask::ResetWeights( TVector<float> const& weights )
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        EE_ASSERT( weights.size() == m_pSkeleton->GetNumBones() );
        m_weights = weights;
    }

    void BoneMask::ResetWeights( BoneMaskDefinition const& definition, bool shouldFeatherIntermediateBones )
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        if ( shouldFeatherIntermediateBones )
        {
            // Set all weights to -1 so we know what needs to be feathered!
            for ( auto& weight : m_weights )
            {
                weight = -1.0f;
            }

            // Root cannot be set to -1, since we cannot guarantee it will be part of the supplied weights
            m_weights[0] = 0.0f;
        }
        else
        {
            ResetWeights();
        }

        // Relatively expensive remap
        for ( auto const& boneWeight : definition.m_weights )
        {
            EE_ASSERT( boneWeight.m_weight >= 0.0f && boneWeight.m_weight <= 1.0f );
            int32_t const boneIdx = m_pSkeleton->GetBoneIndex( boneWeight.m_boneID );
            EE_ASSERT( boneIdx != InvalidIndex );
            m_weights[boneIdx] = boneWeight.m_weight;
        }

        // Feather intermediate weights
        if ( shouldFeatherIntermediateBones )
        {
            TInlineVector<int32_t, 50> boneChainIndices;

            for ( int32_t boneIdx = m_pSkeleton->GetNumBones() - 1; boneIdx > 0; boneIdx-- )
            {
                // Check for zero chains
                if ( m_weights[boneIdx] == -1 )
                {
                    boneChainIndices.clear();
                    boneChainIndices.emplace_back( boneIdx );

                    int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
                    while ( parentBoneIdx != InvalidIndex )
                    {
                        // Exit when we encounter the first parent with a weight
                        if ( m_weights[parentBoneIdx] != -1 )
                        {
                            break;
                        }

                        boneChainIndices.emplace_back( parentBoneIdx );
                        parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
                    }

                    // Set all weights in the chain to 0.0f
                    for ( auto i : boneChainIndices )
                    {
                        m_weights[i] = 0.0f;
                    }
                }
                // Check for feather chains
                else if ( m_weights[m_pSkeleton->GetParentBoneIndex( boneIdx )] == -1 )
                {
                    float endWeight = m_weights[boneIdx];
                    EE_ASSERT( endWeight != -1.0f );
                    float startWeight = 0.0f;

                    boneChainIndices.clear();
                    boneChainIndices.emplace_back( boneIdx );

                    int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
                    while ( parentBoneIdx != InvalidIndex )
                    {
                        boneChainIndices.emplace_back( parentBoneIdx );

                        // Exit when we encounter the first parent with a weight
                        if ( m_weights[parentBoneIdx] != -1 )
                        {
                            startWeight = m_weights[parentBoneIdx];
                            break;
                        }

                        parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
                    }

                    // Interpolate all weights in the chain
                    int32_t const numBonesInChain = (int32_t) boneChainIndices.size();
                    for ( int32_t i = numBonesInChain - 2; i > 0; i-- )
                    {
                        float const percentageThrough = float( i ) / ( numBonesInChain - 1 );
                        m_weights[boneChainIndices[i]] = Math::Lerp( endWeight, startWeight, percentageThrough );
                    }
                }
            }
        }
    }

    BoneMask& BoneMask::operator*=( BoneMask const& rhs )
    {
        int32_t const numWeights = (int32_t) m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] *= rhs.m_weights[i];
        }
        return *this;
    }

    void BoneMask::BlendFrom( BoneMask const& source, float blendWeight )
    {
        EE_ASSERT( source.m_pSkeleton == m_pSkeleton && m_weights.size() == source.m_weights.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        auto const numWeights = m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] = Math::Lerp( source.m_weights[i], m_weights[i], blendWeight );
        }
    }

    void BoneMask::BlendTo( BoneMask const& target, float blendWeight )
    {
        EE_ASSERT( target.m_pSkeleton == m_pSkeleton && m_weights.size() == target.m_weights.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        auto const numWeights = m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] = Math::Lerp( m_weights[i], target[i], blendWeight );
        }
    }

    //-------------------------------------------------------------------------
    // Bone Mask Pool
    //-------------------------------------------------------------------------

    BoneMaskPool::BoneMaskPool( Skeleton const* pSkeleton )
        : m_pSkeleton( pSkeleton )
        , m_firstFreePoolIdx( InvalidIndex )
    {
        EE_ASSERT( m_pool.empty() && m_pSkeleton != nullptr );

        m_pool.reserve( s_initialPoolSize * 2 );

        for ( auto i = 0; i < s_initialPoolSize; i++ )
        {
            m_pool.emplace_back( pSkeleton );
        }

        m_firstFreePoolIdx = 0;
    }

    BoneMaskPool::~BoneMaskPool()
    {
        m_pool.clear();
        m_firstFreePoolIdx = InvalidIndex;
    }

    #if EE_DEVELOPMENT_TOOLS
    void BoneMaskPool::PerformValidation() const
    {
        // Validate that all buffers have been released!
        for ( auto const& slot : m_pool )
        {
            EE_ASSERT( !slot.m_isUsed );
        }

        EE_ASSERT( m_firstFreePoolIdx == 0 );
    }
    #endif

    int8_t BoneMaskPool::AcquireMask( bool resetMask )
    {
        int32_t const currentPoolSize = (int32_t) m_pool.size();
        EE_ASSERT( m_firstFreePoolIdx < currentPoolSize );
        
        // Set the mask as used
        int8_t maskIdx = m_firstFreePoolIdx;
        EE_ASSERT( !m_pool[maskIdx].m_isUsed );
        m_pool[maskIdx].m_isUsed = true;

        if ( resetMask )
        {
            m_pool[maskIdx].m_mask.ResetWeights();
        }

        // Update free idx
        int8_t const searchStartIdx = m_firstFreePoolIdx + 1;
        m_firstFreePoolIdx = InvalidIndex;
        for ( int8_t i = searchStartIdx; i < currentPoolSize; i++ )
        {
            if ( !m_pool[i].m_isUsed )
            {
                m_firstFreePoolIdx = i;
                break;
            }
        }

        // Grow the pool if needed
        if ( m_firstFreePoolIdx == InvalidIndex )
        {
            size_t const newPoolSize = Math::Max( 127, currentPoolSize * 2 );
            size_t const numMasksToAdd = newPoolSize - currentPoolSize;

            for ( auto i = 0; i < numMasksToAdd; i++ )
            {
                m_pool.emplace_back( m_pSkeleton );
            }

            m_firstFreePoolIdx = (int8_t) currentPoolSize + 1;
            EE_ASSERT( m_firstFreePoolIdx < 127 );
        }

        // Return the allocate mask pool index
        EE_ASSERT( maskIdx >= 0 && maskIdx < m_pool.size() );
        return maskIdx;
    }

    void BoneMaskPool::ReleaseMask( int8_t maskIdx )
    {
        EE_ASSERT( maskIdx < m_pool.size() );
        EE_ASSERT( m_pool[maskIdx].m_isUsed );

        // Clear the flag
        m_pool[maskIdx].m_isUsed = false;

        // Update the free index
        if ( maskIdx < m_firstFreePoolIdx )
        {
            m_firstFreePoolIdx = maskIdx;
        }
    }

    //-------------------------------------------------------------------------
    // Task List
    //-------------------------------------------------------------------------

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

    int8_t BoneMaskTaskList::CreateBlend( BoneMaskTaskList const& sourceTaskList, BoneMaskTaskList const& targetTaskList, float blendWeight )
    {
        EE_ASSERT( &sourceTaskList != this && &targetTaskList != this );

        if ( blendWeight == 0 )
        {
            m_tasks = sourceTaskList.m_tasks;
            return GetLastTaskIdx();
        }
        else if ( blendWeight == 1 )
        {
            m_tasks = targetTaskList.m_tasks;
            return GetLastTaskIdx();
        }
        else // Blend
        {
            m_tasks = sourceTaskList.m_tasks;
            return BlendTo( targetTaskList, blendWeight );
        }
    }

    int8_t BoneMaskTaskList::CreateBlendToGeneratedMask( BoneMaskTaskList const& sourceTaskList, float maskWeight, float blendWeight )
    {
        EE_ASSERT( &sourceTaskList != this );

        if ( blendWeight == 0 )
        {
            m_tasks = sourceTaskList.m_tasks;
            return GetLastTaskIdx();
        }
        else if ( blendWeight == 1 )
        {
            m_tasks.clear();
            m_tasks.emplace_back( maskWeight );

            return GetLastTaskIdx();
        }
        else // Blend
        {
            m_tasks = sourceTaskList.m_tasks;
            int8_t const sourceTaskIdx = GetLastTaskIdx();

            m_tasks.emplace_back( maskWeight );
            int8_t const targetTaskIdx = GetLastTaskIdx();

            //-------------------------------------------------------------------------

            m_tasks.emplace_back( sourceTaskIdx, targetTaskIdx, blendWeight );
            EE_ASSERT( m_tasks.size() < s_maxTasks );

            return GetLastTaskIdx();
        }
    }

    int8_t BoneMaskTaskList::CreateBlendFromGeneratedMask( BoneMaskTaskList const& targetTaskList, float maskWeight, float blendWeight )
    {
        EE_ASSERT( &targetTaskList != this );

        if ( blendWeight == 0 )
        {
            m_tasks.clear();
            m_tasks.emplace_back( maskWeight );

            return GetLastTaskIdx();
        }
        else if ( blendWeight == 1 )
        {
            m_tasks = targetTaskList.m_tasks;
            return GetLastTaskIdx();
        }
        else // Blend
        {
            m_tasks.emplace_back( maskWeight );
            EE_ASSERT( m_tasks.size() < s_maxTasks );

            return BlendTo( targetTaskList, blendWeight );
        }
    }

    BoneMaskTaskList::Result BoneMaskTaskList::GenerateBoneMask( BoneMaskPool& pool ) const
    {
        Skeleton const* pSkeleton = pool.GetSkeleton();

        // If we only have a single task, just get the mask from it
        int32_t const numTasks = (int32_t) m_tasks.size();
        EE_ASSERT( numTasks < s_maxTasks );
        if ( numTasks == 1 )
        {
            switch ( m_tasks[0].m_type )
            {
                case BoneMaskTask::Type::GenerateMask:
                {
                    auto pMaskIdx = pool.AcquireMask();
                    pool[pMaskIdx]->ResetWeights( m_tasks[0].m_weight );
                    return { pool[pMaskIdx], pMaskIdx };
                }
                break;

                case BoneMaskTask::Type::Mask:
                {
                    auto pMask = pSkeleton->GetBoneMask( m_tasks[0].m_maskIdx );
                    return { pMask, InvalidIndex };
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return { nullptr, InvalidIndex };
                }
                break;
            }
        }
        else // Execute task list
        {
            TInlineVector<int8_t, 128> maskIndices; // Temp array to avoid writing into the constant tasks

            //-------------------------------------------------------------------------

            for ( auto i = 0; i < numTasks; i++ )
            {
                EE_ASSERT( m_tasks[i].IsValid() );

                // Skip reference tasks
                //-------------------------------------------------------------------------

                if ( m_tasks[i].IsMask() )
                {
                    maskIndices.emplace_back( (int8_t) InvalidIndex );
                    continue;
                }

                // Generate tasks
                //-------------------------------------------------------------------------

                if ( m_tasks[i].m_type == BoneMaskTask::Type::GenerateMask )
                {
                    maskIndices.emplace_back( pool.AcquireMask() );
                    pool[maskIndices[i]]->ResetWeights( m_tasks[i].m_weight );
                }

                // Combine/Blend tasks
                //-------------------------------------------------------------------------

                else
                {
                    BoneMaskTask const& sourceTask = m_tasks[m_tasks[i].m_sourceTaskIdx];
                    int8_t const sourceMaskIdx = maskIndices[m_tasks[i].m_sourceTaskIdx];
                    bool const isSourceAPtr = sourceTask.IsMask();
                    BoneMask const* pSourceMask = isSourceAPtr ? pSkeleton->GetBoneMask( sourceTask.m_maskIdx ) : pool[sourceMaskIdx];
                    EE_ASSERT( pSourceMask != nullptr );

                    BoneMaskTask const& targetTask = m_tasks[m_tasks[i].m_targetTaskIdx];
                    int8_t const targetMaskIdx = maskIndices[m_tasks[i].m_targetTaskIdx];
                    bool const isTargetAPtr = targetTask.IsMask();
                    BoneMask const* pTargetMask = isTargetAPtr ? pSkeleton->GetBoneMask( targetTask.m_maskIdx ) : pool[targetMaskIdx];
                    EE_ASSERT( pTargetMask != nullptr );

                    // If both are masks, acquire a result buffer
                    if ( isSourceAPtr && isTargetAPtr )
                    {
                        maskIndices.emplace_back( pool.AcquireMask() );
                        BoneMask* pResultMask = pool[maskIndices[i]];
                        *pResultMask = *pSourceMask;

                        if ( m_tasks[i].m_type == BoneMaskTask::Type::Combine )
                        {
                            pResultMask->CombineWith( *pTargetMask );
                        }
                        else
                        {
                            pResultMask->BlendTo( *pTargetMask, m_tasks[i].m_weight );
                        }
                    }
                    // If only one is a operation, reuse the result mask
                    else if ( isSourceAPtr || isTargetAPtr )
                    {
                        // Use the result of the target operation
                        if ( isSourceAPtr )
                        {
                            EE_ASSERT( targetMaskIdx != InvalidIndex );
                            maskIndices.emplace_back( targetMaskIdx );

                            if ( m_tasks[i].m_type == BoneMaskTask::Type::Combine )
                            {
                                pool[targetMaskIdx]->CombineWith( *pSourceMask );
                            }
                            else
                            {
                                pool[targetMaskIdx]->BlendFrom( *pSourceMask, m_tasks[i].m_weight );
                            }
                            
                        }
                        else // Use the result of the source operation
                        {
                            EE_ASSERT( sourceMaskIdx != InvalidIndex );
                            maskIndices.emplace_back( sourceMaskIdx );

                            if ( m_tasks[i].m_type == BoneMaskTask::Type::Combine )
                            {
                                pool[sourceMaskIdx]->CombineWith( *pTargetMask );
                            }
                            else
                            {
                                pool[sourceMaskIdx]->BlendTo( *pTargetMask, m_tasks[i].m_weight );
                            }
                        }
                    }
                    else // Neither are operations, request a new mask from the pool
                    {
                        maskIndices.emplace_back( pool.AcquireMask() );
                        BoneMask* pResultMask = pool[maskIndices[i]];
                        *pResultMask = *pSourceMask;

                        if ( m_tasks[i].m_type == BoneMaskTask::Type::Combine )
                        {
                            pResultMask->CombineWith( *pTargetMask );
                        }
                        else
                        {
                            pResultMask->BlendTo( *pTargetMask, m_tasks[i].m_weight );
                        }
                    }
                }
            }

            // Return the pose and optional pool mask index
            // The user is expected to release the return pool mask idx
            int8_t const lastTaskIdx = (int8_t) numTasks - 1;
            return { pool[maskIndices[lastTaskIdx]], maskIndices[lastTaskIdx] };
        }
    }

    void BoneMaskTaskList::Serialize( Serialization::BitArchive<1280>& archive, uint32_t maxBitsForMaskIndex ) const
    {
        uint8_t const numTasks = (uint8_t) m_tasks.size();
        uint32_t const numBitsToUseForTaskIndices = Math::GetMostSignificantBit( numTasks ) + 1;

        archive.WriteUInt( numTasks, 5 );

        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            auto const& task = m_tasks[i];

            archive.WriteUInt( (uint8_t) task.m_type, 2 );

            switch ( task.m_type )
            {
                case BoneMaskTask::Type::Mask:
                {
                    archive.WriteUInt( task.m_maskIdx, maxBitsForMaskIndex );
                }
                break;

                case BoneMaskTask::Type::GenerateMask:
                {
                    archive.WriteNormalizedFloat( task.m_weight );
                }
                break;

                case BoneMaskTask::Type::Blend:
                {
                    archive.WriteUInt( task.m_sourceTaskIdx, numBitsToUseForTaskIndices );
                    archive.WriteUInt( task.m_targetTaskIdx, numBitsToUseForTaskIndices );
                    archive.WriteNormalizedFloat( task.m_weight );
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

    void BoneMaskTaskList::Deserialize( Serialization::BitArchive<1280>& archive, uint32_t maxBitsForMaskIndex )
    {
        uint8_t const numTasks = (uint8_t) archive.ReadUInt( 5 );
        uint32_t const numBitsToUseForTaskIndices = Math::GetMostSignificantBit( numTasks ) + 1;

        for ( uint8_t i = 0; i < numTasks; i++ )
        {
            auto type = (BoneMaskTask::Type) archive.ReadUInt( 2 );

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
                    float const weight = archive.ReadNormalizedFloat();
                    m_tasks.emplace_back( weight );
                }
                break;

                case BoneMaskTask::Type::Blend:
                {
                    auto& task = m_tasks.emplace_back();
                    task.m_type = type;
                    task.m_sourceTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_targetTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_weight = archive.ReadNormalizedFloat();
                }
                break;

                case BoneMaskTask::Type::Combine:
                {
                    auto& task = m_tasks.emplace_back();
                    task.m_type = type;
                    task.m_sourceTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                    task.m_targetTaskIdx = (int8_t) archive.ReadUInt( numBitsToUseForTaskIndices );
                }
                break;
            }
        }
    }
}