#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/ResourceRequesterID.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class EntityWorld;
    class SystemRegistry;
    enum class EntityWorldType : uint8_t;
    namespace TypeSystem { class TypeInfo; }
    namespace Render { class Viewport; }

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldManager : public ISystem
    {
        friend class EntityDebugView;

    public:

        EE_SYSTEM_ID( EntityWorldManager );

    public:

        ~EntityWorldManager();

        void Initialize( SystemRegistry const& systemsRegistry );
        void Shutdown();

        //-------------------------------------------------------------------------

        // Called at the start of the frame
        void StartFrame();

        // Called at the end of the frame, just before rendering
        void EndFrame();

        // Loading
        //-------------------------------------------------------------------------

        bool IsBusyLoading() const;
        void UpdateLoading();

        // Worlds
        //-------------------------------------------------------------------------

        // Returns the currently active game world, there should only be one at any given time
        EntityWorld* GetGameWorld();
        
        // Returns the currently active game world, there should only be one at any given time
        EntityWorld const* GetGameWorld() const { return const_cast<EntityWorldManager*>( this )->GetGameWorld(); }

        // Create a new entity world - note only a single game world is allowed at any given time
        EntityWorld* CreateWorld( EntityWorldType worldType );

        // Destroy an existing world
        void DestroyWorld( EntityWorld* pWorld );

        // Get all created worlds
        TInlineVector<EntityWorld*, 5> const& GetWorlds() const { return m_worlds; }

        // Run the world update - updates all entities, systems and camera
        void UpdateWorlds( UpdateContext const& context );

        // Editor
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Enable or disable the player for the specified world
        void SetPlayerEnabled( EntityWorld* pWorld, bool isPlayerEnabled );
        #endif

        // Hot Reload
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload );
        void EndHotReload();
        #endif

    private:

        SystemRegistry const*                               m_pSystemsRegistry = nullptr;
        TInlineVector<EntityWorld*, 5>                      m_worlds;
        TVector<TypeSystem::TypeInfo const*>                m_worldSystemTypeInfos;

        #if EE_DEVELOPMENT_TOOLS
        TVector<TypeSystem::TypeInfo const*>                m_debugViewTypeInfos;
        #endif
    };
}