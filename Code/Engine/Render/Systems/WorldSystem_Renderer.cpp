#include "WorldSystem_Renderer.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Shaders/EngineShaders.h"
#include "Base/Render/RenderCoreResources.h"
#include "Base/Render/RenderViewport.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Profiling.h"


//-------------------------------------------------------------------------

namespace EE::Render
{
    void RendererWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        m_staticMeshMobilityChangedEventBinding = StaticMeshComponent::OnMobilityChanged().Bind( [this] ( StaticMeshComponent* pMeshComponent ) { OnStaticMeshMobilityUpdated( pMeshComponent ); } );
        m_staticMeshStaticTransformUpdatedEventBinding = StaticMeshComponent::OnStaticMobilityTransformUpdated().Bind( [this] ( StaticMeshComponent* pMeshComponent ) { OnStaticMobilityComponentTransformUpdated( pMeshComponent ); } );
    }

    void RendererWorldSystem::ShutdownSystem()
    {
        // Unbind mobility change handler and remove from various lists
        StaticMeshComponent::OnStaticMobilityTransformUpdated().Unbind( m_staticMeshStaticTransformUpdatedEventBinding );
        StaticMeshComponent::OnMobilityChanged().Unbind( m_staticMeshMobilityChangedEventBinding );

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
            if ( pMeshComponent->GetMobility() == Mobility::Dynamic )
            {
                m_dynamicStaticMeshComponents.Add( pMeshComponent );
            }
            else
            {
                m_staticStaticMeshComponents.Add( pMeshComponent );
                m_staticMobilityTree.InsertBox( pMeshComponent->GetWorldBounds().GetAABB(), pMeshComponent );
            }
        }
    }

    void RendererWorldSystem::UnregisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent )
    {
        // Unregistrations occur at the start of the frame
        // The world might be paused so we might leave an invalid component in this array
        m_visibleStaticMeshComponents.clear();

        if ( pMeshComponent->HasMeshResourceSet() )
        {
            // Remove from any transform update lists
            int32_t const staticMobilityTransformListIdx = VectorFindIndex( m_staticMobilityTransformUpdateList, pMeshComponent );
            if ( staticMobilityTransformListIdx != InvalidIndex )
            {
                m_staticMobilityTransformUpdateList.erase_unsorted( m_staticMobilityTransformUpdateList.begin() + staticMobilityTransformListIdx );
            }

            // Get the real mobility of the component
            Mobility realMobility = pMeshComponent->GetMobility();
            int32_t const mobilityListIdx = VectorFindIndex( m_mobilityUpdateList, pMeshComponent );
            if ( mobilityListIdx != InvalidIndex )
            {
                realMobility = ( realMobility == Mobility::Dynamic ) ? Mobility::Static : Mobility::Dynamic;
                m_mobilityUpdateList.erase_unsorted( m_mobilityUpdateList.begin() + mobilityListIdx );
            }

            // Remove from the relevant runtime list
            if ( realMobility == Mobility::Dynamic )
            {
                m_dynamicStaticMeshComponents.Remove( pMeshComponent->GetID() );
            }
            else
            {
                m_staticStaticMeshComponents.Remove( pMeshComponent->GetID() );
                m_staticMobilityTree.RemoveBox( pMeshComponent );
            }
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
        // Mobility Updates
        //-------------------------------------------------------------------------

        for ( auto pMeshComponent : m_mobilityUpdateList )
        {
            Mobility const mobility = pMeshComponent->GetMobility();

            // Convert from static to dynamic
            if ( mobility == Mobility::Dynamic )
            {
                m_staticMobilityTree.RemoveBox( pMeshComponent );
                m_staticStaticMeshComponents.Remove( pMeshComponent->GetID() );
                m_dynamicStaticMeshComponents.Add( pMeshComponent );
            }
            else // Convert from dynamic to static
            {
                m_dynamicStaticMeshComponents.Remove( pMeshComponent->GetID() );
                m_staticStaticMeshComponents.Add( pMeshComponent );
                m_staticMobilityTree.InsertBox( pMeshComponent->GetWorldBounds().GetAABB(), pMeshComponent );
            }
        }

        m_mobilityUpdateList.clear();

        //-------------------------------------------------------------------------

        for ( auto pMeshComponent : m_staticMobilityTransformUpdateList )
        {
            if ( ctx.IsGameWorld() )
            {
                EE_LOG_ENTITY_ERROR( pMeshComponent, "Render", "Someone moved a mesh with static mobility: %s with entity ID %u. This should not be done!", pMeshComponent->GetNameID().c_str(), pMeshComponent->GetEntityID().m_value );
            }

            m_staticMobilityTree.RemoveBox( pMeshComponent );
            m_staticMobilityTree.InsertBox( pMeshComponent->GetWorldBounds().GetAABB(), pMeshComponent );
        }

        m_staticMobilityTransformUpdateList.clear();

        //-------------------------------------------------------------------------
        // Culling
        //-------------------------------------------------------------------------

        AABB const viewBounds = ctx.GetViewport()->GetViewVolume().GetAABB();

        m_visibleStaticMeshComponents.clear();
        {
            EE_PROFILE_SCOPE_RENDER( "Static Mesh AABB Cull" );
            m_staticMobilityTree.FindOverlaps( viewBounds, m_visibleStaticMeshComponents );

            for ( int32_t i = int32_t( m_visibleStaticMeshComponents.size() ) - 1; i >= 0 ; i-- )
            {
                if ( !m_visibleStaticMeshComponents[i]->IsVisible() )
                {
                    m_visibleStaticMeshComponents.erase_unsorted( m_visibleStaticMeshComponents.begin() + i );
                }
            }
        }

        for ( auto pMeshComponent : m_dynamicStaticMeshComponents )
        {
            EE_PROFILE_SCOPE_RENDER( "Static Mesh Dynamic Cull" );

            if ( pMeshComponent->IsVisible() && viewBounds.Overlaps( pMeshComponent->GetWorldBounds() ) )
            {
                m_visibleStaticMeshComponents.emplace_back( pMeshComponent );
            }
        }

        //-------------------------------------------------------------------------

        m_visibleSkeletalMeshComponents.clear();

        for ( auto const& meshGroup : m_skeletalMeshGroups )
        {
            EE_PROFILE_SCOPE_RENDER( "Skeletal Mesh Dynamic Cull" );

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
        Drawing::DrawContext drawCtx = ctx.GetDrawingContext();

        if ( m_showStaticMeshBounds )
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
            if ( m_showSkeletalMeshBounds )
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

            if ( m_showSkeletalMeshBones )
            {
                pMeshComponent->DrawPose( drawCtx );
            }

            if ( m_showSkeletalMeshBindPoses )
            {
                pMeshComponent->GetMesh()->DrawBindPose( drawCtx, pMeshComponent->GetWorldTransform() );
            }
        }
        #endif
    }

    //-------------------------------------------------------------------------

    void RendererWorldSystem::OnStaticMeshMobilityUpdated( StaticMeshComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() );

        if ( m_registeredStaticMeshComponents.HasItemForID( pComponent->GetID() ) )
        {
            Threading::ScopeLock lock( m_mobilityUpdateListLock );
            EE_ASSERT( !VectorContains( m_mobilityUpdateList, pComponent ) );
            m_mobilityUpdateList.emplace_back( pComponent );
        }
    }

    void RendererWorldSystem::OnStaticMobilityComponentTransformUpdated( StaticMeshComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() );
        EE_ASSERT( pComponent->GetMobility() == Mobility::Static );

        if ( !pComponent->HasMeshResourceSet() )
        {
            return;
        }

        if ( m_registeredStaticMeshComponents.HasItemForID( pComponent->GetID() ) )
        {
            Threading::ScopeLock lock( m_mobilityUpdateListLock );
            if ( !VectorContains( m_staticMobilityTransformUpdateList, pComponent ) )
            {
                m_staticMobilityTransformUpdateList.emplace_back( pComponent );
            }
        }
    }
}