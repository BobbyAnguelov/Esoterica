#pragma once

//-------------------------------------------------------------------------

namespace EE
{
    class SystemRegistry;
    class TaskSystem;
    namespace TypeSystem { class TypeRegistry; }
    namespace Settings { class SettingsRegistry; }
    namespace Resource { class ResourceSystem; }
    namespace Render { class RenderDevice; }
    class EntityWorldManager;

    //-------------------------------------------------------------------------

    struct ModuleContext
    {
        SystemRegistry*                     m_pSystemRegistry = nullptr;
        TaskSystem*                         m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry*           m_pTypeRegistry = nullptr;
        Settings::SettingsRegistry*         m_pSettingsRegistry = nullptr;
        Resource::ResourceSystem*           m_pResourceSystem = nullptr;
        Render::RenderDevice*               m_pRenderDevice = nullptr;
        EntityWorldManager*                 m_pEntityWorldManager = nullptr;
    };
}