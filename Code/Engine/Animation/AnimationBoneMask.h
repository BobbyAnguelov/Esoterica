#pragma once

#include "Engine/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Memory/Memory.h"
#include "Base/Math/Math.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Skeleton;
    using SecondarySkeletonList = TInlineVector<Skeleton const*, 3>;

    // Weight List
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API BoneWeightList final : public IReflectedType
    {
        EE_REFLECT_TYPE( BoneWeightList );
        EE_SERIALIZE( m_skeletonID, m_boneIDs, m_weights );

    public:

        bool IsValid() const;

        void Clear();

        inline int32_t GetNumWeights() const { return (int32_t) m_boneIDs.size(); }

        void SetWeightForBone( StringID boneID, float weight );

        void UnsetWeightForBone( StringID boneID );

        float GetWeightForBone( StringID boneID ) const;

        bool HasWeightForBone( StringID boneID ) const;

        // Fixups
        //-------------------------------------------------------------------------

        // Returns true if any modification was performed
        bool RemoveDuplicateWeights();

        // Returns true if any modification was performed
        bool RemoveWeightsForMissingBones( Skeleton const* pSkeleton );

    public:

        EE_REFLECT( ReadOnly );
        ResourceID                      m_skeletonID;

        EE_REFLECT( ReadOnly );
        TVector<StringID>               m_boneIDs;

        EE_REFLECT( ReadOnly );
        TVector<float>                  m_weights;
    };

    // Runtime bone mask
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

    public:

        BoneMask() : m_pSkeleton( nullptr ) {}
        BoneMask( Skeleton const* pSkeleton );
        BoneMask( Skeleton const* pSkeleton, float fixedWeight );
        BoneMask( Skeleton const* pSkeleton, BoneWeightList const& weightList, TInlineVector<StringID, 10>* pOutMissingBones = nullptr );
        BoneMask( Skeleton const* pSkeleton, TVector<float> const& weights );

        BoneMask( BoneMask const& rhs );
        BoneMask( BoneMask&& rhs );

        BoneMask& operator=( BoneMask const& rhs );
        BoneMask& operator=( BoneMask&& rhs );

        // Only copies the mask value but not the ID!
        inline void CopyFrom( BoneMask const& rhs ) { *this = rhs; }

        //-------------------------------------------------------------------------

        bool IsValid() const;
        inline Skeleton const* GetSkeleton() const { return m_pSkeleton; }
        inline int32_t GetNumWeights() const { return (int32_t) m_weights.size(); }
        inline float GetWeight( uint32_t i ) const { EE_ASSERT( i < (uint32_t) m_weights.size() ); return m_weights[i]; }
        inline TVector<float> const& GetWeights() const { return m_weights; }
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
        void ResetWeights() { ResetWeightsToZero(); }

        // Set all weights to zero
        void ResetWeightsToZero();

        // Set all weights to one
        void ResetWeightsToOne();

        // Set all weights to a fixed weight
        void ResetWeights( float fixedWeight );

        // Set all weights to the supplied weights, note that the number of supplied weights MUST match the expected num of weights!!
        // i.e. This will also feather bones so if head is set to 1.0f, pelvis is set to 0.2f, and intermediate bones are set to -1.0f then the intermediate spine bone weights will be gradually increased till they match the head weight
        inline void ResetWeights( TVector<float> const& weights )
        {
            EE_ASSERT( weights.size() == GetNumRequiredWeights() );
            ResetWeights( weights.data() );
        }

        // Set all weights to the supplied weights, note that the number of supplied weights MUST match the expected num of weights!!
        // i.e. This will also feather bones so if head is set to 1.0f, pelvis is set to 0.2f, and intermediate bones are set to -1.0f then the intermediate spine bone weights will be gradually increased till they match the head weight
        template<size_t S> void ResetWeights( TInlineVector<float, S> const& weights )
        {
            EE_ASSERT( weights.size() == GetNumRequiredWeights() );
            ResetWeights( weights.data() );
        }

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

        int32_t GetNumRequiredWeights() const;

        // Set all weights to the supplied weights, note that the number of supplied weights MUST match the expected num of weights!!
        // i.e. This will also feather bones so if head is set to 1.0f, pelvis is set to 0.2f, and intermediate bones are set to -1.0f then the intermediate spine bone weights will be gradually increased till they match the head weight
        void ResetWeights( float const* pWeights );

    private:

        WeightInfo                  m_weightInfo = WeightInfo::Zero;
        Skeleton const*             m_pSkeleton = nullptr;
        TVector<float>              m_weights;
    };

    // Bone mask set
    //-------------------------------------------------------------------------

    struct BoneMaskSet
    {
        BoneMask const* TryGetBoneMask( Skeleton const* pSkeleton ) const;

    public:

        StringID                        m_ID;
        BoneMask                        m_primaryMask;
        TInlineVector<BoneMask, 3>      m_secondaryMasks;
    };

    // Serialized bone mask definition
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API BoneMaskSetDefinition final : public IReflectedType
    {
        EE_REFLECT_TYPE( BoneMaskSetDefinition );
        EE_SERIALIZE( m_ID, m_primaryWeightList, m_secondaryWeightLists );

    public:

        BoneMaskSetDefinition() = default;
        BoneMaskSetDefinition( StringID ID ) : m_ID( ID ) {}

        bool IsValid() const;

    public:

        EE_REFLECT( ReadOnly );
        StringID                            m_ID;

        EE_REFLECT( ReadOnly );
        BoneWeightList                      m_primaryWeightList;

        EE_REFLECT( ReadOnly );
        TVector<BoneWeightList>             m_secondaryWeightLists;
    };
}