#pragma once

#include "API.h"
#include "Engine/Entity/EntityWorldManager.h"
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
    struct GameModuleContext
    {
        SystemRegistry*                     m_pSystemRegistry = nullptr;
        TaskSystem*                         m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry*           m_pTypeRegistry = nullptr;
        Resource::ResourceSystem*           m_pResourceSystem = nullptr;
        EntityWorldManager*                 m_pEntityWorldManager = nullptr;
        Render::RenderDevice*               m_pRenderDevice = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_GAME_API GameModule final
    {
        EE_REGISTER_MODULE;

    public:

        static void GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs );

    public:

        bool InitializeModule( GameModuleContext& context, IniFile const& iniFile );
        void ShutdownModule( GameModuleContext& context );

        // Resource Loading
        void LoadModuleResources( Resource::ResourceSystem& resourceSystem );
        bool VerifyModuleResourceLoadingComplete();
        void UnloadModuleResources( Resource::ResourceSystem& resourceSystem );

    private:

        bool                                            m_moduleInitialized = false;
    };
}