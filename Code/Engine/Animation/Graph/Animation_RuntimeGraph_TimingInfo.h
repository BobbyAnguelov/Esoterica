#pragma once
#include "Engine/Animation/AnimationSyncTrack.h"
#include "Base/Serialization/BitSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    // Per-Layer Timing Info
    //-------------------------------------------------------------------------

    struct GraphLayerSyncInfo
    {
        GraphLayerSyncInfo() = default;
        GraphLayerSyncInfo( int8_t nLayerIdx, SyncTrackTimeRange &&timeRange ) : m_layerIdx( nLayerIdx ), m_syncTimeRange( timeRange ) {}

    public:

        int8_t                                      m_layerIdx = InvalidIndex;
        SyncTrackTimeRange                          m_syncTimeRange;
    };

    // Layer Node Timing Info
    //-------------------------------------------------------------------------

    struct GraphLayerUpdateState final
    {
        int16_t                                     m_nodeIdx = InvalidIndex; // The index of the layer node
        TInlineVector<GraphLayerSyncInfo, 5>        m_updateRanges; // The update range for each non-synchronized layer in order
    };

    // Graph Timing Info
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphTimeInfo final
    {

    public:

        struct ReferencedGraphState
        {
            int16_t                                 m_referencedGraphNodeIdx = InvalidIndex;
            GraphTimeInfo*                          m_pTimeInfo = nullptr;
        };

    public:

        GraphTimeInfo() = default;
        explicit GraphTimeInfo( Serialization::BitArchive &archive ) { Deserialize( archive ); }
        GraphTimeInfo( GraphTimeInfo const &rhs );
        GraphTimeInfo( GraphTimeInfo &&rhs );
        GraphTimeInfo &operator=( GraphTimeInfo const &rhs );
        GraphTimeInfo &operator=( GraphTimeInfo &&rhs );

        ~GraphTimeInfo()
        {
            Reset();
        }

        void Reset();

        void Serialize( Serialization::BitArchive &archive ) const;
        void Deserialize( Serialization::BitArchive &archive );

    public:

        SyncTrackTimeRange                          m_baseUpdateRange;
        TVector<GraphLayerUpdateState>              m_layerTimes;
        TVector<ReferencedGraphState>               m_referencedGraphStates;
    };
}