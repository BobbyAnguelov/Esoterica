#pragma once

#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class EntityWorld;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IViewportSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( IViewportSettings );

    public:

        // Get the friendly name for these settings
        virtual char const* GetFriendlyName() const = 0;

        // Draw a menu that provides editing options for these settings
        virtual void DrawMenu( EntityWorld* pWorld ) {}
    };
}
#endif