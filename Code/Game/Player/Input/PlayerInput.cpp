#include "PlayerInput.h"
#include "Base/Input/InputSystem.h"
#include "Base/Input/InputDevices/InputDevice_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    void GameInputMap::RegisterInputs()
    {
        RegisterInput( m_move );
        RegisterInput( m_look );

        RegisterInput( m_ghostMode );
        RegisterInput( m_sprint );
        RegisterInput( m_crouch );
        RegisterInput( m_dash );
        RegisterInput( m_jump );
        RegisterInput( m_aim );
        RegisterInput( m_shoot );
        RegisterInput( m_reload );
        RegisterInput( m_interact );
        RegisterInput( m_meleeAttack );

        m_moveUpdater.m_useLeftStick = true;
        m_lookUpdater.m_useLeftStick = false;

        m_ghostModeUpdater;
        m_ghostModeMoveUpUpdater.m_ID = Input::InputID::Controller_LeftTrigger;
        m_ghostModeMoveDownUpdater.m_ID = Input::InputID::Controller_RightTrigger;
        m_ghostModeIncreaseSpeedUpdater.m_ID = Input::InputID::Controller_RightShoulder;
        m_ghostModeDecreaseSpeedUpdater.m_ID = Input::InputID::Controller_LeftShoulder;

        m_sprintUpdater.m_ID = Input::InputID::Controller_LeftStick;
        m_crouchUpdater.m_ID = Input::InputID::Controller_RightStick;
        m_dashUpdater.m_ID = Input::InputID::Controller_FaceButtonRight;
        m_jumpUpdater.m_ID = Input::InputID::Controller_FaceButtonDown;
        m_aimUpdater.m_ID = Input::InputID::Controller_LeftTrigger;
        m_shootUpdater.m_ID = Input::InputID::Controller_RightTrigger;
        m_reloadUpdater.m_ID = Input::InputID::Controller_FaceButtonLeft;
        m_interactUpdater.m_ID = Input::InputID::Controller_FaceButtonUp;
        m_meleeAttackUpdater.m_ID = Input::InputID::Controller_RightShoulder;
    }

    void GameInputMap::Update( Input::InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta )
    {
        Input::ControllerDevice const* pController = pInputSystem->GetFirstConnectedController();
        if ( pController != nullptr )
        {
            m_moveUpdater.m_deadZone.m_inner = pController->GetDefaultLeftStickDeadZones().m_x;
            m_moveUpdater.m_deadZone.m_outer = pController->GetDefaultLeftStickDeadZones().m_y;

            m_lookUpdater.m_deadZone.m_inner = pController->GetDefaultRightStickDeadZones().m_x;
            m_lookUpdater.m_deadZone.m_outer = pController->GetDefaultRightStickDeadZones().m_y;
        }

        //-------------------------------------------------------------------------

        auto UpdateSignal = [&] ( Input::GameInputSignal& signal, Input::GameInputSignalUpdater& updater )
        {
            updater.Update( pInputSystem, timeDelta, scaledTimeDelta );
            signal.m_state = updater.m_state;

            if ( signal.IsHeld() )
            {
                signal.m_holdTime += timeDelta;
            }
            else if ( signal.m_state == Input::InputState::None )
            {
                signal.m_holdTime = 0.0f;
            }
        };

        auto UpdateValue = [&] ( Input::GameInputValue& value, Input::GameInputValueUpdater& updater )
        {
            updater.Update( pInputSystem, timeDelta, scaledTimeDelta );
            value.m_value = updater.m_value;
        };

        auto UpdateAxis = [&] ( Input::GameInputAxis& axis, Input::GameInputAxisUpdater& updater )
        {
            updater.Update( pInputSystem, timeDelta, scaledTimeDelta );
            axis.m_value = updater.m_value;
        };

        //-------------------------------------------------------------------------

        UpdateAxis( m_move, m_moveUpdater );
        UpdateAxis( m_look, m_lookUpdater );

        UpdateSignal( m_ghostMode, m_ghostModeUpdater );
        UpdateValue( m_ghostModeMoveUp, m_ghostModeMoveUpUpdater );
        UpdateValue( m_ghostModeMoveDown, m_ghostModeMoveDownUpdater );
        UpdateSignal( m_ghostModeIncreaseSpeed, m_ghostModeIncreaseSpeedUpdater );
        UpdateSignal( m_ghostModeDecreaseSpeed, m_ghostModeDecreaseSpeedUpdater );

        UpdateSignal( m_sprint, m_sprintUpdater );
        UpdateSignal( m_crouch, m_crouchUpdater );
        UpdateSignal( m_dash, m_dashUpdater );
        UpdateSignal( m_jump, m_jumpUpdater );
        UpdateSignal( m_aim, m_aimUpdater );
        UpdateSignal( m_shoot, m_shootUpdater );
        UpdateSignal( m_reload, m_reloadUpdater );
        UpdateSignal( m_interact, m_interactUpdater );
        UpdateSignal( m_meleeAttack, m_meleeAttackUpdater );
    }
}