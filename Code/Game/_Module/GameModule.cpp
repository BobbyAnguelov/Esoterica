#include "GameModule.h"

//-------------------------------------------------------------------------

namespace EE
{
    void GameModule::GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs )
    {

    }

    bool GameModule::InitializeModule( GameModuleContext& context, IniFile const& iniFile )
    {
        return true;
    }

    void GameModule::ShutdownModule( GameModuleContext& context )
    {
    }

    void GameModule::LoadModuleResources( Resource::ResourceSystem& resourceSystem )
    {

    }

    bool GameModule::VerifyModuleResourceLoadingComplete()
    {
        return true;
    }

    void GameModule::UnloadModuleResources( Resource::ResourceSystem& resourceSystem )
    {

    }
}