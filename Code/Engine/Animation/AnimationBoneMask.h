#pragma once

#include "Engine/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/StringID.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Serialization/BitSerialization.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Skeleton;

    //-------------------------------------------------------------------------
    // Bone Mask Definition
    //-------------------------------------------------------------------------
    // Serialized data that describes a set of weights from which we can generate a bone mask

    struct EE_ENGINE_API BoneMaskDefinition final : public IReflectedType
    {
        friend class SkeletonEditor;

        EE_REFLECT_TYPE( BoneMaskDefinition )
        EE_SERIALIZE( m_ID, m_weights );

    public:

        struct EE_ENGINE_API BoneWeight final : public IReflectedType
        {
            EE_REFLECT_TYPE( BoneWeight )
            EE_SERIALIZE( m_boneID, m_weight );

            BoneWeight() = default;

            BoneWeight( StringID const ID, float weight )
                : m_boneID( ID )
                , m_weight( weight )
            {
                EE_ASSERT( ID.IsValid() && m_weight >= 0.0f && m_weight <= 1.0f );
            }

            EE_REFLECT( "IsToolsReadOnly" : true );
            StringID            m_boneID;

            EE_REFLECT( "IsToolsReadOnly" : true );
            float               m_weight = 0.0f;
        };

    public:

        BoneMaskDefinition() = default;
        BoneMaskDefinition( StringID ID ) : m_ID( ID ) {}

    public:

        EE_REFLECT( "IsToolsReadOnly" : true );
        StringID                m_ID;

        EE_REFLECT( "IsToolsReadOnly" : true );
        TVector<BoneWeight>     m_weights;
    };

    //-------------------------------------------------------------------------
    // Bone Mask
    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoneMask
    {
    public:

        enum class WeightInfo : uint8_t
        {
            Zero, // All weights are set to 0.0f
            Mixed, // Mixed weights
            One, // All weights are set to 1.0f
        };

        #if EE_DEVELOPMENT_TOOLS
        static Color GetColorForWeight( float weight );
        #endif

    public:

        BoneMask() : m_pSkeleton( nullptr ) {}
        BoneMask( Skeleton const* pSkeleton );
        BoneMask( Skeleton const* pSkeleton, float fixedWeight );
        BoneMask( Skeleton const* pSkeleton, BoneMaskDefinition const& definition );

        BoneMask( BoneMask const& rhs );
        BoneMask( BoneMask&& rhs );

        BoneMask& operator=( BoneMask const& rhs );
        BoneMask& operator=( BoneMask&& rhs );

        // Only copies the mask value but not the ID!
        inline void CopyFrom( BoneMask const& rhs ) { *this = rhs; }

        //-------------------------------------------------------------------------

        inline StringID GetID() const { return m_ID; }
        inline bool IsValid() const;
        inline Skeleton const* GetSkeleton() const { return m_pSkeleton; }
        inline int32_t GetNumWeights() const { return (int32_t) m_weights.size(); }
        inline float GetWeight( uint32_t i ) const { EE_ASSERT( i < (uint32_t) m_weights.size() ); return m_weights[i]; }
        inline float operator[]( uint32_t i ) const { return GetWeight( i ); }
        BoneMask& operator*=( BoneMask const& rhs );

        //-------------------------------------------------------------------------

        // Is this a zero mask? i.e. all the weights are set to 1.0f and therefore no masking will occur
        inline bool IsZeroWeightMask() const { return m_weightInfo == WeightInfo::Zero; }

        // Is this a full mask? i.e. all the weights are set to 0.0f and therefore this will completely mask out all bones
        inline bool IsFullWeightMask() const { return m_weightInfo == WeightInfo::One; }

        // Is this a mixed mask? i.e. mixed set of mask weights
        inline bool IsMixedWeightMask() const { return m_weightInfo == WeightInfo::Mixed; }

        //-------------------------------------------------------------------------

        // Set all weights to zero
        void ResetWeights() { Memory::MemsetZero( m_weights.data(), m_weights.size() ); m_weightInfo = WeightInfo::Zero; }

        // Set all weights to a fixed weight
        void ResetWeights( float fixedWeight );

        // Set all weights to the supplied weights, note that the number of supplied weights MUST match the expected num of weights!!
        void ResetWeights( TVector<float> const& weights );

        // Calculate bone weights based on a set of supplied bone IDs and weights, user can specify whether we should feather any weights between set bones
        // i.e. if head is set to 1.0f and pelvis is set to 0.2f, the intermediate spine bone weights will be gradually increased till they match the head weight
        void ResetWeights( BoneMaskDefinition const& definition, bool shouldFeatherIntermediateBones = true );

        // Apply this scaling factor to all weights
        void ScaleWeights( float scale );

        // Multiple the supplied bone mask into the current bone mask
        inline void CombineWith( BoneMask const& rhs ) { operator*=( rhs ); }

        // Blend from the supplied mask weight towards our weights with the supplied blend weight [0:1] (where 0 = fully in the source, and 1 = fully in the target)
        void BlendFrom( BoneMask const& source, float blendWeight );

        // Blend towards the supplied mask weights from our current weights with the supplied blend weight [0:1] (where 0 = fully in the source, and 1 = fully in the target)
        void BlendTo( BoneMask const& target, float blendWeight );

    private:

        EE_FORCE_INLINE void SetWeightInfo( float fixedWeight )
        {
            if ( fixedWeight == 0.0f )
            {
                m_weightInfo = WeightInfo::Zero;
            }
            else if ( fixedWeight == 1.0f )
            {
                m_weightInfo = WeightInfo::One;
            }
            else
            {
                m_weightInfo = WeightInfo::Mixed;
            }
        }

    private:

        StringID                    m_ID;
        WeightInfo                  m_weightInfo = WeightInfo::Zero;
        Skeleton const*             m_pSkeleton = nullptr;
        TVector<float>              m_weights;
    };

    // Bone Mask Task System
    //-------------------------------------------------------------------------

    class BoneMaskPool
    {
        constexpr static int32_t const s_initialPoolSize = 5;

        struct Slot
        {
            Slot( Skeleton const* pSkeleton ) : m_mask( pSkeleton ) {}

            BoneMask    m_mask;
            bool        m_isUsed = false;
        };

    public:

        BoneMaskPool( Skeleton const* pSkeleton );
        BoneMaskPool( BoneMaskPool const& ) = delete;
        ~BoneMaskPool();

        BoneMaskPool& operator=( BoneMaskPool const& rhs ) = delete;

        #if EE_DEVELOPMENT_TOOLS
        void PerformValidation() const;
        #endif

        inline Skeleton const* GetSkeleton() const { return m_pSkeleton; }

        // Get a mask from the pool - pool has a max size of 127
        // By default mask are not reset so be careful what you do with the mask
        int8_t AcquireMask( bool resetMask = false );

        // Release a mask back into the pool
        void ReleaseMask( int8_t maskIdx );

        // Get a used bone mask
        inline BoneMask* operator[]( size_t maskIdx )
        {
            EE_ASSERT( m_pool[maskIdx].m_isUsed );
            return &m_pool[maskIdx].m_mask;
        }

    private:

        Skeleton const*             m_pSkeleton = nullptr;
        TVector<Slot>               m_pool;
        int8_t                      m_firstFreePoolIdx = InvalidIndex;
    };

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
        explicit BoneMaskTask( uint8_t maskIdx )
            : m_type( Type::Mask )
        {
            EE_ASSERT( maskIdx != 0xFF );
            m_maskIdx = maskIdx;
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
                return m_maskIdx != 0xFF;
            }
        }

        EE_FORCE_INLINE bool IsMask() const { return m_type == Type::Mask; }
        EE_FORCE_INLINE bool IsOperation() const { return m_type != Type::Mask; }

    public:

        float       m_weight = 0;
        int8_t      m_sourceTaskIdx = InvalidIndex;
        int8_t      m_targetTaskIdx = InvalidIndex;
        uint8_t     m_maskIdx = 0xFF;
        Type        m_type = Type::Mask;
    };

    //-------------------------------------------------------------------------

    struct BoneMaskTaskList
    {
        struct Result
        {
            Result( BoneMask const* pBoneMask, int8_t maskPoolIdx )
                : m_pBoneMask( pBoneMask )
                , m_maskPoolIdx( maskPoolIdx )
            {}

            BoneMask const*     m_pBoneMask = nullptr;
            int8_t              m_maskPoolIdx = InvalidIndex; // If this is set, you need to manually release the result buffer
        };

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

        // Execute the task list to generate a body mask
        Result GenerateBoneMask( BoneMaskPool& pool ) const;

        //-------------------------------------------------------------------------

        void Serialize( Serialization::BitArchive<1280>& archive, uint32_t maxBitsForMaskIndex ) const;
        void Deserialize( Serialization::BitArchive<1280>& archive, uint32_t maxBitsForMaskIndex );

    private:

        // Returns the index of the last task added
        int8_t AppendTaskListAndFixDependencies( BoneMaskTaskList const& taskList );

    private:

        TInlineVector<BoneMaskTask, 10> m_tasks;
    };
}