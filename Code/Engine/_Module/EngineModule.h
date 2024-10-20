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
#include "Engine/Animation/ResourceLoaders/ResourceLoader_IKRig.h"
#include "Engine/Navmesh/ResourceLoaders/ResourceLoader_Navmesh.h"
#include "Engine/Render/RendererRegistry.h"
#include "Engine/Render/Renderers/WorldRenderer.h"
#include "Engine/Render/Renderers/DebugRenderer.h"
#include "Engine/Render/Renderers/ImguiRenderer.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMaterial.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMesh.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderShader.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderTexture.h"
#include "Engine/Console/Console.h"
#include "Base/Application/Module.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct ModuleContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EngineModule final : public Module
    {
        EE_REFLECT_MODULE;

    public:

        virtual bool InitializeModule( ModuleContext const& context ) override;
        virtual void ShutdownModule( ModuleContext const& context ) override;

        //-------------------------------------------------------------------------

        virtual TInlineVector<Resource::ResourcePtr*, 4> GetModuleResources() const override;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline Console* GetConsole() { return &m_console; }
        #endif

        inline EntityWorldManager* GetEntityWorldManager() { return &m_entityWorldManager; }
        inline Render::RendererRegistry* GetRendererRegistry() { return &m_rendererRegistry; }

    private:

        // Console
        #if EE_DEVELOPMENT_TOOLS
        Console                                         m_console;
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
        Animation::IKRigLoader                          m_IKRigLoader;

        // Physics
        Physics::CollisionMeshLoader                    m_physicsCollisionMeshLoader;
        Physics::PhysicsMaterialDatabaseLoader          m_physicsMaterialLoader;
        Physics::RagdollLoader                          m_physicsRagdollLoader;
        Physics::MaterialRegistry                       m_physicsMaterialRegistry;
        TResourcePtr<Physics::MaterialDatabase>         m_physicsMaterialDB = ResourceID( "data://Physics/PhysicsMaterials.pmdb" );

        #if EE_DEVELOPMENT_TOOLS
        Physics::PhysicsRenderer                        m_physicsRenderer;
        #endif

        // Navmesh
        Navmesh::NavmeshLoader                          m_navmeshLoader;
    };
}