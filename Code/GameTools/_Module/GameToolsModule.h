#pragma once

#include "API.h"
#include "System/TypeSystem/RegisteredType.h"
#include "Engine/ModuleContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    class IniFile;

    //-------------------------------------------------------------------------

    class EE_GAMETOOLS_API GameToolsModule
    {
        EE_REGISTER_MODULE;

    public:

        bool InitializeModule( ModuleContext& context, IniFile const& iniFile );
        void ShutdownModule( ModuleContext& context );
    };
}
