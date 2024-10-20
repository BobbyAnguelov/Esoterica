#pragma once

#include "API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Input/InputSystem.h"
#include "Base/Settings/SettingsRegistry.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Application/Module.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Imgui/ImguiSystem.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API BaseModule : public Module
    {
        EE_REFLECT_MODULE;

    public:

        BaseModule();

        ModuleContext GetModuleContext() const;

        virtual bool InitializeModule( ModuleContext const& context ) override;
        virtual void ShutdownModule( ModuleContext const& context ) override;

        //-------------------------------------------------------------------------

        inline SystemRegistry* GetSystemRegistry() { return &m_systemRegistry; }
        inline TaskSystem* GetTaskSystem() { return &m_taskSystem; }
        inline TypeSystem::TypeRegistry* GetTypeRegistry() { return &m_typeRegistry; }
        inline Input::InputSystem* GetInputSystem() { return &m_inputSystem; }
        inline Resource::ResourceSystem* GetResourceSystem() { return &m_resourceSystem; }
        inline Render::RenderDevice* GetRenderDevice() { return &m_renderDevice; }
        inline Settings::SettingsRegistry* GetSettingsRegistry() { return &m_settingsRegistry; }

        #if EE_DEVELOPMENT_TOOLS
        inline ImGuiX::ImguiSystem* GetImguiSystem() { return &m_imguiSystem; }
        #endif

    private:

        TaskSystem                                      m_taskSystem;
        TypeSystem::TypeRegistry                        m_typeRegistry;
        SystemRegistry                                  m_systemRegistry;
        Settings::SettingsRegistry                      m_settingsRegistry;
        Input::InputSystem                              m_inputSystem;
        Resource::ResourceSystem                        m_resourceSystem;
        Resource::ResourceProvider*                     m_pResourceProvider = nullptr;
        Render::RenderDevice                            m_renderDevice;

        #if EE_DEVELOPMENT_TOOLS
        ImGuiX::ImguiSystem                             m_imguiSystem;
        #endif
    };
}