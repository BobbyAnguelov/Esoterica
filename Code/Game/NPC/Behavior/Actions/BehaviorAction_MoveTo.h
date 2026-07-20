#pragma once
#include "Game/NPC/Behavior/BehaviorAction.h"
#include "Engine/Navmesh/NavPower.h"
#include "Base/Math/Vector.h"
#include "Base/Types/Percentage.h"

//-------------------------------------------------------------------------

namespace EE
{
    class BehaviorAction_MoveTo : public BehaviorAction
    {
    public:

        void Start( BehaviorContext const& ctx, Vector const& goalPosition );
        virtual Status Update( BehaviorContext const& ctx ) override;

    private:

        #if EE_ENABLE_NAVPOWER
        bfx::PolylinePathRCPtr      m_path;
        #endif

        int32_t                     m_currentPathSegmentIdx = InvalidIndex;
        Percentage                  m_progressAlongSegment = 0.0f;
    };
}