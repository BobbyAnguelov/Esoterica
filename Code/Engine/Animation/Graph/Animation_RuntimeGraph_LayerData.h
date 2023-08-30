#pragma once

#include "Engine/Animation/AnimationSyncTrack.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    // Layer Context
    //-------------------------------------------------------------------------
    // Used to track layer data

    struct GraphLayerContext final
    {
        BoneMaskTaskList                                            m_layerMaskTaskList;
        float                                                       m_layerWeight = 1.0f;
        float                                                       m_rootMotionLayerWeight = 1.0f;
    };

    // Layer Update state
    //-------------------------------------------------------------------------

    struct GraphLayerUpdateState final
    {
        int16_t                                                     m_nodeIdx; // The index of the layer node
        TInlineVector<TPair<int8_t, SyncTrackTimeRange>, 5>         m_updateRanges; // The update range for each non-synchronized layer in order
    };
}