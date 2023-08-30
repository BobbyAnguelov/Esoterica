#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Controller.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class WeaponGraphController final : public Animation::SubGraphController
    {
        enum States
        {
            Holstered = 0,
            Drawing,
            Drawn,
            Aimed,
            Holstering,
        };

    public:

        EE_ANIMATION_SUBGRAPH_CONTROLLER_ID( WeaponGraphController );

    public:

        WeaponGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent );

        void DrawWeapon();
        void HolsterWeapon();

        bool IsWeaponDrawn() const { return m_isWeaponDrawn; }

        void Aim( Vector const& targetWS );
        void Shoot();

        virtual void PostGraphUpdate( Seconds deltaTime ) override;

    private:

        ControlParameter<bool>          m_shootSignal = "Weapon_Fire";
        ControlParameter<StringID>      m_weaponState = "Weapon_State";
        ControlParameter<float>         m_aimAngleHorizontal =  "Weapon_AimAngleHorizontal";
        ControlParameter<float>         m_aimAngleVertical = "Weapon_AimAngleVertical";

        bool                            m_isWeaponDrawn;
    };
}