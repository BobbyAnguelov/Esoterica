#include "AIPhysicsController.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Math/Transform.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    bool CharacterPhysicsController::TryMoveCapsule( EntityWorldUpdateContext const& ctx, Physics::PhysicsWorld* pPhysicsWorld, Vector const& deltaTranslation, Quaternion const& deltaRotation )
    {
        Transform worldTransform = m_pCharacterComponent->GetWorldTransform();

        // Create sphere to correct Z-position
        //-------------------------------------------------------------------------

        float const sphereRadiusReduction = 0.2f;
        Vector const sphereOrigin = worldTransform.GetTranslation();

        // Attempt a sweep from current position to the bottom of the capsule including some "gravity"
        float const verticalDistanceAllowedToTravelThisFrame = ( ctx.GetDeltaTime() * 0.5f );
        Vector halfHeightVector = Vector( 0, 0, m_pCharacterComponent->GetCapsuleHalfHeight() + sphereRadiusReduction );
        Vector sweepStartPos = sphereOrigin + deltaTranslation + halfHeightVector;
        Vector sweepEndPos = sweepStartPos - Vector( 0, 0, ( ( m_pCharacterComponent->GetCapsuleHalfHeight() + sphereRadiusReduction ) * 2 ) + verticalDistanceAllowedToTravelThisFrame );
        Vector capsuleFinalPosition = sphereOrigin;

        #if EE_DEVELOPMENT_TOOLS
        auto drawingContext = ctx.GetDrawingContext();
        //drawingContext.DrawSphere( sweepStartPos, Vector( sphereRadius ), Colors::Gray, 5.0f );
        //drawingContext.DrawCapsuleHeightX( worldTransform, m_pCharacterComponent->GetCapsuleRadius(), m_pCharacterComponent->GetCapsuleHalfHeight(), Colors::Red );
        #endif

        //-------------------------------------------------------------------------

        //Physics::QueryFilter filter;
        //filter.SetLayerMask( Physics::CreateLayerMask( Physics::Layers::Environment ) );
        //filter.AddIgnoredEntity( m_pCharacterComponent->GetEntityID() );

        //pPhysicsWorld->AcquireReadLock();

        //Physics::SweepResults sweepResults;
        //if ( pPhysicsWorld->SphereSweep( m_pCharacterComponent->GetCapsuleRadius(), sweepStartPos, sweepEndPos, filter, sweepResults ) )
        //{
        //    if ( sweepResults.HadInitialOverlap() )
        //    {
        //        //EE_UNIMPLEMENTED_FUNCTION();
        //    }
        //    else
        //    {
        //        #if EE_DEVELOPMENT_TOOLS
        //        //drawingContext.DrawSphere( sweepResults.GetShapePosition(), Vector( sphereRadius ), Colors::Yellow, 2.0f );
        //        #endif

        //        capsuleFinalPosition = sweepResults.GetShapePosition() + halfHeightVector;
        //    }
        //}
        //else
        //{
        //    capsuleFinalPosition = sweepEndPos + halfHeightVector;

        //    #if EE_DEVELOPMENT_TOOLS
        //    //drawingContext.DrawSphere( capsuleFinalPosition, Vector( sphereRadius ), Colors::LimeGreen, 2.0f );
        //    #endif
        //}

        //pPhysicsWorld->ReleaseReadLock();

        //-------------------------------------------------------------------------

        m_pCharacterComponent->MoveCharacter( ctx.GetDeltaTime(), deltaRotation, deltaTranslation );
        return true;
    }
}