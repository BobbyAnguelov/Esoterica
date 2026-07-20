#pragma once
#include "Engine/Animation/AnimationBoneMask.h"
#include "Base/Serialization/BitSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BoneMaskPool;

    // A bone mask operation
    //-------------------------------------------------------------------------

    struct BoneMaskTask
    {
        enum class Type : uint8_t
        {
            Mask = 0,
            GenerateMask,
            Blend,
            Scale,
            Combine
        };

    public:

        BoneMaskTask() = default;

        // Create a mask generate task!
        explicit BoneMaskTask( float weight )
            : m_type( Type::GenerateMask )
        {
            m_weight = weight;
            EE_ASSERT( IsValid() );
        }

        // Create a bone mask ptr task
        explicit BoneMaskTask( uint8_t maskSetIdx )
            : m_type( Type::Mask )
        {
            EE_ASSERT( maskSetIdx != 0xFF );
            m_maskSetIdx = maskSetIdx;
            EE_ASSERT( IsValid() );
        }

        // Create a bone mask ptr task
        explicit BoneMaskTask( int8_t sourceIdx, float scale )
            : m_type( Type::Scale )
        {
            m_sourceTaskIdx = sourceIdx;
            m_targetTaskIdx = InvalidIndex;
            m_weight = scale;
            EE_ASSERT( IsValid() );
        }

        // Create a combine task
        explicit BoneMaskTask( int8_t sourceIdx, int8_t targetIdx )
            : m_type( Type::Combine )
        {
            m_sourceTaskIdx = sourceIdx;
            m_targetTaskIdx = targetIdx;
            m_weight = -1.0f;
            EE_ASSERT( IsValid() );
        }

        // Create a blend task
        explicit BoneMaskTask( int8_t sourceIdx, int8_t targetIdx, float blendWeight )
            : m_type( Type::Blend )
        {
            m_sourceTaskIdx = sourceIdx;
            m_targetTaskIdx = targetIdx;
            m_weight = blendWeight;
            EE_ASSERT( IsValid() );
        }

        EE_FORCE_INLINE bool IsValid() const
        {
            if ( m_type == Type::Combine )
            {
                return m_sourceTaskIdx != InvalidIndex && m_targetTaskIdx != InvalidIndex && m_weight == -1.0f;
            }
            else if ( m_type == Type::Scale )
            {
                return m_sourceTaskIdx != InvalidIndex && m_targetTaskIdx == InvalidIndex && m_weight >= 0.0f && m_weight <= 1.0f;
            }
            else if ( m_type == Type::GenerateMask )
            {
                return m_sourceTaskIdx == InvalidIndex && m_targetTaskIdx == InvalidIndex;
            }
            else if ( m_type == Type::Blend )
            {
                return m_sourceTaskIdx != InvalidIndex && m_targetTaskIdx != InvalidIndex && m_weight >= 0.0f && m_weight <= 1.0f;
            }
            else // Mask Idx
            {
                return m_maskSetIdx != 0xFF;
            }
        }

        EE_FORCE_INLINE bool IsMaskSet() const { return m_type == Type::Mask; }
        EE_FORCE_INLINE bool IsOperation() const { return m_type != Type::Mask; }

    public:

        float                   m_weight = 0;
        int8_t                  m_sourceTaskIdx = InvalidIndex;
        int8_t                  m_targetTaskIdx = InvalidIndex;
        uint8_t                 m_maskSetIdx = 0xFF;
        Type                    m_type = Type::Mask;
    };

    // A command list of bone mask operations
    //-------------------------------------------------------------------------

    struct BoneMaskTaskList
    {
        constexpr static int32_t s_maxTasks = 32;

        // A default mask list with a single zero mask task (generate task with weights set to 1.0f)
        static BoneMaskTaskList const s_defaultMaskList;

    public:

        BoneMaskTaskList() = default;

        // Create with an initial task
        BoneMaskTaskList( BoneMaskTask const& task ) { m_tasks.emplace_back( task ); }

        // Create from a blend between task lists
        BoneMaskTaskList( BoneMaskTaskList const& sourceTaskList, BoneMaskTaskList const& targetTaskList, float blendWeight );

        // Do we have any tasks
        inline bool HasTasks() const { return !m_tasks.empty(); }

        // Clear all registered tasks
        inline void Reset() { m_tasks.clear(); }

        // Get the index of the last registered task
        inline int8_t GetLastTaskIdx() const { return (int8_t) m_tasks.size() - 1; }

        // Copy the tasks from another list
        inline int8_t CopyFrom( BoneMaskTaskList const& taskList )
        {
            m_tasks = taskList.m_tasks;
            EE_ASSERT( m_tasks.size() < s_maxTasks );
            return GetLastTaskIdx();
        }

        // Emplace a task onto the task list
        template<class... Args>
        BoneMaskTask& EmplaceTask( Args&&... args )
        {
            auto& task = m_tasks.emplace_back( eastl::forward<Args>( args )... );
            EE_ASSERT( m_tasks.size() < s_maxTasks );
            return task;
        }

        // Blend this task list to the result of a supplied task list
        inline int8_t BlendTo( BoneMaskTaskList const& taskList, float blendWeight )
        {
            int8_t const sourceTaskIdx = GetLastTaskIdx();
            int8_t const targetTaskIdx = AppendTaskListAndFixDependencies( taskList );
            m_tasks.emplace_back( sourceTaskIdx, targetTaskIdx, blendWeight );
            return GetLastTaskIdx();
        }

        // Combine the result of this task list with the result of a supplied task list (i.e. this * supplied)
        inline int8_t CombineWith( BoneMaskTaskList const& taskList )
        {
            int8_t const sourceTaskIdx = GetLastTaskIdx();
            int8_t const targetTaskIdx = AppendTaskListAndFixDependencies( taskList );
            m_tasks.emplace_back( sourceTaskIdx, targetTaskIdx );
            return GetLastTaskIdx();
        }

        // Set this task list to a blend between two task lists - returns the last task idx
        int8_t SetToBlendBetweenTaskLists( BoneMaskTaskList const& sourceTaskList, BoneMaskTaskList const& targetTaskList, float blendWeight );

        // Create a blend from the current registered tasks to a generated mask - returns the last task idx
        int8_t BlendToGeneratedMask( float generatedMaskWeight, float blendWeight );

        // Create a blend from a generated mask to our current registered tasks - returns the last task idx
        int8_t BlendFromGeneratedMask( float generatedMaskWeight, float blendWeight ) { return BlendToGeneratedMask( generatedMaskWeight, 1.0f - blendWeight ); }

        //-------------------------------------------------------------------------

        // Execute the task list to generate a bone mask, returns the buffer that stores the result
        int8_t GenerateBoneMask( Skeleton const* pSkeleton, BoneMaskPool& pool ) const;

        //-------------------------------------------------------------------------

        void Serialize( Serialization::BitArchive& archive, uint32_t maxBitsForMaskIndex ) const;
        void Deserialize( Serialization::BitArchive& archive, uint32_t maxBitsForMaskIndex );

    private:

        // Returns the index of the last task added
        int8_t AppendTaskListAndFixDependencies( BoneMaskTaskList const& taskList );

    private:

        TInlineVector<BoneMaskTask, 10> m_tasks;
    };
}