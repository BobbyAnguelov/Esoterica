#include "GameModule.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool GameModule::InitializeModule( ModuleContext const& context )
    {
        return true;
    }

    void GameModule::ShutdownModule( ModuleContext const& context )
    {}
}