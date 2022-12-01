#include "AIPhysicsController.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Math/Transform.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    bool CharacterPhysicsController::TryMoveCapsule( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, Vector const& deltaTranslation, Quaternion const& deltaRotation )
    {
        Transform capsuleWorldTransform = m_pCharacterComponent->GetCapsuleWorldTransform();

        // Create sphere to correct Z-position
        //-------------------------------------------------------------------------

        float const sphereRadiusReduction = 0.2f;
        Vector const sphereOrigin = capsuleWorldTransform.GetTranslation();

        // Attempt a sweep from current position to the bottom of the capsule including some "gravity"
        float const verticalDistanceAllowedToTravelThisFrame = ( ctx.GetDeltaTime() * 0.5f );
        Vector halfHeightVector = Vector( 0, 0, m_pCharacterComponent->GetCapsuleHalfHeight() + sphereRadiusReduction );
        Vector sweepStartPos = sphereOrigin + deltaTranslation + halfHeightVector;
        Vector sweepEndPos = sweepStartPos - Vector( 0, 0, ( ( m_pCharacterComponent->GetCapsuleHalfHeight() + sphereRadiusReduction ) * 2 ) + verticalDistanceAllowedToTravelThisFrame );
        Vector capsuleFinalPosition = sphereOrigin;

        #if EE_DEVELOPMENT_TOOLS
        auto drawingContext = ctx.GetDrawingContext();
        //drawingContext.DrawSphere( sweepStartPos, Vector( sphereRadius ), Colors::Gray, 5.0f );
        //drawingContext.DrawCapsuleHeightX( capsuleWorldTransform, m_pCharacterComponent->GetCapsuleRadius(), m_pCharacterComponent->GetCapsuleHalfHeight(), Colors::Red );
        #endif

        //-------------------------------------------------------------------------

        Physics::QueryFilter filter;
        filter.SetLayerMask( Physics::CreateLayerMask( Physics::Layers::Environment ) );
        filter.AddIgnoredEntity( m_pCharacterComponent->GetEntityID() );

        pPhysicsScene->AcquireReadLock();

        Physics::SweepResults sweepResults;
        if ( pPhysicsScene->SphereSweep( m_pCharacterComponent->GetCapsuleRadius(), sweepStartPos, sweepEndPos, filter, sweepResults ) )
        {
            if ( sweepResults.HadInitialOverlap() )
            {
                //EE_UNIMPLEMENTED_FUNCTION();
            }
            else
            {
                #if EE_DEVELOPMENT_TOOLS
                //drawingContext.DrawSphere( sweepResults.GetShapePosition(), Vector( sphereRadius ), Colors::Yellow, 2.0f );
                #endif

                capsuleFinalPosition = sweepResults.GetShapePosition() + halfHeightVector;
            }
        }
        else
        {
            capsuleFinalPosition = sweepEndPos + halfHeightVector;

            #if EE_DEVELOPMENT_TOOLS
            //drawingContext.DrawSphere( capsuleFinalPosition, Vector( sphereRadius ), Colors::LimeGreen, 2.0f );
            #endif
        }

        pPhysicsScene->ReleaseReadLock();

        //-------------------------------------------------------------------------

        Transform finalCapsuleWorldTransform = capsuleWorldTransform;
        finalCapsuleWorldTransform.SetTranslation( capsuleFinalPosition );

        // Apply rotation delta and move character
        Transform newCharacterTransform = m_pCharacterComponent->CalculateWorldTransformFromCapsuleTransform( finalCapsuleWorldTransform );
        newCharacterTransform.AddRotation( deltaRotation );
        m_pCharacterComponent->MoveCharacter( ctx.GetDeltaTime(), newCharacterTransform );
        return true;
    }
}