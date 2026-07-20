#include "WorldSystem_Camera.h"
#include "Engine/Camera/Components/Component_ToolsCamera.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"

//-------------------------------------------------------------------------

namespace EE
{
    void CameraSystem::ShutdownSystem()
    {
        EE_ASSERT( m_cameras.empty() );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // If the debug camera entity was actually added to the world, assert that it was correctly removed
        if ( m_debugCameraWasAdded )
        {
            EE_ASSERT( m_pToolsCameraEntity == nullptr );
            EE_ASSERT( m_pToolsCamera == nullptr );
        }
        else
        {
            m_pToolsCameraEntity = nullptr;
            m_pToolsCamera = nullptr;
        }

        EE_ASSERT( m_pPreviousCamera == nullptr );
        #endif
    }

    void CameraSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            m_cameras.emplace_back( pCameraComponent );
            m_registeredCamerasStateChanged = true;
            m_newlyAddedCamerasStartIdx = ( m_newlyAddedCamerasStartIdx == InvalidIndex ) ? (int32_t) m_cameras.size() - 1 : m_newlyAddedCamerasStartIdx;
        }
    }

    void CameraSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            if ( m_pActiveCamera == pCameraComponent )
            {
                m_pActiveCamera = nullptr;
            }

            #if EE_DEVELOPMENT_TOOLS
            if ( m_pToolsCamera == pCameraComponent )
            {
                DisableToolsCamera();
                m_pToolsCamera = nullptr;
                m_pToolsCameraEntity = nullptr;
                m_debugCameraWasAdded = false;
            }

            if ( m_pPreviousCamera == pCameraComponent )
            {
                m_pPreviousCamera = nullptr;
            }
            #endif

            // Remove camera record
            m_cameras.erase_first( pCameraComponent );
            m_registeredCamerasStateChanged = true;

            // Check if we still have non-tools cameras added
            m_hasNonToolsCamera = false;
            for ( auto pCamera : m_cameras )
            {
                if ( !IsOfType<ToolsCameraComponent>( pCamera ) )
                {
                    m_hasNonToolsCamera = true;
                    break;
                }
            }
        }
    }

    void CameraSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        // Maintain valid active camera ptr
        //-------------------------------------------------------------------------

        if ( m_registeredCamerasStateChanged )
        {
            if ( m_pActiveCamera == nullptr || !m_hasNonToolsCamera )
            {
                // Try to set the active camera to the first non-debug camera
                if ( ctx.IsGameWorld() && m_newlyAddedCamerasStartIdx != InvalidIndex )
                {
                    int32_t const numCameras = (int32_t) m_cameras.size();
                    int32_t i = m_newlyAddedCamerasStartIdx;
                    while ( i < numCameras && !m_hasNonToolsCamera )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        if ( m_cameras[i] == m_pToolsCamera )
                        {
                            i++;
                            continue;
                        }
                        #endif

                        m_pActiveCamera = m_cameras[i];
                        m_hasNonToolsCamera = true;
                        i++;
                    }
                }

                // If we still couldn't set a camera, use the first one (likely the debug one)
                if ( m_pActiveCamera == nullptr )
                {
                    if ( !m_cameras.empty() )
                    {
                        m_pActiveCamera = m_cameras[0];
                    }
                }
            }

            m_registeredCamerasStateChanged = false;
            m_newlyAddedCamerasStartIdx = InvalidIndex;
        }

        // Debug camera management
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Add debug camera entity to world if required
        if ( m_pToolsCameraEntity != nullptr && !m_pToolsCameraEntity->IsAddedToMap() )
        {
            ctx.GetPersistentMap()->AddEntity( m_pToolsCameraEntity );
        }
        #endif
    }

    void CameraSystem::SetActiveCamera( CameraComponent const* pCamera )
    {
        EE_ASSERT( pCamera != nullptr );
        EE_ASSERT( VectorContains( m_cameras, pCamera ) );
        m_pActiveCamera = const_cast<CameraComponent*>( pCamera );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    ToolsCameraComponent* CameraSystem::TrySpawnToolsCamera( Vector const& cameraPos, Vector const& cameraViewDir )
    {
        EE_ASSERT( !cameraViewDir.IsNearZero3() );

        if ( m_pToolsCamera == nullptr )
        {
            m_pToolsCamera = EE::New<ToolsCameraComponent>( StringID( "Debug Camera Component" ) );
            m_pToolsCamera->SetPositionAndLookAtDirection( cameraPos, cameraViewDir.GetNormalized3() );

            m_pToolsCameraEntity = EE::New<Entity>( StringID( "Debug Camera" ) );
            m_pToolsCameraEntity->AddComponent( m_pToolsCamera );
        }
        else
        {
            m_pToolsCamera->SetPositionAndLookAtDirection(cameraPos, cameraViewDir.GetNormalized3());
        }

        return m_pToolsCamera;
    }

    bool CameraSystem::IsToolsCameraEnabled() const
    {
        return m_pToolsCamera == m_pActiveCamera;
    }

    void CameraSystem::UpdateToolsCamera( EntityWorldUpdateContext const& ctx )
    {
        if( m_pToolsCamera != nullptr )
        {
            m_pToolsCamera->Update( ctx );
        }
    }

    void CameraSystem::EnableToolsCamera()
    {
        Vector cameraPos = Vector::Zero;
        Vector cameraViewDir = Vector::WorldForward;

        // Try maintain current view
        if ( m_pActiveCamera != nullptr )
        {
            cameraPos = m_pActiveCamera->GetViewPosition();
            cameraViewDir = m_pActiveCamera->GetViewDirection();
        }

        // If we dont have debug camera set, try to find one
        if ( m_pToolsCamera == nullptr )
        {
            m_pToolsCamera = TrySpawnToolsCamera( cameraPos, cameraViewDir );
        }
        else // Update camera position
        {
            m_pToolsCamera->SetPositionAndLookAtDirection( cameraPos, cameraViewDir.GetNormalized3() );
        }

        // Only switch active camera if the debug camera is initialized
        m_pPreviousCamera = m_pActiveCamera;

        if ( m_pToolsCamera->IsInitialized() )
        {
            m_pActiveCamera = m_pToolsCamera;
        }
    }

    void CameraSystem::DisableToolsCamera()
    {
        m_pActiveCamera = m_pPreviousCamera;
    }
    #endif
}