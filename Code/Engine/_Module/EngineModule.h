#pragma once

#include "API.h"
#include "Base/Application/Module.h"
#include "Base/Render/RenderWindow.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/ResourceLoaders/ResourceLoader_EntityCollection.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsMaterialDatabase.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsCollisionMesh.h"
#include "Engine/Physics/ResourceLoaders/ResourceLoader_PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Engine/Animation/ResourceLoaders/ResourceLoader_AnimationSkeleton.h"
#include "Engine/Animation/ResourceLoaders/ResourceLoader_AnimationClip.h"
#include "Engine/Animation/ResourceLoaders/ResourceLoader_AnimationGraph.h"
#include "Engine/Hitbox/ResourceLoaders/ResourceLoader_Hitbox.h"
#include "Engine/Navmesh/ResourceLoaders/ResourceLoader_Navmesh.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMaterial.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderMesh.h"
#include "Engine/Render/ResourceLoaders/ResourceLoader_RenderTexture.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Render/RenderSystem.h"
#include "Engine/Render/Renderer_ForwardShading.h"

#if EE_DEVELOPMENT_TOOLS
#include "Engine/Render/Imgui/ImguiRenderer.h"
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#endif

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

        virtual void OnModuleResourceLoaded( Resource::ResourcePtr* pResource ) override;
        virtual void OnModuleResourceUnload( Resource::ResourcePtr* pResource ) override;

        //-------------------------------------------------------------------------

        virtual TInlineVector<Resource::ResourcePtr*, 10> GetModuleResources() const override;

        //-------------------------------------------------------------------------

        inline Render::Window* GetRenderWindow() { return &m_renderWindow; }
        inline Render::ForwardShadingRenderer* GetForwardShadingRenderer() { return &m_forwardShadingRenderer; }

        #if EE_DEVELOPMENT_TOOLS
        inline Render::ImguiRenderer* GetImguiRenderer() { return &m_imguiRenderer; }
        #endif

        inline EntityWorldManager* GetEntityWorldManager() { return &m_entityWorldManager; }

        #if EE_ENABLE_LPP
        virtual void LivePP_PreReload( ModuleContext const& context ) override;
        virtual void LivePP_PostReload( ModuleContext const& context ) override;
        #endif

    private:

        // Entity
        EntityWorldManager                              m_entityWorldManager;
        EntityModel::EntityCollectionLoader             m_entityCollectionLoader;

        // Rendering
        Render::MeshLoader                              m_meshLoader;
        Render::TextureLoader                           m_textureLoader;
        Render::MaterialLoader                          m_materialLoader;

        Render::RenderSystem                            m_renderSystem;
        Render::Window                                  m_renderWindow;
        TResourcePtr<Render::TextureResource>           m_tonemapLUT = ResourceID( "data://Render/TonyMcMapface/TonyMcMapFaceLUT.texture" );
        TResourcePtr<Render::TextureResource>           m_smaaAreaTexture = ResourceID( "data://Render/SMAA/AreaTexture.texture" );
        TResourcePtr<Render::TextureResource>           m_smaaSearchTexture = ResourceID( "data://Render/SMAA/SearchTexture.texture" );
        TResourcePtr<Render::Material>                  m_placeholderMaterial = ResourceID( "data://Render/Materials/PlaceholderMaterial.material" );

        Render::ForwardShadingRenderer                  m_forwardShadingRenderer;

        #if EE_DEVELOPMENT_TOOLS
        Render::DebugMeshRegistry                       m_debugMeshRegistry;
        Render::ImguiRenderer                           m_imguiRenderer;
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
        TResourcePtr<Physics::MaterialDatabase>         m_physicsMaterialDB = ResourceID( "data://Physics/PhysicsMaterials.pmdb" );

        // Navmesh
        Navmesh::NavmeshLoader                          m_navmeshLoader;

        // Game
        HitboxLoader                                    m_hitboxLoader;

        #if EE_ENABLE_LPP
        EventBindingID                                  m_lppReloadEventBindingID;
        TVector<FileSystem::Path>                       m_modifiedFiles; // The set of files that were modified and reloaded
        bool                                            m_needShaderReload = false;
        #endif
    };
}