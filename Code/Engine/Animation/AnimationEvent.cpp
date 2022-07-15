#include "AnimationEvent.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    StringID Event::GetSyncEventID() const
    {
        return StringID();
    }
}