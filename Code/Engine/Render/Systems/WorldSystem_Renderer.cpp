#include "WorldSystem_Renderer.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Shaders/EngineShaders.h"
#include "Engine/Render/Settings/WorldSettings_Render.h"
#include "Base/Render/RenderCoreResources.h"
#include "Base/Render/RenderViewport.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void RendererWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {}

    void RendererWorldSystem::ShutdownSystem()
    {
        EE_ASSERT( m_registeredStaticMeshComponents.empty() );
        EE_ASSERT( m_registeredSkeletalMeshComponents.empty() );
        EE_ASSERT( m_skeletalMeshGroups.empty() );

        EE_ASSERT( m_registeredDirectionLightComponents.empty() );
        EE_ASSERT( m_registeredPointLightComponents.empty() );
        EE_ASSERT( m_registeredSpotLightComponents.empty() );

        EE_ASSERT( m_registeredLocalEnvironmentMaps.empty() );
        EE_ASSERT( m_registeredGlobalEnvironmentMaps.empty() );
    }

    void RendererWorldSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( auto pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            RegisterStaticMeshComponent( pEntity, pStaticMeshComponent );
        }
        else if ( auto pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            RegisterSkeletalMeshComponent( pEntity, pSkeletalMeshComponent );
        }
        
        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                m_registeredDirectionLightComponents.Add( pDirectionalLightComponent );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_registeredPointLightComponents.Add( pPointLightComponent );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_registeredSpotLightComponents.Add( pSpotLightComponent );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredLocalEnvironmentMaps.Add( pLocalEnvMapComponent );
        }
        else if ( auto pGlobalEnvMapComponent = TryCast<GlobalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredGlobalEnvironmentMaps.Add( pGlobalEnvMapComponent );
        }
    }

    void RendererWorldSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( auto pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            UnregisterStaticMeshComponent( pEntity, pStaticMeshComponent );
        }
        else if ( auto pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            UnregisterSkeletalMeshComponent( pEntity, pSkeletalMeshComponent );
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                m_registeredDirectionLightComponents.Remove( pDirectionalLightComponent->GetID() );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_registeredPointLightComponents.Remove( pPointLightComponent->GetID() );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_registeredSpotLightComponents.Remove( pSpotLightComponent->GetID() );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredLocalEnvironmentMaps.Remove( pLocalEnvMapComponent->GetID() );
        }
        else if ( auto pGlobalEnvMapComponent = TryCast<GlobalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredGlobalEnvironmentMaps.Remove( pGlobalEnvMapComponent->GetID() );
        }
    }

    void RendererWorldSystem::RegisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent )
    {
        m_registeredStaticMeshComponents.Add( pMeshComponent );

        //-------------------------------------------------------------------------

        // Add to appropriate sub-list
        if ( pMeshComponent->HasMeshResourceSet() )
        {
            m_staticMeshComponents.Add( pMeshComponent );
        }
    }

    void RendererWorldSystem::UnregisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent )
    {
        if ( pMeshComponent->HasMeshResourceSet() )
        {
            // Remove from the relevant runtime list
            m_staticMeshComponents.Remove( pMeshComponent->GetID() );
        }

        // Remove record
        m_registeredStaticMeshComponents.Remove( pMeshComponent->GetID() );
    }

    void RendererWorldSystem::RegisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent )
    {
        m_registeredSkeletalMeshComponents.Add( pMeshComponent );

        // Add to mesh groups
        //-------------------------------------------------------------------------

        if ( pMeshComponent->HasMeshResourceSet() )
        {
            auto pMesh = pMeshComponent->GetMesh();
            EE_ASSERT( pMesh != nullptr && pMesh->IsValid() );
            uint32_t const meshID = pMesh->GetResourceID().GetPathID();

            auto pMeshGroup = m_skeletalMeshGroups.FindOrAdd( meshID, pMesh );
            pMeshGroup->m_components.emplace_back( pMeshComponent );
        }
    }

    void RendererWorldSystem::UnregisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent )
    {
        // Unregistrations occur at the start of the frame
        // The world might be paused so we might leave an invalid component in this array
        m_visibleSkeletalMeshComponents.clear();

        // Remove component from mesh group
        if ( pMeshComponent->HasMeshResourceSet() )
        {
            uint32_t const meshID = pMeshComponent->GetMesh()->GetResourceID().GetPathID();
            auto pMeshGroup = m_skeletalMeshGroups.Get( meshID );
            pMeshGroup->m_components.erase_first_unsorted( pMeshComponent );

            // Remove empty groups
            if ( pMeshGroup->m_components.empty() )
            {
                m_skeletalMeshGroups.Remove( meshID );
            }
        }

        // Remove record
        m_registeredSkeletalMeshComponents.Remove( pMeshComponent->GetID() );
    }

    //-------------------------------------------------------------------------

    void RendererWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        EE_PROFILE_FUNCTION_RENDER();

        if ( ctx.IsWorldPaused() && ctx.GetUpdateStage() != UpdateStage::Paused )
        {
            return;
        }

        EE_ASSERT( ( ctx.GetUpdateStage() == UpdateStage::Paused ) ? ctx.IsWorldPaused() : true );

        //-------------------------------------------------------------------------
        // Culling
        //-------------------------------------------------------------------------

        AABB const viewBounds = ctx.GetViewport()->GetViewVolume().GetAABB();

        m_visibleStaticMeshComponents.clear();

        for ( auto const& pMeshComponent : m_staticMeshComponents )
        {
            EE_PROFILE_SCOPE_RENDER( "Static Mesh Cull" );

            if ( pMeshComponent->IsVisible() && viewBounds.Overlaps( pMeshComponent->GetWorldBounds() ) )
            {
                m_visibleStaticMeshComponents.emplace_back( pMeshComponent );
            }
        }

        m_visibleSkeletalMeshComponents.clear();

        for ( auto const& meshGroup : m_skeletalMeshGroups )
        {
            EE_PROFILE_SCOPE_RENDER( "Skeletal Mesh Cull" );

            for ( auto pMeshComponent : meshGroup.m_components )
            {
                if ( pMeshComponent->IsVisible() && viewBounds.Overlaps( pMeshComponent->GetWorldBounds() ) )
                {
                    m_visibleSkeletalMeshComponents.emplace_back( pMeshComponent );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS

        auto const* pRenderSettings = ctx.GetSettings<Render::RenderWorldSettings>();

        Drawing::DrawContext drawCtx = ctx.GetDrawingContext();

        if ( pRenderSettings->m_showStaticMeshBounds )
        {
            for ( auto const& pMeshComponent : m_registeredStaticMeshComponents )
            {
                if ( pMeshComponent->IsVisible() )
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan );
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen );
                }
                else
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan.GetAlphaVersion( 0.2f ) );
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen.GetAlphaVersion( 0.2f ) );
                }
            }
        }

        for ( auto const& pMeshComponent : m_registeredSkeletalMeshComponents )
        {
            if ( pRenderSettings->m_showSkeletalMeshBounds )
            {
                if ( pMeshComponent->IsVisible() )
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan );
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen );
                }
                else
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan.GetAlphaVersion( 0.2f ) );
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen.GetAlphaVersion( 0.2f ) );
                }
            }

            if ( pRenderSettings->m_showSkeletalMeshBones )
            {
                pMeshComponent->DrawPose( drawCtx );
            }

            if ( pRenderSettings->m_showSkeletalMeshBindPoses )
            {
                pMeshComponent->GetMesh()->DrawBindPose( drawCtx, pMeshComponent->GetWorldTransform() );
            }
        }
        #endif
    }
}