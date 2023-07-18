#include "AnimationEvent.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    StringID Event::GetSyncEventID() const
    {
        return StringID();
    }
}