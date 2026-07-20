#include "EntityWorldUpdateContext.h"
#include "EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE
{
    EntityWorldUpdateContext::EntityWorldUpdateContext( UpdateContext const& context, EntityWorld* pWorld )
        : UpdateContext( context )
        , m_pWorld( pWorld )
        , m_rawDeltaTime( m_deltaTime )
        , m_isGameWorld( pWorld->IsGameWorld() )
    {
        EE_ASSERT( m_pWorld != nullptr );

        // Handle paused world
        if ( pWorld->IsPaused() )
        {
            if ( pWorld->IsTimeStepRequested() )
            {
                m_deltaTime = pWorld->GetTimeStepLength();
            }
            else // Set delta time to 0.0f
            {
                m_deltaTime = 0.0f;
                m_isPaused = true;
            }
        }
        else // Apply world time scale
        {
            m_deltaTime *= pWorld->GetTimeScale();
        }

        EE_ASSERT( m_deltaTime >= 0.0f );
    }

    TInlineVector<EE::Viewport*, 3> const& EntityWorldUpdateContext::GetViewports() const
    {
        return m_pWorld->GetViewports();
    }

    Viewport const* EntityWorldUpdateContext::GetMainViewport() const
    {
        return m_pWorld->GetMainViewport();
    }

    EntityWorldSystem* EntityWorldUpdateContext::GetWorldSystem( TypeSystem::TypeID worldSystemTypeID ) const
    {
        return m_pWorld->GetWorldSystem( worldSystemTypeID );
    }

    EntityModel::EntityMap* EntityWorldUpdateContext::GetPersistentMap() const
    {
        return m_pWorld->GetPersistentMap();
    }

    float EntityWorldUpdateContext::GetTimeScale() const
    {
        return m_pWorld->GetTimeScale();
    }

    void EntityWorldUpdateContext::SetTimeScale( float timeScale ) const
    {
        EE_ASSERT( timeScale > 0.0f && timeScale < 100.0f );
        m_pWorld->SetTimeScale( timeScale );
    }

    EntityWorldID const& EntityWorldUpdateContext::GetWorldID() const
    {
        return m_pWorld->GetID();
    }

    #if EE_DEVELOPMENT_TOOLS
    DebugDrawContext EntityWorldUpdateContext::GetDebugDrawContext() const
    {
        return m_pWorld->m_debugDrawSystem.GetDebugDrawContext();
    }

    DebugDrawSystem* EntityWorldUpdateContext::GetDebugDrawSystem() const
    {
        return m_pWorld->GetDebugDrawSystem();
    }
    #endif
}