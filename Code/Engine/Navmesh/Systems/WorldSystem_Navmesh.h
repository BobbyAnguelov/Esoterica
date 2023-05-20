#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Navmesh/NavPower.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/UpdateContext.h"

//-------------------------------------------------------------------------
// Navmesh World System
//-------------------------------------------------------------------------
// This is the main system responsible for managing navmesh within a specific world
// Manages navmesh registration, obstacles creation/destruction, etc...
// Primarily also needed to get the space handle needed for any queries ( GetSpaceHandle )

namespace EE { struct AABB; }

//-------------------------------------------------------------------------


namespace EE::Navmesh
{
    class NavmeshComponent;
    namespace Navpower { class Renderer; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NavmeshWorldSystem : public IEntityWorldSystem
    {
        friend class NavmeshDebugView;
        friend class NavmeshDebugRenderer;

        //-------------------------------------------------------------------------

        struct RegisteredNavmesh
        {
            RegisteredNavmesh( ComponentID const& ID, char* pNavmesh ) : m_componentID( ID ), m_pNavmesh( pNavmesh ) { EE_ASSERT( ID.IsValid() && pNavmesh != nullptr ); }

            ComponentID     m_componentID;
            char*           m_pNavmesh;
        };

    public:

        EE_ENTITY_WORLD_SYSTEM( NavmeshWorldSystem, RequiresUpdate( UpdateStage::Physics ) );

    public:

        NavmeshWorldSystem() = default;

        AABB GetNavmeshBounds( uint32_t layerIdx ) const;

        #if EE_ENABLE_NAVPOWER
        EE_FORCE_INLINE bfx::SpaceHandle GetSpaceHandle() const { return bfx::GetDefaultSpaceHandle( m_pInstance ); }
        #endif

    private:

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override;
        virtual void ShutdownSystem() override;

        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;

        void RegisterNavmesh( NavmeshComponent* pComponent );
        void UnregisterNavmesh( NavmeshComponent* pComponent );

        void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

        #if EE_DEVELOPMENT_TOOLS
        bool IsDebugRendererDepthTestEnabled() const;
        void SetDebugRendererDepthTestState( bool isDepthTestingEnabled );
        #endif

    private:

        #if EE_ENABLE_NAVPOWER
        bfx::Instance*                                  m_pInstance = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        Navpower::Renderer*                             m_pRenderer = nullptr;
        #endif

        #endif

        //-------------------------------------------------------------------------

        TVector<NavmeshComponent*>                      m_navmeshComponents;
        TVector<RegisteredNavmesh>                      m_registeredNavmeshes;
    };
}