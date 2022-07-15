#include "WorldSystem_PlayerManager.h"
#include "Engine/Player/Components/Component_PlayerSpawn.h"
#include "Engine/Player/Components/Component_Player.h"
#include "Engine/Camera/Components/Component_FreeLookCamera.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityMap.h"
#include "System/Input/InputSystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PlayerManager::ShutdownSystem()
    {
        EE_ASSERT( !m_player.m_entityID.IsValid() );
        EE_ASSERT( m_spawnPoints.empty() );

        #if EE_DEVELOPMENT_TOOLS
        m_pDebugCameraComponent = nullptr;
        #endif
    }

    void PlayerManager::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<Player::PlayerSpawnComponent>( pComponent ) )
        {
            m_spawnPoints.emplace_back( pSpawnComponent );
        }
        else if ( auto pPlayerComponent = TryCast<Player::PlayerComponent>( pComponent ) )
        {
            if ( m_player.m_entityID.IsValid() )
            {
                EE_LOG_ERROR( "Player", "Multiple players spawned! this is not supported" );
            }
            else
            {
                m_player.m_entityID = pEntity->GetID();
                EE_ASSERT( m_player.m_pPlayerComponent == nullptr && m_player.m_pCameraComponent == nullptr );
                m_player.m_pPlayerComponent = pPlayerComponent;

                // Check if the camera for this player has already been registered
                for ( auto pCamera : m_cameras )
                {
                    if ( pCamera->GetEntityID() == m_player.m_entityID )
                    {
                        m_player.m_pCameraComponent = pCamera;
                    }
                }

                m_registeredPlayerStateChanged = true;
            }
        }
        else if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            m_cameras.emplace_back( pCameraComponent );

            // If this is the camera for the player set it in the player struct
            if ( m_player.m_entityID == pEntity->GetID() )
            {
                EE_ASSERT( m_player.m_pCameraComponent == nullptr );
                m_player.m_pCameraComponent = pCameraComponent;
                m_registeredPlayerStateChanged = true;
            }
        }
    }

    void PlayerManager::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpawnComponent = TryCast<Player::PlayerSpawnComponent>( pComponent ) )
        {
            m_spawnPoints.erase_first_unsorted( pSpawnComponent );
        }
        else if ( auto pPlayerComponent = TryCast<Player::PlayerComponent>( pComponent ) )
        {
            // Remove the player
            if ( m_player.m_entityID == pEntity->GetID() )
            {
                EE_ASSERT( m_player.m_pPlayerComponent == pPlayerComponent );
                m_player.m_entityID.Clear();
                m_player.m_pPlayerComponent = nullptr;
                m_player.m_pCameraComponent = nullptr;
                m_registeredPlayerStateChanged = true;
            }
        }
        else if ( auto pCameraComponent = TryCast<CameraComponent>( pComponent ) )
        {
            if ( m_pActiveCamera == pCameraComponent )
            {
                m_pActiveCamera = nullptr;
            }

            m_cameras.erase_first( pCameraComponent );

            // Remove camera from the player state
            if ( m_player.IsValid() && m_player.m_entityID == pEntity->GetID() )
            {
                EE_ASSERT( m_player.m_pCameraComponent == pCameraComponent );
                m_player.m_pCameraComponent = nullptr;
                m_registeredPlayerStateChanged = true;
            }
        }
    }

    //-------------------------------------------------------------------------

    bool PlayerManager::TrySpawnPlayer( EntityWorldUpdateContext const& ctx )
    {
        if ( m_spawnPoints.empty() )
        {
            return false;
        }

        auto pTypeRegistry = ctx.GetSystem<TypeSystem::TypeRegistry>();
        auto pTaskSystem = ctx.GetSystem<TaskSystem>();
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        // For now we only support a single spawn point
        pPersistentMap->AddEntityCollection( pTaskSystem, *pTypeRegistry, *m_spawnPoints[0]->GetEntityCollectionDesc(), m_spawnPoints[0]->GetWorldTransform() );
        return true;
    }

    bool PlayerManager::IsPlayerEnabled() const
    {
        if ( m_player.m_pPlayerComponent == nullptr )
        {
            return false;
        }

        return m_player.m_pPlayerComponent->IsPlayerEnabled();
    }

    void PlayerManager::SetPlayerControllerState( bool isEnabled )
    {
        m_isControllerEnabled = isEnabled;

        if ( m_player.m_pPlayerComponent != nullptr )
        {
            m_player.m_pPlayerComponent->SetPlayerEnabled( m_isControllerEnabled );
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PlayerManager::SpawnDebugCamera( EntityWorldUpdateContext const& ctx )
    {
        EE_ASSERT( m_pDebugCameraComponent == nullptr );
        m_pDebugCameraComponent = EE::New<FreeLookCameraComponent>( StringID( "Debug Camera Component" ) );
        m_pDebugCameraComponent->SetPositionAndLookatTarget( Vector( 0, -5, 5 ), Vector( 0, 0, 0 ) );

        auto pEntity = EE::New<Entity>( StringID( "Debug Camera" ) );
        pEntity->AddComponent( m_pDebugCameraComponent );
        ctx.GetPersistentMap()->AddEntity( pEntity );
    }

    void PlayerManager::UpdateDebugCamera( EntityWorldUpdateContext const& ctx )
    {
        EE_ASSERT( m_pDebugCameraComponent != nullptr );
        EE_ASSERT( m_isControllerEnabled );

        //-------------------------------------------------------------------------

        Seconds const deltaTime = ctx.GetRawDeltaTime();
        auto pInputSystem = ctx.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );

        auto const pMouseState = pInputSystem->GetMouseState();
        auto const pKeyboardState = pInputSystem->GetKeyboardState();

        // Speed Update
        //-------------------------------------------------------------------------

        if ( pKeyboardState->IsAltHeldDown() )
        {
            if ( pMouseState->WasReleased( Input::MouseButton::Middle ) )
            {
                m_debugCameraMoveSpeed = s_debugCameraDefaultSpeed;
            }
            else
            {
                int32_t const wheelDelta = pMouseState->GetWheelDelta();
                m_debugCameraMoveSpeed = FloatRange( s_debugCameraMinSpeed, s_debugCameraMaxSpeed ).GetClampedValue( m_debugCameraMoveSpeed + ( wheelDelta * 0.5f ) );
            }
        }

        // Position update
        //-------------------------------------------------------------------------

        Vector positionDelta = Vector::Zero;

        bool const fwdButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_W );
        bool const backButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_S );
        bool const leftButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_A );
        bool const rightButton = pKeyboardState->IsHeldDown( Input::KeyboardButton::Key_D );
        bool const needsPositionUpdate = fwdButton || backButton || leftButton || rightButton;

        if ( needsPositionUpdate )
        {
            float LR = 0;
            if ( leftButton ) { LR -= 1.0f; }
            if ( rightButton ) { LR += 1.0f; }

            float FB = 0;
            if ( fwdButton ) { FB += 1.0f; }
            if ( backButton ) { FB -= 1.0f; }

            Vector const spatialInput( LR, FB, 0, 0 );

            Transform cameraTransform = m_pDebugCameraComponent->GetWorldTransform();
            Vector const forwardDirection = cameraTransform.GetForwardVector();
            Vector const rightDirection = cameraTransform.GetRightVector();

            Vector const moveDelta = Vector( m_debugCameraMoveSpeed * deltaTime );
            Vector const deltaForward = spatialInput.GetSplatY() * moveDelta;
            Vector const deltaRight = spatialInput.GetSplatX() * moveDelta;

            // Calculate position delta
            positionDelta = deltaRight * rightDirection;
            positionDelta += deltaForward * forwardDirection;
            positionDelta.m_w = 0.0f;

            // Update camera transform
            cameraTransform.AddTranslation( positionDelta );
            m_pDebugCameraComponent->SetWorldTransform( cameraTransform );
        }

        // Direction update
        //-------------------------------------------------------------------------

        constexpr static float const g_mouseSensitivity = Math::DegreesToRadians * 0.25f; // Convert pixels to radians
        constexpr static float const g_debugCameraMaxAngularVelocity = Math::Pi * 5.0f;

        // Update accumulator
        if ( pMouseState->IsHeldDown( Input::MouseButton::Right ) )
        {
            Vector const mouseDelta = pMouseState->GetMovementDelta();
            Vector const directionDelta = mouseDelta.GetNegated();
            m_directionChangeAccumulator += ( directionDelta * ( Math::DegreesToRadians * 0.25f ) );
        }

        // Update view direction
        if ( !m_directionChangeAccumulator.IsNearZero2() )
        {
            float const maxAdjustmentPerFrame = g_debugCameraMaxAngularVelocity * deltaTime;

            Radians yawDelta = 0.0f;
            if ( m_directionChangeAccumulator.m_x < 0.0f )
            {
                yawDelta = Math::Max( -maxAdjustmentPerFrame, m_directionChangeAccumulator.m_x );
                m_directionChangeAccumulator.m_x = Math::Min( m_directionChangeAccumulator.m_x - yawDelta.ToFloat(), 0.0f);
            }
            else
            {
                yawDelta = Math::Min( maxAdjustmentPerFrame, m_directionChangeAccumulator.m_x );
                m_directionChangeAccumulator.m_x = Math::Max( m_directionChangeAccumulator.m_x - yawDelta.ToFloat(), 0.0f );
            }

            Radians pitchDelta = 0.0f;
            if ( m_directionChangeAccumulator.m_y < 0.0f )
            {
                pitchDelta = Math::Max( -maxAdjustmentPerFrame, m_directionChangeAccumulator.m_y );
                m_directionChangeAccumulator.m_y = Math::Min( m_directionChangeAccumulator.m_y - pitchDelta.ToFloat(), 0.0f );
            }
            else
            {
                pitchDelta = Math::Min( maxAdjustmentPerFrame, m_directionChangeAccumulator.m_y );
                m_directionChangeAccumulator.m_y = Math::Max( m_directionChangeAccumulator.m_y - pitchDelta.ToFloat(), 0.0f );
            }

            //-------------------------------------------------------------------------

            m_pDebugCameraComponent->AdjustPitchAndYaw( yawDelta, pitchDelta );
        }
    }

    void PlayerManager::SetDebugCameraView( Transform const& cameraTransform )
    {
        m_pDebugCameraComponent->SetWorldTransform( cameraTransform );
    }

    void PlayerManager::SetDebugMode( DebugMode newMode )
    {
        switch ( newMode )
        {
            case DebugMode::None:
            {
                if ( m_player.IsValid() )
                {
                    m_pActiveCamera = m_player.m_pCameraComponent;
                    m_player.m_pPlayerComponent->SetPlayerEnabled( true );
                }
            }
            break;

            case DebugMode::PlayerWithDebugCamera:
            {
                m_pActiveCamera = m_pDebugCameraComponent;

                if ( m_player.IsValid() )
                {
                    // Transform camera position/orientation to the debug camera
                    if ( m_debugMode == DebugMode::None )
                    {
                        m_pDebugCameraComponent->SetWorldTransform( m_player.m_pCameraComponent->GetWorldTransform() );
                    }

                    m_player.m_pPlayerComponent->SetPlayerEnabled( true );
                }
            }
            break;

            case DebugMode::OnlyDebugCamera:
            {
                m_pActiveCamera = m_pDebugCameraComponent;

                if ( m_player.IsValid() )
                {
                    // Transform camera position/orientation to the debug camera
                    if ( m_debugMode == DebugMode::None )
                    {
                        m_pDebugCameraComponent->SetWorldTransform( m_player.m_pCameraComponent->GetWorldTransform() );
                    }

                    m_player.m_pPlayerComponent->SetPlayerEnabled( false );
                }
            }
            break;
        }

        m_debugMode = newMode;
    }
    #endif

    //-------------------------------------------------------------------------

    void PlayerManager::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( ctx.GetUpdateStage() == UpdateStage::FrameStart )
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( m_pDebugCameraComponent == nullptr )
            {
                SpawnDebugCamera( ctx );
            }

            // Always ensure we have an active camera set
            if ( m_pActiveCamera == nullptr && m_pDebugCameraComponent->IsInitialized() )
            {
                m_pActiveCamera = m_pDebugCameraComponent;
            }

            // If we are in full debug mode, update the camera position
            if ( m_debugMode == DebugMode::OnlyDebugCamera && m_isControllerEnabled )
            {
                UpdateDebugCamera( ctx );
            }
            #endif

            //-------------------------------------------------------------------------

            // Spawn players in game worlds
            if ( ctx.IsGameWorld() && !m_hasSpawnedPlayer )
            {
                m_hasSpawnedPlayer = TrySpawnPlayer( ctx );
            }

            // Handle players state changes
            if ( m_registeredPlayerStateChanged )
            {
                // Only automatically switch to player in game worlds
                if ( ctx.IsGameWorld() && m_player.IsValid() )
                {
                    m_pActiveCamera = m_player.m_pCameraComponent;
                    m_player.m_pPlayerComponent->SetPlayerEnabled( m_isControllerEnabled );
                }

                m_registeredPlayerStateChanged = false;
            }
        }
        else if ( ctx.GetUpdateStage() == UpdateStage::Paused )
        {
            #if EE_DEVELOPMENT_TOOLS
            // If we are in full debug mode, update the camera position
            if ( m_debugMode == DebugMode::OnlyDebugCamera && m_isControllerEnabled )
            {
                UpdateDebugCamera( ctx );
            }
            #endif
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }
}