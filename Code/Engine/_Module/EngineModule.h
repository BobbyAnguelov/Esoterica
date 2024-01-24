#pragma once

#include "API.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/ResourceLoaders/ResourceLoader_EntityCollection.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsMaterialDatabase.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsCollisionMesh.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Physics/Debug/PhysicsDebugRenderer.h"
#include "Engine/Animation/ResourceLoaders/ResourceLoader_AnimationSkeleton.h"
#include "Engine/Animation/ResourceLoaders/ResourceLoader_AnimationClip.h"
#include "Engine/Animation/ResourceLoaders/ResourceLoader_AnimationGraph.h"
#include "Engine/Navmesh/ResourceLoaders/ResourceLoader_Navmesh.h"
#include "Engine/Render/RendererRegistry.h"
#include "Engine/Render/Renderers/WorldRenderer.h"
#include "Engine/Render/Renderers/DebugRenderer.h"
#include "Engine/Render/Renderers/ImguiRenderer.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMaterial.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMesh.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderShader.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct ModuleContext;
    class Console;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EngineModule final
    {
        EE_REFLECT_MODULE;

        static void GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs );

    public:

        bool InitializeModule( ModuleContext& context );
        void ShutdownModule( ModuleContext& context );

        //-------------------------------------------------------------------------

        // Resource Loading
        void LoadModuleResources( Resource::ResourceSystem& resourceSystem );
        bool VerifyModuleResourceLoadingComplete();
        void UnloadModuleResources( Resource::ResourceSystem& resourceSystem );

        //-------------------------------------------------------------------------

        inline Console* GetConsole() { return m_pConsole; }
        inline EntityWorldManager* GetEntityWorldManager() { return &m_entityWorldManager; }
        inline Render::RendererRegistry* GetRendererRegistry() { return &m_rendererRegistry; }

    private:

        // Console
        #if EE_DEVELOPMENT_TOOLS
        Console*                                        m_pConsole = nullptr;
        #endif

        // Entity
        EntityWorldManager                              m_entityWorldManager;
        EntityModel::EntityCollectionLoader             m_entityCollectionLoader;

        // Rendering
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
        Animation::AnimationClipLoader                  m_animationClipLoader;
        Animation::GraphLoader                          m_graphLoader;

        // Physics
        Physics::CollisionMeshLoader                    m_physicsCollisionMeshLoader;
        Physics::PhysicsMaterialDatabaseLoader          m_physicsMaterialLoader;
        Physics::RagdollLoader                          m_physicsRagdollLoader;
        Physics::MaterialRegistry                       m_physicsMaterialRegistry;
        TResourcePtr<Physics::MaterialDatabase>         m_physicsMaterialDB;

        #if EE_DEVELOPMENT_TOOLS
        Physics::PhysicsRenderer                        m_physicsRenderer;
        #endif

        // Navmesh
        Navmesh::NavmeshLoader                          m_navmeshLoader;
    };
}