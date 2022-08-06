#include "EntityWorldManager.h"
#include "EntityWorld.h"
#include "EntityWorldDebugView.h"
#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/Camera/Components/Component_Camera.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "Engine/UpdateContext.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE
{
    EntityWorldManager::~EntityWorldManager()
    {
        EE_ASSERT( m_worlds.empty() && m_worldSystemTypeInfos.empty() );
    }

    void EntityWorldManager::Initialize( SystemRegistry const& systemsRegistry )
    {
        m_pSystemsRegistry = &systemsRegistry;

        //-------------------------------------------------------------------------

        auto pTypeRegistry = systemsRegistry.GetSystem<TypeSystem::TypeRegistry>();
        EE_ASSERT( pTypeRegistry != nullptr );
        m_worldSystemTypeInfos = pTypeRegistry->GetAllDerivedTypes( IEntityWorldSystem::GetStaticTypeID(), false, false, true );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_debugViewTypeInfos = pTypeRegistry->GetAllDerivedTypes( EntityWorldDebugView::GetStaticTypeID(), false, false, true );
        #endif

        // Create a game world
        //-------------------------------------------------------------------------

        CreateWorld( EntityWorldType::Game );
    }

    void EntityWorldManager::Shutdown()
    {
        for ( auto& pWorld : m_worlds )
        {
            #if EE_DEVELOPMENT_TOOLS
            pWorld->ShutdownDebugViews();
            #endif

            pWorld->Shutdown();
            EE::Delete( pWorld );
        }
        m_worlds.clear();

        //-------------------------------------------------------------------------

        m_worldSystemTypeInfos.clear();
        m_pSystemsRegistry = nullptr;
    }

    //-------------------------------------------------------------------------

    void EntityWorldManager::StartFrame()
    {
        for ( auto& pWorld : m_worlds )
        {
            #if EE_DEVELOPMENT_TOOLS
            pWorld->ResetDebugDrawingSystem();
            #endif
        }
    }

    void EntityWorldManager::EndFrame()
    {
        // Do Nothing
    }

    //-------------------------------------------------------------------------

    EntityWorld* EntityWorldManager::GetGameWorld()
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsGameWorld() )
            {
                return pWorld;
            }
        }

        return nullptr;
    }

    EntityWorld* EntityWorldManager::CreateWorld( EntityWorldType worldType )
    {
        EE_ASSERT( m_pSystemsRegistry != nullptr );

        //-------------------------------------------------------------------------

        // Only a single game world is allowed
        if ( worldType == EntityWorldType::Game )
        {
            if ( GetGameWorld() != nullptr )
            {
                EE_UNREACHABLE_CODE();
                return nullptr;
            }
        }

        //-------------------------------------------------------------------------

        auto pNewWorld = EE::New<EntityWorld>( worldType );
        pNewWorld->Initialize( *m_pSystemsRegistry, m_worldSystemTypeInfos );
        m_worlds.emplace_back( pNewWorld );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        pNewWorld->InitializeDebugViews( *m_pSystemsRegistry, m_debugViewTypeInfos );
        #endif

        return pNewWorld;
    }

    void EntityWorldManager::DestroyWorld( EntityWorld* pWorld )
    {
        EE_ASSERT( Threading::IsMainThread() );

        // Remove world from worlds list
        auto foundWorldIter = eastl::find( m_worlds.begin(), m_worlds.end(), pWorld );
        EE_ASSERT( foundWorldIter != m_worlds.end() );
        m_worlds.erase( foundWorldIter );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        pWorld->ShutdownDebugViews();
        #endif

        // Shutdown and destroy world
        pWorld->Shutdown();
        EE::Delete( pWorld );
    }

    //-------------------------------------------------------------------------

    bool EntityWorldManager::IsBusyLoading() const
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsBusyLoading() )
            {
                return true;
            }
        }

        return false;
    }

    void EntityWorldManager::UpdateLoading()
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->UpdateLoading();
        }
    }

    void EntityWorldManager::UpdateWorlds( UpdateContext const& context )
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsSuspended() )
            {
                continue;
            }

            // Reflect input state
            //-------------------------------------------------------------------------

            if ( context.GetUpdateStage() == UpdateStage::FrameStart )
            {
                auto pPlayerManager = pWorld->GetWorldSystem<PlayerManager>();
                auto pWorldInputState = pWorld->GetInputState();

                if ( pPlayerManager->IsPlayerEnabled() )
                {
                    auto pInputSystem = context.GetSystem<Input::InputSystem>();
                    pInputSystem->ReflectState( context.GetDeltaTime(), pWorld->GetTimeScale(), *pWorldInputState );
                }
                else
                {
                    pWorldInputState->Clear();
                }
            }

            // Run world updates
            //-------------------------------------------------------------------------

            pWorld->Update( context );

            // Update world view
            //-------------------------------------------------------------------------
            // We explicitly reflect the camera at the end of the post-physics stage as we assume it has been updated at that point

            if ( context.GetUpdateStage() == UpdateStage::PostPhysics && pWorld->GetViewport() != nullptr )
            {
                auto pViewport = pWorld->GetViewport();
                auto pCameraManager = pWorld->GetWorldSystem<CameraManager>();
                if ( pCameraManager->HasActiveCamera() )
                {
                    auto pActiveCamera = pCameraManager->GetActiveCamera();

                    // Update camera view dimensions if needed
                    if ( pViewport->GetDimensions() != pActiveCamera->GetViewVolume().GetViewDimensions() )
                    {
                        pActiveCamera->UpdateViewDimensions( pViewport->GetDimensions() );
                    }

                    // Update world viewport
                    pViewport->SetViewVolume( pActiveCamera->GetViewVolume() );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void EntityWorldManager::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload )
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->BeginHotReload( usersToReload );
        }
    }

    void EntityWorldManager::EndHotReload()
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->EndHotReload();
        }
    }
    #endif
}