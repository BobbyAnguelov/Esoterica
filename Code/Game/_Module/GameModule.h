#pragma once

#include "API.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/ModuleContext.h"
#include "System/Resource/ResourceID.h"
#include "System/Input/InputSystem.h"
#include "System/Render/RenderDevice.h"
#include "System/Resource/ResourceSystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API GameModule final
    {
        EE_REFLECT_MODULE;

    public:

        static void GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs );

    public:

        bool InitializeModule( ModuleContext& context, IniFile const& iniFile );
        void ShutdownModule( ModuleContext& context );

        // Resource Loading
        void LoadModuleResources( Resource::ResourceSystem& resourceSystem );
        bool VerifyModuleResourceLoadingComplete();
        void UnloadModuleResources( Resource::ResourceSystem& resourceSystem );

    private:

        bool                                            m_moduleInitialized = false;
    };
}