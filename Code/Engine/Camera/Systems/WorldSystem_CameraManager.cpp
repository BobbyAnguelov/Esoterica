#include "WorldSystem_CameraManager.h"
#include "EntitySystem_DebugCameraController.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"

//-------------------------------------------------------------------------

namespace EE
{
    void CameraManager::ShutdownSystem()
    {
        EE_ASSERT( m_cameras.empty() );

        #if EE_DEVELOPMENT_TOOLS
        m_pDebugCamera = nullptr;
        m_debugCameraSpawned = false;
        #endif
    }

    void CameraManager::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            m_cameras.emplace_back( pCameraComponent );
            m_registeredCamerasStateChanged = true;

            // Handle Debug Cameras
            if ( auto pDebugCameraComponent = TryCast<DebugCameraComponent>( pComponent ) )
            {
                #if EE_DEVELOPMENT_TOOLS
                if ( m_pDebugCamera == nullptr )
                {
                    m_pDebugCamera = pDebugCameraComponent;
                }
                else
                {
                    // TODO: we dont support multiple debug cameras atm
                }
                #endif
            }
        }
    }

    void CameraManager::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            if ( m_pActiveCamera == pCameraComponent )
            {
                m_pActiveCamera = nullptr;
            }

            #if EE_DEVELOPMENT_TOOLS
            if ( m_pDebugCamera == pCameraComponent )
            {
                m_pDebugCamera = nullptr;
            }
            #endif

            m_cameras.erase_first( pCameraComponent );
            m_registeredCamerasStateChanged = true;
        }
    }

    void CameraManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        #if EE_DEVELOPMENT_TOOLS
        TrySpawnDebugCamera( ctx );
        #endif

        //-------------------------------------------------------------------------

        if ( m_registeredCamerasStateChanged )
        {
            // Always use the debug camera in tools
            #if EE_DEVELOPMENT_TOOLS
            if ( !ctx.IsGameWorld() )
            {
                m_pActiveCamera = m_pDebugCamera;
            }
            else
            #endif
            {
                // Switch to the first camera in game worlds
                if ( m_pActiveCamera == nullptr && !m_cameras.empty() )
                {
                    m_pActiveCamera = m_cameras.front();
                }
            }

            m_registeredCamerasStateChanged = false;
        }
    }

    void CameraManager::SetActiveCamera( CameraComponent const* pCamera )
    {
        EE_ASSERT( pCamera != nullptr );
        EE_ASSERT( VectorContains( m_cameras, pCamera ) );
        m_pActiveCamera = const_cast<CameraComponent*>( pCamera );
    }

    #if EE_DEVELOPMENT_TOOLS
    bool CameraManager::TrySpawnDebugCamera( EntityWorldUpdateContext const& ctx )
    {
        if ( !ctx.IsGameWorld() )
        {
            return false;
        }

        if ( m_debugCameraSpawned )
        {
            return false;
        }

        auto pDebugCamera = EE::New<DebugCameraComponent>( StringID( "Debug Camera Component" ) );
        pDebugCamera->SetEnabled( false );

        auto pEntity = EE::New<Entity>( StringID( "Debug Camera" ) );
        pEntity->AddComponent( pDebugCamera );
        pEntity->CreateSystem<DebugCameraController>();
        ctx.GetPersistentMap()->AddEntity( pEntity );

        m_debugCameraSpawned = true;
        return m_debugCameraSpawned;
    }

    bool CameraManager::IsDebugCameraEnabled() const
    {
        return m_pDebugCamera != nullptr && m_pDebugCamera->IsEnabled();
    }
    #endif
}