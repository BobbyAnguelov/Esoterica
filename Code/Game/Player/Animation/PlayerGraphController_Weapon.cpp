#include "PlayerGraphController_Weapon.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    WeaponGraphController::WeaponGraphController( Animation::GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
        : Animation::SubGraphController( pGraphInstance, pMeshComponent )
    {
        m_shootSignal.TryBind( this );
        m_weaponState.TryBind( this );
        m_aimAngleHorizontal.TryBind( this );
        m_aimAngleVertical.TryBind( this );
    }

    //-------------------------------------------------------------------------

    void WeaponGraphController::Shoot()
    {
        m_shootSignal.Set( true );
    }

    void WeaponGraphController::DrawWeapon()
    {
        m_weaponState.Set( StringID( "Draw" ) );
    }

    void WeaponGraphController::HolsterWeapon()
    {
        m_weaponState.Set( StringID( "Holster" ) );
    }

    void WeaponGraphController::Aim( Vector const& targetWS )
    {
        float angleH = 0.0f;
        float angleV = 0.0f;
        m_weaponState.Set( StringID( "Aim" ) );

        // This is stupid but it's a demo
        Animation::Pose const* pPose = GetCurrentPose();
        int32_t const headIdx = pPose->GetSkeleton()->GetBoneIndex( StringID( "head" ) );
        if ( headIdx != InvalidIndex )
        {
            Vector headPos = pPose->GetGlobalTransform( headIdx ).GetTranslation();
            Vector targetPosCS = ConvertWorldSpacePointToCharacterSpace( targetWS );
            Vector aimDir = ( targetPosCS - headPos ).GetNormalized3();

            angleH = Math::GetYawAngleBetweenNormalizedVectors( Vector::WorldForward, aimDir ).ToDegrees().ToFloat();
            angleV = Math::GetPitchAngleBetweenNormalizedVectors( Vector::WorldForward, aimDir ).ToDegrees().ToFloat();
        }

        m_aimAngleHorizontal.Set( angleH );
        m_aimAngleVertical.Set( angleV );
    }

    //-------------------------------------------------------------------------

    void WeaponGraphController::PostGraphUpdate( Seconds deltaTime )
    {
        static StringID const stateEventIDs[] =
        {
            StringID( "Weapon_Holstered" ),
            StringID( "Weapon_Drawing" ),
            StringID( "Weapon_Drawn" ),
            StringID( "Weapon_Aimed" ),
            StringID( "Weapon_Holstering" ),
        };

        //-------------------------------------------------------------------------

        m_shootSignal.Set( false );
        m_isWeaponDrawn = false;

        //-------------------------------------------------------------------------

        for ( auto sampledEvent : GetSampledEvents() )
        {
            if ( sampledEvent.IsIgnored() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( sampledEvent.IsStateEvent() )
            {
                StringID const stateID = sampledEvent.GetStateEventID();
                if ( stateID == stateEventIDs[States::Drawn] || stateID == stateEventIDs[States::Aimed] )
                {
                    m_isWeaponDrawn = true;
                }
            }
        }
    }
}