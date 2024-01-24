#include "GameModule.h"

//-------------------------------------------------------------------------

namespace EE
{
    void GameModule::GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs )
    {

    }

    bool GameModule::InitializeModule( ModuleContext& context )
    {
        return true;
    }

    void GameModule::ShutdownModule( ModuleContext& context )
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