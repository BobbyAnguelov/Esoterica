#pragma once
#include "Engine/Animation/AnimationEvent.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API IDEvent final : public Event
    {
        EE_REFLECT_TYPE( IDEvent );

    public:

        inline StringID const& GetID() const { return m_ID; }
        virtual StringID GetSyncEventID() const override { return m_ID; }
        virtual bool IsValid() const override{ return m_ID.IsValid(); }

        #if EE_DEVELOPMENT_TOOLS
        virtual InlineString GetDebugText() const override { return m_ID.IsValid() ? m_ID.c_str() : "Invalid ID"; }
        #endif

    private:

        EE_REFLECT() StringID         m_ID;
    };
}