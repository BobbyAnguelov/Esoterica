#pragma once

#include "EntityWorldSystem.h"
#include "EntityActivationContext.h"
#include "EntityLoadingContext.h"
#include "Entity.h"
#include "EntityMap.h"
#include "Engine/Render/RenderViewport.h"
#include "System/Types/Arrays.h"
#include "System/Drawing/DebugDrawingSystem.h"
#include "System/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class TaskSystem;
    class UpdateContext;
    class EntityWorldDebugView;

    //-------------------------------------------------------------------------

    enum class EntityWorldType : uint8_t
    {
        Game,
        Tools
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorld
    {
        friend class EntityDebugView;
        friend class EntityWorldUpdateContext;

    public:

        EntityWorld( EntityWorldType worldType = EntityWorldType::Game ) : m_worldType( worldType ) {}
        ~EntityWorld();

        inline EntityWorldID const& GetID() const { return m_worldID; }
        inline bool IsGameWorld() const { return m_worldType == EntityWorldType::Game; }

        void Initialize( SystemRegistry const& systemsRegistry, TVector<TypeSystem::TypeInfo const*> worldSystemTypeInfos );
        void Shutdown();

        //-------------------------------------------------------------------------
        // Updates
        //-------------------------------------------------------------------------

        // Have the world updates been suspended?
        inline bool IsSuspended() const { return m_isSuspended; }

        // Put this world to sleep, no entity/system updates will be run
        void SuspendUpdates() { m_isSuspended = true; }

        // Wake this world, resumes execution of entity/system updates
        void ResumeUpdates() { m_isSuspended = false; }

        // Run entity and system updates
        void Update( UpdateContext const& context );

        // This function will handle all actual loading/unloading operations for the world/maps.
        // Any queued requests will be handled here as will any requests to the resource system.
        void UpdateLoading();

        //-------------------------------------------------------------------------
        // Systems
        //-------------------------------------------------------------------------

        IEntityWorldSystem* GetWorldSystem( uint32_t worldSystemID ) const;

        template<typename T>
        inline T* GetWorldSystem() const { return static_cast<T*>( GetWorldSystem( T::s_entitySystemID ) ); }

        //-------------------------------------------------------------------------
        // Input
        //-------------------------------------------------------------------------
        // Each world has it's own input state which is affected by the state of the player controller as well the time scale/pause/etc...
        // Each system can choose whether to use the global input state or the per world state depending on the needs
        // e.g., gameplay will likely need to use the world input state, whereas UI and menus will use the global input state

        Input::InputState const* GetInputState() const { return &m_inputState; }
        Input::InputState* GetInputState() { return &m_inputState; }

        //-------------------------------------------------------------------------
        // Time Management
        //-------------------------------------------------------------------------

        // Is the current world paused
        inline bool IsPaused() const { return m_timeScale <= 0.0f; }

        // Get the current time scale for this world
        inline float GetTimeScale() const { return m_timeScale; }

        // Get the current time scale for this world
        inline void SetTimeScale( float newTimeScale ) { m_timeScale = newTimeScale; }

        // Request a time step - only applicable to paused worlds
        inline void RequestTimeStep() { EE_ASSERT( IsPaused() ); m_timeStepRequested = true; }

        // Has a time-step for a paused world been requested?
        inline bool IsTimeStepRequested() const { return m_timeStepRequested; }

        // How long should each time step be for this world?
        inline Seconds GetTimeStepLength() const { return m_timeStepLength; }

        // Set the length of each time step for this world
        inline void SetTimeStepLength( Seconds newTimeStepLength ) { EE_ASSERT( newTimeStepLength > 0 ); m_timeStepLength = newTimeStepLength; }

        //-------------------------------------------------------------------------
        // Viewport
        //-------------------------------------------------------------------------

        inline Render::Viewport* GetViewport() { return &m_viewport; }
        inline Render::Viewport const* GetViewport() const { return &m_viewport; }

        //-------------------------------------------------------------------------
        // Map Management
        //-------------------------------------------------------------------------

        // Get the persistent map - this is a transient map that's always presents - be very careful with what you add to this map
        EntityModel::EntityMap* GetPersistentMap() { return &m_maps[0]; }
        EntityModel::EntityMap const* GetPersistentMap() const { return &m_maps[0]; }

        // Create a transient map (one that is managed programatically)
        EntityModel::EntityMap* CreateTransientMap();

        // Get a specific map
        EntityModel::EntityMap const* GetMap( ResourceID const& mapResourceID ) const;

        // Get a specific map
        EntityModel::EntityMap* GetMap( ResourceID const& mapResourceID ) { return const_cast<EntityModel::EntityMap*>( const_cast<EntityWorld const*>( this )->GetMap( mapResourceID ) ); }

        // Get a specific map
        EntityModel::EntityMap const* GetMap( EntityMapID const& mapID ) const;

        // Get a specific map
        EntityModel::EntityMap* GetMap( EntityMapID const& mapID ) { return const_cast<EntityModel::EntityMap*>( const_cast<EntityWorld const*>( this )->GetMap( mapID ) ); }

        // Are we currently loading anything
        bool IsBusyLoading() const;

        // Have we added this map to the world
        bool HasMap( ResourceID const& mapResourceID ) const;

        // Does the specified map exist and is fully loaded
        bool IsMapActive( ResourceID const& mapResourceID ) const;

        // Does the specified map exist and is fully loaded
        bool IsMapActive( EntityMapID const& mapID ) const;

        // These functions queue up load and unload requests to be processed during the next loading update for the world
        void LoadMap( ResourceID const& mapResourceID );
        void UnloadMap( ResourceID const& mapResourceID );

        // Find an entity in the map
        inline Entity* FindEntity( EntityID entityID ) const
        {
            Entity* pEntity = nullptr;
            for ( auto const& map : m_maps )
            {
                pEntity = map.FindEntity( entityID );
                if ( pEntity != nullptr )
                {
                    break;
                }
            }

            return pEntity;
        }

        //-------------------------------------------------------------------------
        // Editor
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS

        // This function will immediately unload the specified component so that its properties can be edited
        void PrepareComponentForEditing( EntityMapID const& mapID, EntityID const& entityID, ComponentID const& componentID );

        // Get all the registered components of the specified type
        inline TVector<EntityComponent const*> const& GetAllRegisteredComponentsOfType( TypeSystem::TypeID typeID ) { return m_componentTypeLookup[typeID]; }

        // Get all the registered components of the specified type
        template<typename T>
        inline TInlineVector<T const*, 20> GetAllRegisteredComponentsOfType() 
        {
            TInlineVector<T const*, 20> results;
            for ( auto pComponent : m_componentTypeLookup[T::GetStaticTypeID()] )
            {
                results.emplace_back( static_cast<T const*>( pComponent ) );
            }
            return results;
        }

        #endif

        //-------------------------------------------------------------------------
        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline TVector<EntityWorldDebugView*> const& GetDebugViews() const { return m_debugViews; }
        void InitializeDebugViews( SystemRegistry const& systemsRegistry, TVector<TypeSystem::TypeInfo const*> debugViewTypeInfos );
        void ShutdownDebugViews();

        inline String const& GetDebugName() const { return m_debugName; }
        inline void SetDebugName( String const& newName ) { m_debugName = newName; }
        inline void SetDebugName( char const* pName ) { EE_ASSERT( pName != nullptr ); m_debugName = pName; }

        inline Drawing::DrawingSystem* GetDebugDrawingSystem() { return &m_debugDrawingSystem; }
        inline void ResetDebugDrawingSystem() { m_debugDrawingSystem.Reset(); }
        #endif

        //-------------------------------------------------------------------------
        // Hot Reload
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Starts the hot-reload process - deactivates and unloads all specified entities
        void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload );

        // Ends the hot-reload process - starts re-loading of unloaded entities
        void EndHotReload();
        #endif

    private:

        // Process entity registration/unregistration requests occurring during map loading
        void ProcessEntityRegistrationRequests();

        // Process component registration/unregistration requests occurring during map loading
        void ProcessComponentRegistrationRequests();

    private:

        EntityWorldID                                                           m_worldID = UUID::GenerateID();
        TaskSystem*                                                             m_pTaskSystem = nullptr;
        Input::InputState                                                       m_inputState;
        EntityModel::EntityLoadingContext                                       m_loadingContext;
        EntityModel::ActivationContext                                          m_activationContext;
        TVector<IEntityWorldSystem*>                                            m_worldSystems;
        EntityWorldType                                                         m_worldType = EntityWorldType::Game;
        bool                                                                    m_initialized = false;
        bool                                                                    m_isSuspended = false;
        Render::Viewport                                                        m_viewport = Render::Viewport( Int2::Zero, Int2( 640, 480 ), Math::ViewVolume( Float2( 640, 480 ), FloatRange( 0.1f, 100.0f ) ) );

        // Maps
        TInlineVector<EntityModel::EntityMap, 3>                                m_maps;

        // Entities
        TVector<Entity*>                                                        m_entityUpdateList;
        TVector<IEntityWorldSystem*>                                            m_systemUpdateLists[(int8_t) UpdateStage::NumStages];

        // Time Scaling + Pause
        float                                                                   m_timeScale = 1.0f; // <= 0 means that the world is paused
        Seconds                                                                 m_timeStepLength = 1.0f / 30.0f;
        bool                                                                    m_timeStepRequested = false;

        #if EE_DEVELOPMENT_TOOLS
        THashMap<TypeSystem::TypeID, TVector<EntityComponent const*>>           m_componentTypeLookup;
        Drawing::DrawingSystem                                                  m_debugDrawingSystem;
        TVector<EntityWorldDebugView*>                                          m_debugViews;
        String                                                                  m_debugName;
        #endif
    };
}