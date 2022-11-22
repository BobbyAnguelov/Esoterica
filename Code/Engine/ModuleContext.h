#pragma once

//-------------------------------------------------------------------------

namespace EE
{
    class SystemRegistry;
    class TaskSystem;
    namespace TypeSystem { class TypeRegistry; }
    namespace Resource { class ResourceSystem; }
    class EntityWorldManager;
    namespace Render { class RenderDevice; }

    //-------------------------------------------------------------------------

    struct ModuleContext
    {
        SystemRegistry*                     m_pSystemRegistry = nullptr;
        TaskSystem*                         m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry*           m_pTypeRegistry = nullptr;
        Resource::ResourceSystem*           m_pResourceSystem = nullptr;
        EntityWorldManager*                 m_pEntityWorldManager = nullptr;
        Render::RenderDevice*               m_pRenderDevice = nullptr;
    };
}