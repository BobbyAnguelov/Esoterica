#pragma once
#include "Game/Player/StateMachine/PlayerAction.h"

// HACK
#include "Base/Imgui/ImguiCurveEditor.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerOverlayAction_Weapon final : public OverlayPlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerOverlayAction_Weapon );

        PlayerOverlayAction_Weapon();

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

    private:

        void FireWeapon( PlayerActionContext const& ctx );

        // HACK
    public:

        // Weapon
        //-------------------------------------------------------------------------
        int32_t                     m_ammo = 15;
        bool                        m_isAutomatic = false;

        // Accuracy
        //-------------------------------------------------------------------------

        float                       m_accuracy = 1.0f; // 1 = Fully Accurate, 0 = Fully Inaccurate
        
        FloatCurve                  m_velocityAccuracyCurve;
        float                       m_movingAccuracy = 1.0f;

        // HACK
        #if EE_DEVELOPMENT_TOOLS
        ImGuiX::CurveEditor         m_curveEditor = ImGuiX::CurveEditor( m_velocityAccuracyCurve );
        #endif

        float                       m_shootingAccuracy = 1.0f;
        float                       m_inaccuracyPenaltyPerShot = 0.1f; // percent per shot
        float                       m_shootingInaccuracyRecoveryRate = 0.5f; // percent per second
    };
}