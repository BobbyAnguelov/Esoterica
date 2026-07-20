#pragma once
#include "Engine/_Module/API.h"
#include "EntityIDs.h"
#include "Engine/UpdateContext.h"
#include "Engine/Viewport/Viewport.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EntityWorldSystem;
    class EntityWorld;
    class DebugDrawSystem;
    class DebugDrawContext;
    namespace EntityModel { class EntityMap; }
    namespace TypeSystem { class TypeInfo; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldUpdateContext : public UpdateContext
    {
    public:

        EntityWorldUpdateContext( UpdateContext const& context, EntityWorld* pWorld );

        // Update Info
        //-------------------------------------------------------------------------

        // Get the original delta time for this frame (without the world timescale applied)
        EE_FORCE_INLINE Seconds GetRawDeltaTime() const { return m_rawDeltaTime; }

        // Get the scaled delta time for this frame
        EE_FORCE_INLINE Seconds GetScaledDeltaTime() const { return m_deltaTime; }

        // Get the original delta time for this frame (without the world timescale applied)
        EE_FORCE_INLINE Seconds GetUnscaledDeltaTime() const { return GetRawDeltaTime(); }

        // Get the time scaling for the current world
        float GetTimeScale() const;

        // Set the time scaling for the current world
        void SetTimeScale( float timeScale ) const;

        // World Info
        //-------------------------------------------------------------------------

        // Get the world ID - threadsafe
        EntityWorldID const& GetWorldID() const;

        // Get an entity world system - threadsafe since these never changed during the lifetime of a world
        template<typename T> inline T* GetWorldSystem() const
        {
            static_assert( std::is_base_of<EE::EntityWorldSystem, T>::value, "T is not derived from IEntityWorldSystem" );
            return Cast<T>( GetWorldSystem( T::GetStaticTypeID() ) );
        }

        // Get the world type - threadsafe since this never changes
        EE_FORCE_INLINE bool IsGameWorld() const { return m_isGameWorld; }

        // Is the world paused? i.e. time scale <= 0.0f;
        inline bool IsWorldPaused() const { return m_isPaused; }

        // Get the persistent map - threadsafe - all dynamic entity creation is done in this map
        EntityModel::EntityMap* GetPersistentMap() const;

        // Viewports
        //-------------------------------------------------------------------------

        TInlineVector<Viewport*, 3> const& GetViewports() const;
        Viewport const* GetMainViewport() const;

        #if EE_DEVELOPMENT_TOOLS
        template<typename T>
        T const* GetViewportSettings() const
        {
            auto pViewport = GetMainViewport();
            if ( pViewport == nullptr )
            {
                return nullptr;
            }

            return pViewport->GetViewportSettings<T>();
        }
        #endif

        //  Debug Draw
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Get the debug drawing context for this world - threadsafe
        [[nodiscard]] DebugDrawContext GetDebugDrawContext() const;
        
        [[nodiscard]] DebugDrawSystem* GetDebugDrawSystem() const;
        #endif

    private:

        // Threadsafe since these never changed during the lifetime of a world
        EntityWorldSystem* GetWorldSystem( TypeSystem::TypeID worldSystemTypeID ) const;

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