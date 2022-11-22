#include "EngineToolsModule.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool EngineToolsModule::InitializeModule( ModuleContext& context, IniFile const& iniFile )
    {
        return true;
    }

    void EngineToolsModule::ShutdownModule( ModuleContext& context )
    {
    }
}