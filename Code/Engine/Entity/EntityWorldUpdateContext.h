#pragma once
#include "Engine/_Module/API.h"
#include "EntityIDs.h"
#include "Engine/UpdateContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EntityWorldSystem;
    class EntityWorld;
    namespace Input { class InputState; }
    namespace Render { class Viewport; }
    namespace EntityModel{ class EntityMap; }
    namespace Drawing { class DrawingSystem; class DrawContext; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldUpdateContext : public UpdateContext
    {
    public:

        EntityWorldUpdateContext( UpdateContext const& context, EntityWorld* pWorld );

        // Get the original delta time for this frame (without the world timescale applied)
        EE_FORCE_INLINE Seconds GetRawDeltaTime() const { return m_rawDeltaTime; }

        // Get the time scaling for the current world
        float GetTimeScale() const;

        // Get the world ID - threadsafe
        EntityWorldID const& GetWorldID() const;

        // Get an entity world system - threadsafe since these never changed during the lifetime of a world
        template<typename T> inline T* GetWorldSystem() const
        {
            static_assert( std::is_base_of<EE::EntityWorldSystem, T>::value, "T is not derived from IEntityWorldSystem" );
            return Cast<T>( GetWorldSystem( T::s_entitySystemID ) );
        }

        // Get the world type - threadsafe since this never changes
        EE_FORCE_INLINE bool IsGameWorld() const { return m_isGameWorld; }

        // Is the world paused? i.e. time scale <= 0.0f;
        inline bool IsWorldPaused() const { return m_isPaused; }

        // Get the persistent map - threadsafe - all dynamic entity creation is done in this map
        EntityModel::EntityMap* GetPersistentMap() const;

        // Get the viewport for this world
        Render::Viewport const* GetViewport() const;

        // Get the input state for this world
        Input::InputState const* GetInputState() const;

        // Get the debug drawing context for this world - threadsafe
        #if EE_DEVELOPMENT_TOOLS
        [[nodiscard]] Drawing::DrawContext GetDrawingContext() const;
        [[nodiscard]] Drawing::DrawingSystem* GetDebugDrawingSystem() const;
        #endif

    private:

        // Threadsafe since these never changed during the lifetime of a world
        EntityWorldSystem* GetWorldSystem( uint32_t worldSystemID ) const;

        // Delete any ability to copy this struct
        explicit EntityWorldUpdateContext( EntityWorldUpdateContext const& ) = delete;
        explicit EntityWorldUpdateContext( EntityWorldUpdateContext&& ) = delete;
        EntityWorldUpdateContext& operator=( EntityWorldUpdateContext const& ) = delete;
        EntityWorldUpdateContext& operator=( EntityWorldUpdateContext&& ) = delete;

    private:

        EntityWorld*                    m_pWorld = nullptr;
        Seconds                         m_rawDeltaTime;
        bool                            m_isGameWorld = true;
        bool                            m_isPaused = false;
    };
}