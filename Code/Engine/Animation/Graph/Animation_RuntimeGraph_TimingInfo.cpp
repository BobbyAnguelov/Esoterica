#include "Animation_RuntimeGraph_TimingInfo.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphTimeInfo::GraphTimeInfo( GraphTimeInfo const &rhs )
    {
        operator=( rhs );
    }

    GraphTimeInfo::GraphTimeInfo( GraphTimeInfo &&rhs )
    {
        operator=( eastl::move( rhs ) );
    }

    GraphTimeInfo &GraphTimeInfo::operator=( GraphTimeInfo const &rhs )
    {
        m_baseUpdateRange = rhs.m_baseUpdateRange;
        m_layerTimes = rhs.m_layerTimes;

        for ( auto rgs : rhs.m_referencedGraphStates )
        {
            ReferencedGraphState& newRGS = m_referencedGraphStates.emplace_back();
            newRGS.m_referencedGraphNodeIdx = rgs.m_referencedGraphNodeIdx;
            newRGS.m_pTimeInfo = EE::New<GraphTimeInfo>( *rgs.m_pTimeInfo );
        }

        return *this;
    }

    GraphTimeInfo &GraphTimeInfo::operator=( GraphTimeInfo &&rhs )
    {
        m_baseUpdateRange = rhs.m_baseUpdateRange;
        m_layerTimes = rhs.m_layerTimes;

        // Move all memory ownership
        m_referencedGraphStates.swap( rhs.m_referencedGraphStates );
        return *this;
    }

    void GraphTimeInfo::Reset()
    {
        m_baseUpdateRange.Reset();

        for ( ReferencedGraphState &rgs : m_referencedGraphStates )
        {
            EE::Delete( rgs.m_pTimeInfo );
            rgs.m_pTimeInfo = nullptr;
        }

        m_referencedGraphStates.clear();
        m_layerTimes.clear();
    }

    //-------------------------------------------------------------------------

    static void SerializeType( Serialization::BitArchive &serializer, SyncTrackTime const& time )
    {
        EE_ASSERT( time.m_eventIdx >= 0 && time.m_eventIdx < 255 );
        serializer.WriteUInt( (uint32_t) time.m_eventIdx, 8 );
        serializer.WriteNormalizedFloat8Bit( time.m_percentageThrough.ToFloat() );
    }

    static void SerializeType( Serialization::BitArchive &serializer, SyncTrackTimeRange const &timeRange )
    {
        SerializeType( serializer, timeRange.m_startTime );
        SerializeType( serializer, timeRange.m_endTime );
    }

    static void SerializeType( Serialization::BitArchive &serializer, GraphLayerUpdateState const &layerState )
    {
        EE_ASSERT( layerState.m_nodeIdx != InvalidIndex );
        serializer.WriteUInt( layerState.m_nodeIdx, 16 );

        EE_ASSERT( layerState.m_updateRanges.size() < 128 );
        serializer.WriteUInt( (uint32_t) layerState.m_updateRanges.size(), 7 );

        for ( auto &updateRange : layerState.m_updateRanges )
        {
            EE_ASSERT( updateRange.m_layerIdx >= 0 && updateRange.m_layerIdx < 32 );
            serializer.WriteUInt( (uint32_t) updateRange.m_layerIdx, 5 );

            SerializeType( serializer, updateRange.m_syncTimeRange );
        }
    }

    static void DeserializeType( Serialization::BitArchive &serializer, SyncTrackTime &time )
    {
        time.m_eventIdx = (int32_t) serializer.ReadUInt( 8 );
        time.m_percentageThrough = Percentage( serializer.ReadNormalizedFloat8Bit() );
    }

    static void DeserializeType( Serialization::BitArchive &serializer, SyncTrackTimeRange &timeRange )
    {
        DeserializeType( serializer, timeRange.m_startTime );
        DeserializeType( serializer, timeRange.m_endTime );
    }

    static void DeserializeType( Serialization::BitArchive &serializer, GraphLayerUpdateState &layerState )
    {
        layerState.m_nodeIdx = (int16_t) serializer.ReadUInt( 16 );

        uint8_t numLayerUpdateRanges = (uint8_t) serializer.ReadUInt( 7 );
        for ( int32_t i = 0; i < numLayerUpdateRanges; i++ )
        {
            GraphLayerSyncInfo &layerUpdateRange = layerState.m_updateRanges.emplace_back();

            layerUpdateRange.m_layerIdx = (int8_t) serializer.ReadUInt( 5 );

            DeserializeType( serializer, layerUpdateRange.m_syncTimeRange );
        }
    }

    void GraphTimeInfo::Serialize( Serialization::BitArchive &serializer ) const
    {
        SerializeType( serializer, m_baseUpdateRange );

        EE_ASSERT( m_layerTimes.size() < UINT8_MAX );
        serializer.WriteUInt( (uint32_t) m_layerTimes.size(), 8 );

        for ( auto &layerTime : m_layerTimes )
        {
            SerializeType( serializer, layerTime );
        }

        EE_ASSERT( m_referencedGraphStates.size() < UINT8_MAX );
        serializer.WriteUInt( (uint32_t) m_referencedGraphStates.size(), 8 );

        for ( auto &rgs : m_referencedGraphStates )
        {
            EE_ASSERT( rgs.m_referencedGraphNodeIdx != InvalidIndex );
            serializer.WriteUInt( rgs.m_referencedGraphNodeIdx, 16 );

            rgs.m_pTimeInfo->Serialize( serializer );
        }
    }

    void GraphTimeInfo::Deserialize( Serialization::BitArchive &serializer )
    {
        Reset();

        //-------------------------------------------------------------------------

        DeserializeType( serializer, m_baseUpdateRange );

        uint8_t numLayers = (uint8_t) serializer.ReadUInt( 8 );
        for ( int32_t i = 0; i < numLayers; i++ )
        {
            GraphLayerUpdateState &newLayerTime = m_layerTimes.emplace_back();
            DeserializeType( serializer, newLayerTime );
        }

        uint8_t numReferencedGraphs = (uint8_t) serializer.ReadUInt( 8 );
        for ( int32_t i = 0; i < numReferencedGraphs; i++ )
        {
            ReferencedGraphState &rgs = m_referencedGraphStates.emplace_back();
            rgs.m_referencedGraphNodeIdx = (int16_t) serializer.ReadUInt( 16 );
            rgs.m_pTimeInfo = EE::New<GraphTimeInfo>();

            rgs.m_pTimeInfo->Deserialize( serializer );
        }
    }

}