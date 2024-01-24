#pragma once

#include "API.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/ModuleContext.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Input/InputSystem.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API GameModule final
    {
        EE_REFLECT_MODULE;

    public:

        static void GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs );

    public:

        bool InitializeModule( ModuleContext& context );
        void ShutdownModule( ModuleContext& context );

        // Resource Loading
        void LoadModuleResources( Resource::ResourceSystem& resourceSystem );
        bool VerifyModuleResourceLoadingComplete();
        void UnloadModuleResources( Resource::ResourceSystem& resourceSystem );

    private:

        bool                                            m_moduleInitialized = false;
    };
}