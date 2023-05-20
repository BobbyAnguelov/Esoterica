#pragma once

#include "Engine/_Module/API.h"
#include "System/Types/Arrays.h"
#include "System/Types/StringID.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/TypeSystem/ReflectedType.h"
#include "System/Resource/IResource.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Skeleton;

    //-------------------------------------------------------------------------
    // Bone Weights
    //-------------------------------------------------------------------------
    // Serialized data that describes a set of weights from which we can generate a bone mask

    class EE_ENGINE_API BoneMaskDefinition final : public Resource::IResource
    {
        EE_RESOURCE( 'bmsk', "Animation Bone Mask" );
        EE_SERIALIZE( m_weights );

        friend class BoneMaskCompiler;
        friend class BoneMaskLoader;
        friend class BoneMaskWorkspace;

    public:

        struct EE_ENGINE_API BoneWeight
        {
            EE_SERIALIZE( m_boneID, m_weight );

            BoneWeight() = default;

            BoneWeight( StringID const ID, float weight )
                : m_boneID( ID )
                , m_weight( weight)
            {
                EE_ASSERT( ID.IsValid() && m_weight >= 0.0f && m_weight <= 1.0f );
            }

            StringID            m_boneID;
            float               m_weight = 0.0f;
        };

    public:

        virtual bool IsValid() const final { return true; }
        TVector<BoneWeight> const& GetWeights() const { return m_weights; }

    private:

        TVector<BoneWeight>     m_weights;
    };

    //-------------------------------------------------------------------------
    // Bone Mask
    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoneMask
    {

    public:

        static inline BoneMask SetFromBlend( BoneMask const& source, BoneMask const& target, float blendWeight )
        {
            BoneMask result( source );
            result.BlendTo( target, blendWeight );
            return result;
        }

    public:

        BoneMask() : m_pSkeleton( nullptr ) {}
        BoneMask( Skeleton const* pSkeleton );
        BoneMask( Skeleton const* pSkeleton, float fixedWeight, float rootMotionWeight );
        BoneMask( Skeleton const* pSkeleton, BoneMaskDefinition const& definition, float rootMotionWeight );

        BoneMask( BoneMask const& rhs );
        BoneMask( BoneMask&& rhs );

        BoneMask& operator=( BoneMask const& rhs );
        BoneMask& operator=( BoneMask&& rhs );

        inline void CopyFrom( BoneMask const& rhs ) { *this = rhs; }

        //-------------------------------------------------------------------------

        inline bool IsValid() const;
        inline Skeleton const* GetSkeleton() const { return m_pSkeleton; }
        inline int32_t GetNumWeights() const { return (int32_t) m_weights.size(); }
        inline float GetWeight( uint32_t i ) const { EE_ASSERT( i < (uint32_t) m_weights.size() ); return m_weights[i]; }
        inline float operator[]( uint32_t i ) const { return GetWeight( i ); }
        BoneMask& operator*=( BoneMask const& rhs );

        // Set all weights to zero
        void ResetWeights() { Memory::MemsetZero( m_weights.data(), m_weights.size() ); m_rootMotionWeight = 0.0f; }

        // Set all weights to a fixed weight
        void ResetWeights( float fixedWeight, float rootMotionWeight );

        // Set all weights to a fixed weight
        void ResetWeights( float fixedWeight ) { ResetWeights( fixedWeight, fixedWeight ); }

        // Set all weights to the supplied weights, note that the number of supplied weights MUST match the expected num of weights!!
        void ResetWeights( TVector<float> const& weights, float rootMotionWeight );

        // Calculate bone weights based on a set of supplied bone IDs and weights, user can specify whether we should feather any weights between set bones
        // i.e. if head is set to 1.0f and pelvis is set to 0.2f, the intermediate spine bone weights will be gradually increased till they match the head weight
        void ResetWeights( BoneMaskDefinition const& definition, float rootMotionWeight, bool shouldFeatherIntermediateBones = true );

        // Multiple the supplied bone mask into the current bone mask
        inline void CombineWith( BoneMask const& rhs ) { operator*=( rhs ); }

        // Blend from the supplied mask weight towards our weights with the supplied blend weight
        void BlendFrom( BoneMask const& source, float blendWeight );

        // Blend towards the supplied mask weights from our current weights with the supplied blend weight
        void BlendTo( BoneMask const& target, float blendWeight );

    private:

        Skeleton const*             m_pSkeleton = nullptr;
        TVector<float>              m_weights;
        float                       m_rootMotionWeight = 0.0f;
    };

    //-------------------------------------------------------------------------

    class BoneMaskPool
    {
        constexpr static int32_t const s_initialPoolSize = 4;

    public:

        BoneMaskPool( Skeleton const* pSkeleton );
        ~BoneMaskPool();

        inline bool IsEmpty() const { return m_pool.size() == 0; }
        BoneMask* GetBoneMask();
        void Reset() { m_firstFreePoolIdx = 0; }

    private:

        TVector<BoneMask*>          m_pool;
        int32_t                     m_firstFreePoolIdx = InvalidIndex;
    };
}