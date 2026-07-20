#include "AnimationFloatChannels.h"
#include "Base/Memory/Memory.h"
#include "Base/Math/Lerp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool FloatChannelSet::RemoveDuplicateChannelIDs()
    {
        bool fixupPerformed = false;

        for ( int32_t i = 0; i < m_channelIDs.size(); i++ )
        {
            for ( int32_t j = i + 1; j < m_channelIDs.size(); j++ )
            {
                if ( m_channelIDs[i] == m_channelIDs[j] )
                {
                    m_channelIDs.erase_unsorted( m_channelIDs.begin() + j );
                    j--;
                    fixupPerformed = true;
                }
            }
        }

        return fixupPerformed;
    }

    //-------------------------------------------------------------------------

    void FloatChannelSetValues::Reset()
    {
        Memory::MemsetZero( m_values.data(), m_values.size() );
        m_state = State::Unset;
    }

    int32_t FloatChannelSetValues::GetNumRequiredValuesToAllocate() const
    {
        EE_ASSERT( m_pSet != nullptr );
        int32_t const numValues = m_pSet->GetNumChannels();
        return Math::CeilingToInt32( float( numValues ) / 4.0f ) * 4;
    }

    FloatChannelSetValues::FloatChannelSetValues( FloatChannelSetValues const &rhs )
    {
        CopyFrom( rhs );
    }

    FloatChannelSetValues &FloatChannelSetValues::operator=( FloatChannelSetValues const& rhs )
    {
        CopyFrom( rhs );
        return *this;
    }

    FloatChannelSetValues &FloatChannelSetValues::operator=( FloatChannelSetValues&& rhs )
    {
        EE_ASSERT( rhs.m_values.size() == rhs.GetNumRequiredValuesToAllocate() );

        const_cast<FloatChannelSet const *&>( m_pSet ) = rhs.m_pSet;
        m_values.swap( rhs.m_values );
        m_state = rhs.m_state;
        return *this;
    }

    void FloatChannelSetValues::CopyFrom( FloatChannelSetValues const& rhs )
    {
        EE_ASSERT( rhs.m_values.size() == rhs.GetNumRequiredValuesToAllocate() );

        m_values.resize( rhs.GetNumRequiredValuesToAllocate() );

        if ( m_pSet != rhs.m_pSet || rhs.IsSet() )
        {
            m_values = rhs.m_values;
        }

        const_cast<FloatChannelSet const*&>( m_pSet ) = rhs.m_pSet;
        m_state = rhs.m_state;
    }

    void FloatChannelSetValues::SwapWith( FloatChannelSetValues &rhs )
    {
        FloatChannelSet const* const pTempSet = m_pSet;
        const_cast<FloatChannelSet const*&>( m_pSet ) = rhs.m_pSet;
        const_cast<FloatChannelSet const*&>( rhs.m_pSet ) = pTempSet;

        State const tempState = m_state;
        m_state = rhs.m_state;
        rhs.m_state = tempState;

        m_values.swap( rhs.m_values );
    }

    //-------------------------------------------------------------------------

    void FloatChannelData::GetValues( FrameTime const& frameTime, FloatChannelSetValues &outValues ) const
    {
        EE_ASSERT( m_pSkeleton != nullptr );
        EE_ASSERT( outValues.GetSetID() == GetSetID() );

        //-------------------------------------------------------------------------

        int32_t const numfloatCurves = GetNumChannels();

        EE_ASSERT( numfloatCurves > 0 );

        auto ReadCompressedFloatCurveValues = [&] ( int32_t poseIdx, float *pChannelValues )
        {
            uint16_t const *pReadPtr = m_compressedData.data() + m_compressedOffsets[poseIdx];

            bool hasNonZeroValue = false;

            for ( auto i = 0; i < numfloatCurves; i++ )
            {
                ChannelSettings const &compressionSettings = m_channelSettings[i];
                if ( compressionSettings.m_isStatic )
                {
                    pChannelValues[i] = compressionSettings.GetStaticValue();
                }
                else
                {
                    pChannelValues[i] = Quantization::DecodeFloat( *pReadPtr, compressionSettings.m_range.m_rangeStart, compressionSettings.m_range.m_rangeLength );
                    pReadPtr += 1;
                }

                hasNonZeroValue |= !Math::IsNearZero( pChannelValues[i] );
            }

            return hasNonZeroValue;
        };

        // Read Values
        //-------------------------------------------------------------------------

        float *pValuesData = outValues.m_values.data();

        // Read the lower curve value pose
        bool hasNonZeroValue = ReadCompressedFloatCurveValues( frameTime.GetLowerBoundFrameIndex(), pValuesData );

        // If we're not exactly at a key frame we need to read the upper frame value pose and blend
        if ( !frameTime.IsExactlyAtKeyFrame() )
        {
            TInlineVector<float, 200> tmpCurveValues;
            tmpCurveValues.resize( numfloatCurves );
            ReadCompressedFloatCurveValues( frameTime.GetUpperBoundFrameIndex(), tmpCurveValues.data() );

            float const percentageThrough = frameTime.GetPercentageThrough();
            for ( int32_t i = 0; i < numfloatCurves; i++ )
            {
                pValuesData[i] = Math::Lerp( percentageThrough, pValuesData[i], tmpCurveValues[i] );
                hasNonZeroValue |= !Math::IsNearZero( pValuesData[i] );
            }
        }

        outValues.m_state = hasNonZeroValue ? FloatChannelSetValues::State::Set : FloatChannelSetValues::State::Unset;
    }
}