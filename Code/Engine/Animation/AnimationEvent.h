#pragma once

#include "Engine/_Module/API.h"
#include "System/Time/Time.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------
// Animation Event
//-------------------------------------------------------------------------
// Base class for all animation events

namespace EE
{
    class StringID;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API Event : public IRegisteredType
    {
        EE_REGISTER_TYPE( Event );

        friend class AnimationClipCompiler;

    public:

        Event() = default;
        virtual ~Event() = default;

        inline Seconds GetStartTime() const { return m_startTime; }
        inline Seconds GetDuration() const { return m_duration; }
        inline Seconds GetEndTime() const { EE_ASSERT( IsDurationEvent() ); return m_startTime + m_duration; }
        inline bool IsImmediateEvent() const { return m_duration == 0; }
        inline bool IsDurationEvent() const { return m_duration > 0; }

        // Get the time range for this event (in seconds)
        EE_FORCE_INLINE FloatRange GetTimeRange() const { return FloatRange( m_startTime, m_startTime + m_duration ); }

        // Optional: Allow the track to return a specific sync event ID
        virtual StringID GetSyncEventID() const;

        #if EE_DEVELOPMENT_TOOLS
        // Get a string description of the event for use when debugging
        virtual InlineString GetDebugText() const { return GetStaticTypeID().c_str(); }
        #endif

    protected:

        EE_REGISTER Seconds         m_startTime = 0.0f;
        EE_REGISTER Seconds         m_duration = 0.0f;
    };
}