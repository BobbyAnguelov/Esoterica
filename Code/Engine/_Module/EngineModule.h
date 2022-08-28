#pragma once

#include "API.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/ResourceLoaders/ResourceLoader_EntityCollection.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Physics/Debug/PhysicsDebugRenderer.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsMaterialDatabase.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsMesh.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsRagdoll.h"
#include "Engine/Animation/ResourceLoaders/AnimationSkeletonLoader.h"
#include "Engine/Animation/ResourceLoaders/AnimationClipLoader.h"
#include "Engine/Animation/ResourceLoaders/AnimationGraphLoader.h"
#include "Engine/Animation/ResourceLoaders/AnimationBoneMaskLoader.h"
#include "Engine/Navmesh/ResourceLoaders/ResourceLoader_Navmesh.h"
#include "Engine/Navmesh/NavmeshSystem.h"
#include "Engine/Render/RendererRegistry.h"
#include "Engine/Render/Renderers/WorldRenderer.h"
#include "Engine/Render/Renderers/DebugRenderer.h"
#include "Engine/Render/Renderers/ImguiRenderer.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMaterial.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMesh.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderShader.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderTexture.h"

#include "System/Imgui/ImguiX.h"
#include "System/Input/InputSystem.h"
#include "System/Imgui/ImguiSystem.h"
#include "System/Render/RenderDevice.h"
#include "System/Resource/ResourceProvider.h"
#include "System/Resource/ResourceSystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Threading/TaskSystem.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API EngineModule final
    {
        EE_REGISTER_MODULE;

        static void GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs );

    public:

        bool InitializeCoreSystems( IniFile const& iniFile );
        void ShutdownCoreSystems();

        bool InitializeModule();
        void ShutdownModule();

        void LoadModuleResources( Resource::ResourceSystem& resourceSystem );
        bool VerifyModuleResourceLoadingComplete();
        void UnloadModuleResources( Resource::ResourceSystem& resourceSystem );

        //-------------------------------------------------------------------------

        inline SystemRegistry* GetSystemRegistry() { return &m_systemRegistry; }
        inline TaskSystem* GetTaskSystem() { return &m_taskSystem; }
        inline TypeSystem::TypeRegistry* GetTypeRegistry() { return &m_typeRegistry; }
        inline Input::InputSystem* GetInputSystem() { return &m_inputSystem; }
        inline Resource::ResourceSystem* GetResourceSystem() { return &m_resourceSystem; }
        inline Render::RenderDevice* GetRenderDevice() { return m_pRenderDevice; }
        inline EntityWorldManager* GetEntityWorldManager() { return &m_entityWorldManager; }
        inline Render::RendererRegistry* GetRendererRegistry() { return &m_rendererRegistry; }
        inline Physics::PhysicsSystem* GetPhysicsSystem() { return &m_physicsSystem; }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline ImGuiX::ImguiSystem* GetImguiSystem() { return &m_imguiSystem; }
        void EnableImguiViewports() { m_imguiViewportsEnabled = true; }
        #endif

    private:

        bool                                            m_moduleInitialized = false;

        // System
        TaskSystem                                      m_taskSystem;
        TypeSystem::TypeRegistry                        m_typeRegistry;
        SystemRegistry                                  m_systemRegistry;
        Input::InputSystem                              m_inputSystem;

        // Resource
        Resource::ResourceProvider*                     m_pResourceProvider = nullptr;
        Resource::ResourceSystem                        m_resourceSystem = Resource::ResourceSystem( m_taskSystem );

        // ImGui
        #if EE_DEVELOPMENT_TOOLS
        ImGuiX::ImguiSystem                             m_imguiSystem;
        bool                                            m_imguiViewportsEnabled = false;
        #endif

        // Entity
        EntityWorldManager                              m_entityWorldManager;
        EntityModel::EntityCollectionLoader             m_entityCollectionLoader;

        // Rendering
        Render::RenderDevice*                           m_pRenderDevice = nullptr;
        Render::MeshLoader                              m_renderMeshLoader;
        Render::ShaderLoader                            m_shaderLoader;
        Render::TextureLoader                           m_textureLoader;
        Render::MaterialLoader                          m_materialLoader;
        Render::RendererRegistry                        m_rendererRegistry;
        Render::WorldRenderer                           m_worldRenderer;

        #if EE_DEVELOPMENT_TOOLS
        Render::ImguiRenderer                           m_imguiRenderer;
        Render::DebugRenderer                           m_debugRenderer;
        #endif

        // Animation
        Animation::SkeletonLoader                       m_skeletonLoader;
        Animation::BoneMaskLoader                       m_boneMaskLoader;
        Animation::AnimationClipLoader                  m_animationClipLoader;
        Animation::GraphLoader                          m_graphLoader;

        // Physics
        Physics::PhysicsSystem                          m_physicsSystem;
        Physics::PhysicsMeshLoader                      m_physicsMeshLoader;
        Physics::PhysicsMaterialDatabaseLoader          m_physicsMaterialLoader;
        Physics::RagdollLoader                          m_physicsRagdollLoader;
        TResourcePtr<Physics::PhysicsMaterialDatabase>  m_pPhysicMaterialDB;

        #if EE_DEVELOPMENT_TOOLS
        Physics::PhysicsRenderer                        m_physicsRenderer;
        #endif

        // Navmesh
        Navmesh::NavmeshLoader                          m_navmeshLoader;
        Navmesh::NavmeshSystem                          m_navmeshSystem;
    };
}