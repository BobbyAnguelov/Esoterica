#pragma once

#include "AnimationFrameTime.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Arrays.h"
#include "Base/Encoding/Quantization.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Skeleton;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatChannelSet final : public IReflectedType
    {
        EE_REFLECT_TYPE( FloatChannelSet );
        EE_SERIALIZE( m_ID, m_channelIDs );

    public:

        FloatChannelSet() = default;
        FloatChannelSet( StringID ID ) : m_ID( ID ) {}

        FloatChannelSet( FloatChannelSet const &rhs )
        {
            m_ID = rhs.m_ID;
            m_channelIDs = rhs.m_channelIDs;
        }

        bool IsValid() const { return m_ID.IsValid() && !m_channelIDs.empty(); }

        inline bool operator==( StringID const& ID ) const { return m_ID == ID; }

        inline int32_t GetNumChannels() const { return (int32_t) m_channelIDs.size(); }
        inline StringID GetChannelID( int32_t idx ) const { return m_channelIDs[idx]; }

        // This is a naive linear search, avoid using this!
        inline int32_t GetChannelIndex( StringID const ID ) const { return VectorFindIndex( m_channelIDs, ID ); }

        //-------------------------------------------------------------------------

        // Returns true if a modification was made
        bool RemoveDuplicateChannelIDs();

    public:

        EE_REFLECT();
        StringID                                                m_ID;

        EE_REFLECT();
        TVector<StringID>                                       m_channelIDs;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatChannelSetValues final
    {
        friend class FloatChannelData;
        friend class Blender;

        enum class State
        {
            Unset,
            Set,
        };

    public:

        FloatChannelSetValues() = default;

        FloatChannelSetValues( FloatChannelSet const *pSet )
            : m_pSet( pSet )
        {
            EE_ASSERT( m_pSet != nullptr );
            m_values.resize( GetNumRequiredValuesToAllocate(), 0.0f );
        }

        FloatChannelSetValues( FloatChannelSetValues const &rhs );
        FloatChannelSetValues &operator=( FloatChannelSetValues &&rhs );
        FloatChannelSetValues &operator=( FloatChannelSetValues const &rhs );

        inline bool IsValid() const { return m_pSet != nullptr; }

        inline StringID GetSetID() const { return m_pSet != nullptr ? m_pSet->m_ID : StringID(); }

        // Copy
        //-------------------------------------------------------------------------

        void CopyFrom( FloatChannelSetValues const &rhs );
        EE_FORCE_INLINE void CopyFrom( FloatChannelSetValues const *pRhs ) { CopyFrom( *pRhs ); }

        // Swap all internals with another pose
        void SwapWith( FloatChannelSetValues &rhs );

        // Info
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE int32_t GetNumChannels() const { return m_pSet->GetNumChannels(); }
        EE_FORCE_INLINE StringID GetChannelID( int32_t idx ) const { EE_ASSERT( IsValid() ); return m_pSet->GetChannelID( idx ); }
        EE_FORCE_INLINE float GetChannelValue( int32_t idx ) const { return m_values[idx]; }

        // This is a naive linear search, avoid using this!
        EE_FORCE_INLINE int32_t GetChannelIndex( StringID channelID ) const { return m_pSet->GetChannelIndex( channelID ); }

        // State
        //-------------------------------------------------------------------------

        void Reset();
        inline bool IsUnset() const { return m_state == State::Unset; }
        inline bool IsSet() const { return m_state == State::Set; }

    private:

        int32_t GetNumRequiredValuesToAllocate() const;

    public:

        FloatChannelSet const* const                        m_pSet = nullptr;
        TVector<float>                                      m_values; // WARNING! - the number of values here maybe larger than the number of channels in the set
        State                                               m_state = State::Unset;
    };

    //-------------------------------------------------------------------------

    class FloatChannelData final
    {
        friend class ResourceCompilerAnimationClip;
        friend class AnimationClipCompiler;
        friend class AnimationClipLoader;

    public:

        struct ChannelSettings
        {
        public:

            EE_FORCE_INLINE float GetStaticValue() const { return m_range.m_rangeStart; }

        public:

            Quantization::FloatRange                        m_range;
            bool                                            m_isStatic = false;
        };

    public:

        StringID GetSetID() const { return m_setID; }

        EE_FORCE_INLINE int32_t GetNumChannels() const { return (int32_t) m_channelSettings.size(); }

        // Get the float channels values at the specified time
        void GetValues( FrameTime const &frameTime, FloatChannelSetValues &outValues ) const;

    private:

        Skeleton const*                                     m_pSkeleton = nullptr;
        StringID                                            m_setID;
        TVector<ChannelSettings>                            m_channelSettings;
        TVector<uint16_t>                                   m_compressedData;
        TVector<uint32_t>                                   m_compressedOffsets;
    };
}