#pragma once
#include "Game/_Module/API.h"
#include "Engine/Input/GameInput.h"
#include "Engine/Input/GameInputUpdaters.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class EE_GAME_API GameInputMap : public Input::GameInputMap
    {
        EE_REFLECT_TYPE( GameInputMap );

    public:

        GameInputMap() = default;

    private:

        virtual void RegisterInputs() override;
        virtual void Update( Input::InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta ) override;

    public:

        Input::GameInputAxis        m_move;
        Input::GameInputAxis        m_look;

        Input::GameInputSignal      m_ghostMode;
        Input::GameInputValue       m_ghostModeMoveUp;
        Input::GameInputValue       m_ghostModeMoveDown;
        Input::GameInputSignal      m_ghostModeIncreaseSpeed;
        Input::GameInputSignal      m_ghostModeDecreaseSpeed;

        Input::GameInputSignal      m_sprint;
        Input::GameInputSignal      m_crouch;
        Input::GameInputSignal      m_dash;
        Input::GameInputSignal      m_jump;
        Input::GameInputSignal      m_aim;
        Input::GameInputSignal      m_shoot;
        Input::GameInputSignal      m_reload;
        Input::GameInputSignal      m_interact;
        Input::GameInputSignal      m_meleeAttack;

        //-------------------------------------------------------------------------

        Input::ControllerAxisUpdater       m_moveUpdater;
        Input::ControllerAxisUpdater       m_lookUpdater;

        Input::GameInputSignalUpdater      m_ghostModeUpdater;
        Input::GameInputValueUpdater       m_ghostModeMoveUpUpdater;
        Input::GameInputValueUpdater       m_ghostModeMoveDownUpdater;
        Input::GameInputSignalUpdater      m_ghostModeIncreaseSpeedUpdater;
        Input::GameInputSignalUpdater      m_ghostModeDecreaseSpeedUpdater;

        Input::GameInputSignalUpdater      m_sprintUpdater;
        Input::GameInputSignalUpdater      m_crouchUpdater;
        Input::GameInputSignalUpdater      m_dashUpdater;
        Input::GameInputSignalUpdater      m_jumpUpdater;
        Input::GameInputSignalUpdater      m_aimUpdater;
        Input::GameInputSignalUpdater      m_shootUpdater;
        Input::GameInputSignalUpdater      m_reloadUpdater;
        Input::GameInputSignalUpdater      m_interactUpdater;
        Input::GameInputSignalUpdater      m_meleeAttackUpdater;
    };
}