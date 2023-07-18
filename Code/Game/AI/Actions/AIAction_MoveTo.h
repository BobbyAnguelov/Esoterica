#pragma once
#include "Engine/Navmesh/NavPower.h"
#include "Base/Math/Vector.h"
#include "Base/Types/Percentage.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    struct BehaviorContext;

    //-------------------------------------------------------------------------

    // Todo: think about more behavior centric name: MoveToChildBehavior, MoveToSubBehavior, MoveToActionBehavior??
    class MoveToAction
    {
    public:

        bool IsRunning() const;
        void Start( BehaviorContext const& ctx, Vector const& goalPosition );
        void Update( BehaviorContext const& ctx );

    private:

        #if EE_ENABLE_NAVPOWER
        bfx::PolylinePathRCPtr      m_path;
        #endif

        int32_t                       m_currentPathSegmentIdx = InvalidIndex;
        Percentage                  m_progressAlongSegment = 0.0f;
    };
}