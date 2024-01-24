#pragma once

#include "EntityWorldSystem.h"
#include "EntityContexts.h"
#include "Entity.h"
#include "EntityMap.h"
#include "Base/Render/RenderViewport.h"
#include "Base/Types/Arrays.h"
#include "Base/Settings/SettingsRegistry.h"
#include "Base/Drawing/DebugDrawingSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class TaskSystem;
    class UpdateContext;
    class DebugView;

    namespace Settings { class SettingsRegistry; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorld
    {
        friend class EntityDebugView;
        friend class EntityWorldUpdateContext;

    public:

        EntityWorld( EntityWorldType worldType = EntityWorldType::Game );
        ~EntityWorld();

        inline EntityWorldID const& GetID() const { return m_worldID; }

        inline EntityWorldType GetWorldType() const { return m_worldType; }
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

        EntityWorldSystem* GetWorldSystem( uint32_t worldSystemID ) const;

        template<typename T>
        inline T* GetWorldSystem() const { return reinterpret_cast<T*>( GetWorldSystem( T::s_entitySystemID ) ); }

        //-------------------------------------------------------------------------
        // Settings
        //-------------------------------------------------------------------------

        // Get world settings
        template<typename T> T* GetSettings() { return m_pSettingsRegistry->GetSettings<T>( m_worldID.m_value ); }

        // Get world settings
        template<typename T> T* GetMutableSettings() const { return const_cast<EntityWorld*>( this )->GetSettings<T>(); }

        // Get world settings
        template<typename T> T const* GetSettings() const { return const_cast<EntityWorld*>( this )->GetSettings<T>(); }

        // Get world settings
        Settings::ISettings* GetSettings( TypeSystem::TypeInfo const* pTypeInfo ) { return m_pSettingsRegistry->GetSettings( m_worldID.m_value, pTypeInfo ); }

        // Get world settings
        Settings::ISettings* GetMutableSettings( TypeSystem::TypeInfo const* pTypeInfo ) const { return const_cast<EntityWorld*>( this )->GetSettings( pTypeInfo ); }

        // Get world settings
        Settings::ISettings const* GetSettings( TypeSystem::TypeInfo const* pTypeInfo ) const { return const_cast<EntityWorld*>( this )->GetSettings( pTypeInfo ); }

        //-------------------------------------------------------------------------
        // Time Management
        //-------------------------------------------------------------------------

        // Is the current world paused
        inline bool IsPaused() const { return m_timeScale <= 0.0f; }

        // Pause the update of this world
        inline void SetPaused() { m_timeScale = 0.0f; }

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
        EntityModel::EntityMap* GetPersistentMap() { return m_maps[0]; }

        // Get the persistent map - this is a transient map that's always presents - be very careful with what you add to this map
        EntityModel::EntityMap const* GetPersistentMap() const { return m_maps[0]; }

        // Get the first non-persistent map
        EntityModel::EntityMap* GetFirstNonPersistentMap() { return ( m_maps.size() > 1 ) ? m_maps[1] : nullptr; }

        // Get the first non-persistent map
        EntityModel::EntityMap const* GetFirstNonPersistentMap() const { return ( m_maps.size() > 1 ) ? m_maps[1] : nullptr; }

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

        // Get map for a given entity
        EntityModel::EntityMap const* GetMapForEntity( Entity const* pEntity ) const;

        // Get map for a given entity
        EntityModel::EntityMap* GetMapForEntity( Entity const* pEntity ) { return const_cast<EntityModel::EntityMap*>( const_cast<EntityWorld const*>( this )->GetMapForEntity( pEntity ) ); }

        // Are we currently loading anything
        bool IsBusyLoading() const;

        // Have we added this map to the world
        bool HasMap( ResourceID const& mapResourceID ) const;

        // Do we have a map with this ID?
        bool HasMap( EntityMapID const& mapID ) const;

        // Does the specified map exist and is fully loaded
        bool IsMapLoaded( ResourceID const& mapResourceID ) const;

        // Does the specified map exist and is fully loaded
        bool IsMapLoaded( EntityMapID const& mapID ) const;

        // These functions queue up load and unload requests to be processed during the next loading update for the world
        EntityMapID LoadMap( ResourceID const& mapResourceID );
        void UnloadMap( ResourceID const& mapResourceID );

        // Find an entity in the map
        inline Entity* FindEntity( EntityID entityID ) const
        {
            Entity* pEntity = nullptr;
            for ( auto const& pMap : m_maps )
            {
                pEntity = pMap->FindEntity( entityID );
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
        // This function will immediately shutdown and unload all components so that component properties can be edited.
        void BeginComponentEdit( Entity* pEntity );

        // End a bulk component edit operation, will request all components to be reloaded
        void EndComponentEdit( Entity* pEntity );

        // This function will immediately shutdown and unload the specified component so that its properties can be edited
        // Note:  do not call this multiple times in a row, if you need to modify multiple components on the same entity use the functions above
        void BeginComponentEdit( EntityComponent* pComponent );

        // End a component edit operation, will request the unloaded component to be reloaded
        void EndComponentEdit( EntityComponent* pComponent );

        // Get all the registered components of the specified type
        inline TVector<EntityComponent const*> const& GetAllRegisteredComponentsOfType( TypeSystem::TypeID typeID ) { return m_componentTypeLookup[typeID]; }

        // Get all the registered components of the specified type
        template<typename T>
        inline TInlineVector<T const*, 20> GetAllRegisteredComponentsOfType() 
        {
            TInlineVector<T const*, 20> results;
            for ( auto pComponent : m_componentTypeLookup[T::GetStaticTypeID()] )
            {
                results.emplace_back( reinterpret_cast<T const*>( pComponent ) );
            }
            return results;
        }

        // Are any maps currently in the process of loading or have any add/remove entity actions pending
        inline bool HasPendingMapChangeActions() const
        {
            for ( auto pMap : m_maps )
            {
                if ( pMap->IsLoading() )
                {
                    return true;
                }

                if ( pMap->HasPendingAddOrRemoveRequests() )
                {
                    return true;
                }
            }
            return false;
        }

        // This function will update all maps so as to complete any entity removal requests
        // WARNING!!! Be very careful when you call this
        inline void ProcessAllRemovalRequests()
        {
            // Shutdown Entities
            UpdateLoading();
            // Complete Removal
            UpdateLoading();
        }
        #endif

        //-------------------------------------------------------------------------
        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
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
        // Starts the hot-reload process - shuts down and unloads all specified entities
        void HotReload_UnloadEntities( TVector<Resource::ResourceRequesterID> const& usersToReload );

        // Ends the hot-reload process - starts re-loading of unloaded entities
        void HotReload_ReloadEntities();
        #endif

    private:

        EntityWorldID                                                           m_worldID = EntityWorldID::Generate();
        TaskSystem*                                                             m_pTaskSystem = nullptr;
        Settings::SettingsRegistry*                                             m_pSettingsRegistry = nullptr;
        EntityModel::LoadingContext                                             m_loadingContext;
        EntityModel::InitializationContext                                      m_initializationContext;
        TVector<EntityWorldSystem*>                                             m_worldSystems;
        EntityWorldType                                                         m_worldType = EntityWorldType::Game;
        bool                                                                    m_initialized = false;
        bool                                                                    m_isSuspended = false;
        Render::Viewport                                                        m_viewport = Render::Viewport( Int2::Zero, Int2( 640, 480 ), Math::ViewVolume( Float2( 640, 480 ), FloatRange( 0.1f, 100.0f ) ) );

        // Maps
        TInlineVector<EntityModel::EntityMap*, 3>                               m_maps;

        // Entities
        TVector<Entity*>                                                        m_entityUpdateList;
        TVector<EntityWorldSystem*>                                             m_systemUpdateLists[(int8_t) UpdateStage::NumStages];

        // Time Scaling + Pause
        float                                                                   m_timeScale = 1.0f; // <= 0 means that the world is paused
        Seconds                                                                 m_timeStepLength = 1.0f / 30.0f;
        bool                                                                    m_timeStepRequested = false;

        #if EE_DEVELOPMENT_TOOLS
        EntityModel::EntityComponentTypeMap                                     m_componentTypeLookup;
        Drawing::DrawingSystem                                                  m_debugDrawingSystem;
        String                                                                  m_debugName;
        #endif
    };
}