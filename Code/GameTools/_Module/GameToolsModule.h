#pragma once

#include "API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Engine/ModuleContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAMETOOLS_API GameToolsModule
    {
        EE_REFLECT_MODULE;

    public:

        bool InitializeModule( ModuleContext& context );
        void ShutdownModule( ModuleContext& context );
    };
}
