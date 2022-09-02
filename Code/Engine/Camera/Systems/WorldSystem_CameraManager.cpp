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

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // If the debug camera entity was actually added to the world, assert that it was correctly removed
        if ( m_debugCameraWasAdded )
        {
            EE_ASSERT( m_pDebugCameraEntity == nullptr );
            EE_ASSERT( m_pDebugCamera == nullptr );
        }
        else
        {
            m_pDebugCameraEntity = nullptr;
            m_pDebugCamera = nullptr;
        }

        EE_ASSERT( m_pPreviousCamera == nullptr );
        m_useDebugCamera = false;
        #endif
    }

    void CameraManager::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            m_cameras.emplace_back( pCameraComponent );
            m_registeredCamerasStateChanged = true;

            // Handle Debug Cameras
            #if EE_DEVELOPMENT_TOOLS
            if ( auto pDebugCameraComponent = TryCast<DebugCameraComponent>( pComponent ) )
            {
                // TODO: we dont support multiple debug cameras ATM
                EE_ASSERT( m_pDebugCamera == pDebugCameraComponent );
                m_debugCameraWasAdded = true;
            }
            #endif
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
                DisableDebugCamera();
                m_pDebugCamera = nullptr;
                m_pDebugCameraEntity = nullptr;
                m_debugCameraWasAdded = false;
            }

            if ( m_pPreviousCamera == pCameraComponent )
            {
                m_pPreviousCamera = nullptr;
            }
            #endif

            m_cameras.erase_first( pCameraComponent );
            m_registeredCamerasStateChanged = true;
        }
    }

    void CameraManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        // Maintain valid active camera ptr
        //-------------------------------------------------------------------------

        if ( m_registeredCamerasStateChanged && m_pActiveCamera == nullptr )
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( m_useDebugCamera && m_pDebugCamera != m_pActiveCamera )
            {
                if ( m_pDebugCamera != nullptr && m_pDebugCamera->IsInitialized() )
                {
                    m_pActiveCamera = m_pDebugCamera;
                }
            }
            #endif

            //-------------------------------------------------------------------------

            if ( m_pActiveCamera == nullptr )
            {
                for ( auto pCamera : m_cameras )
                {
                    if ( IsOfType<DebugCameraComponent>( pCamera ) )
                    {
                        continue;
                    }

                    m_pActiveCamera = pCamera;
                    break;
                }
            }

            m_registeredCamerasStateChanged = false;
        }

        // Debug camera management
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Add debug camera entity to world if required
        if ( m_pDebugCameraEntity != nullptr && !m_pDebugCameraEntity->IsAddedToMap() )
        {
            ctx.GetPersistentMap()->AddEntity( m_pDebugCameraEntity );
        }
        #endif
    }

    void CameraManager::SetActiveCamera( CameraComponent const* pCamera )
    {
        EE_ASSERT( pCamera != nullptr );
        EE_ASSERT( VectorContains( m_cameras, pCamera ) );
        m_pActiveCamera = const_cast<CameraComponent*>( pCamera );
    }

    #if EE_DEVELOPMENT_TOOLS
    EE::DebugCameraComponent* CameraManager::TrySpawnDebugCamera( Vector const& cameraPos, Vector const& cameraViewDir )
    {
        EE_ASSERT( !cameraViewDir.IsNearZero3() );

        if ( m_pDebugCamera == nullptr )
        {
            m_pDebugCamera = EE::New<DebugCameraComponent>( StringID( "Debug Camera Component" ) );
            m_pDebugCamera->SetEnabled( false );
            m_pDebugCamera->SetPositionAndLookAtDirection( cameraPos, cameraViewDir.GetNormalized3() );

            m_pDebugCameraEntity = EE::New<Entity>( StringID( "Debug Camera" ) );
            m_pDebugCameraEntity->AddComponent( m_pDebugCamera );
            m_pDebugCameraEntity->CreateSystem<DebugCameraController>();
        }
        else
        {
            m_pDebugCamera->SetPositionAndLookAtDirection( cameraPos, cameraViewDir.GetNormalized3() );
        }

        return m_pDebugCamera;
    }

    void CameraManager::EnableDebugCamera( Vector const& cameraPos, Vector const& cameraViewDir )
    {
        // If we dont have debug camera set, try to find one
        if ( m_pDebugCamera == nullptr )
        {
            m_pDebugCamera = TrySpawnDebugCamera( cameraPos, cameraViewDir );
        }
        else // Update camera position
        {
            m_pDebugCamera->SetPositionAndLookAtDirection( cameraPos, cameraViewDir.GetNormalized3() );
        }

        // Only switch active camera if the debug camera is initialized
        m_pPreviousCamera = m_pActiveCamera;
        
        if ( m_pDebugCamera->IsInitialized() )
        {
            m_pActiveCamera = m_pDebugCamera;
        }

        m_useDebugCamera = true;
    }

    void CameraManager::DisableDebugCamera()
    {
        m_pActiveCamera = m_pPreviousCamera;
        m_useDebugCamera = false;
    }
    #endif
}