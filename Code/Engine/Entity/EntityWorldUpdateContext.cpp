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

    Settings::ISettings* EntityWorldUpdateContext::GetSettings( TypeSystem::TypeInfo const* pTypeInfo )
    {
        return m_pWorld->GetSettings( pTypeInfo );
    }

    EntityWorldSystem* EntityWorldUpdateContext::GetWorldSystem( uint32_t worldSystemID ) const
    {
        return m_pWorld->GetWorldSystem( worldSystemID );
    }

    EntityModel::EntityMap* EntityWorldUpdateContext::GetPersistentMap() const
    {
        return m_pWorld->GetPersistentMap();
    }

    Render::Viewport const* EntityWorldUpdateContext::GetViewport() const
    {
        return m_pWorld->GetViewport();
    }

    float EntityWorldUpdateContext::GetTimeScale() const
    {
        return m_pWorld->GetTimeScale();
    }

    EntityWorldID const& EntityWorldUpdateContext::GetWorldID() const
    {
        return m_pWorld->GetID();
    }

    #if EE_DEVELOPMENT_TOOLS
    Drawing::DrawContext EntityWorldUpdateContext::GetDrawingContext() const
    {
        return m_pWorld->m_debugDrawingSystem.GetDrawingContext();
    }

    Drawing::DrawingSystem* EntityWorldUpdateContext::GetDebugDrawingSystem() const
    {
        return m_pWorld->GetDebugDrawingSystem();
    }
    #endif
}