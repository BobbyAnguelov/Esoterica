#pragma once

#include "API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Application/Module.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAMETOOLS_API GameToolsModule : public Module
    {
        EE_REFLECT_MODULE;

    public:

        virtual bool InitializeModule( ModuleContext const& context ) override;
        virtual void ShutdownModule( ModuleContext const& context ) override;
    };
}
