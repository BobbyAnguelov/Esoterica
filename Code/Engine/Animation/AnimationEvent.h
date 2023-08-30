#pragma once

#include "Engine/_Module/API.h"
#include "Base/Time/Time.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Math/NumericRange.h"

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
    class EE_ENGINE_API Event : public IReflectedType
    {
        EE_REFLECT_TYPE( Event );

        friend class AnimationClipCompiler;

    public:

        Event() = default;
        Event( Event const& ) = default;
        virtual ~Event() = default;

        Event& operator=( Event const& rhs ) = default;

        virtual bool IsValid() const { return true; }

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

        EE_REFLECT( "IsToolsReadOnly" : true ) Seconds         m_startTime = 0.0f;
        EE_REFLECT( "IsToolsReadOnly" : true ) Seconds         m_duration = 0.0f;
    };
}