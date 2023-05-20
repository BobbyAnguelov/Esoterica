#pragma once

#include "API.h"
#include "System/TypeSystem/ReflectedType.h"
#include "Engine/ModuleContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    class IniFile;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EngineToolsModule
    {
        EE_REFLECT_MODULE;

    public:

        bool InitializeModule( ModuleContext& context, IniFile const& iniFile );
        void ShutdownModule( ModuleContext& context );
    };
}