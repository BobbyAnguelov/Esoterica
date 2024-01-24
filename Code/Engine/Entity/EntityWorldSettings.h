#pragma once
#include "Engine/_Module/API.h"
#include "Base/Settings/Settings.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API IEntityWorldSettings : public Settings::ISettings
    {
        EE_REFLECT_TYPE( IEntityWorldSettings );
    };
}