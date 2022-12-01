#include "PlayerPhysicsController.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Math/MathHelpers.h"

#if EE_DEVELOPMENT_TOOLS
#include "System/Drawing/DebugDrawing.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathStringHelpers.h"
#endif

//-------------------------------------------------------------------------

namespace EE::Player
{
    static float g_floorDetectionDistance = 0.01f; // meters
    static float g_cylinderDiscretisationOffset = 0.02f; // meters

    //-------------------------------------------------------------------------

    Vector CorrectOverlappingPosition( Physics::SweepResults const& sweepResults )
    {
        EE_ASSERT( sweepResults.hasBlock && sweepResults.block.hadInitialOverlap() );

        // Sometime PhysX will detect an initial overlap with 0.0 penetration depth,
        // this is due to the discrepancy between math of the scene query (sweep) and the geo query (use for penetration dept calculation).
        // Read this for more info : 
        // https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxguide/Manual/BestPractices.html#character-controller-systems-using-scene-queries-and-penetration-depth-computation

        Vector normal = Physics::FromPx( sweepResults.block.normal );

        if( sweepResults.block.distance == 0.0f )
        {
            // When the distance is 0 the geometry query doesn't detect the penetration
            // and so the base behavior for the sweep is to set the normal as the opposite of the sweep
            // even if the collision is only touching on the side and not blocking.
            // In that case, we should push away from the collision using the collision point !
            normal = Vector( sweepResults.m_sweepStart - Physics::FromPx( sweepResults.block.position ) ).GetNormalized3();
        }

        float const penetrationDistance = Math::Abs( sweepResults.block.distance ) + Physics::Scene::s_sweepSeperationDistance;
        Vector const correctedStartPosition = sweepResults.m_sweepStart + ( normal * penetrationDistance );
        return correctedStartPosition;
    }

    bool CharacterPhysicsController::TryMoveCapsule( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, Vector const& deltaTranslation, Quaternion const& deltaRotation )
    {
        Transform const& capsuleOriginalWorldTransform = m_pCharacterComponent->GetCapsuleWorldTransform();
        Transform finalCapsuleWorldTransform = capsuleOriginalWorldTransform;

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debug_characterCapsule )
        {
            m_debug_capsuleBeforeTransform = capsuleOriginalWorldTransform;
        }
        #endif

        // No Clip / Ghost Mode
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_isInGhostMode )
        {
            finalCapsuleWorldTransform.AddTranslation( deltaTranslation );
        }
        else
        #endif

        // Collision Tests
        //-------------------------------------------------------------------------

        {
            pPhysicsScene->AcquireReadLock();
            finalCapsuleWorldTransform = SweepCharacterThroughWorld( ctx, pPhysicsScene, capsuleOriginalWorldTransform, deltaTranslation );
            finalCapsuleWorldTransform = ApplyGravity( ctx, pPhysicsScene, finalCapsuleWorldTransform );
            pPhysicsScene->ReleaseReadLock();
        }

        // Set the final character position
        //-------------------------------------------------------------------------

        auto characterWorldTransform = m_pCharacterComponent->CalculateWorldTransformFromCapsuleTransform( finalCapsuleWorldTransform );
        characterWorldTransform.SetRotation( deltaRotation * characterWorldTransform.GetRotation() );
        m_pCharacterComponent->MoveCharacter( ctx.GetDeltaTime(), characterWorldTransform );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        auto debugRenderer = ctx.GetDrawingContext();
        if ( m_debug_characterCapsule )
        {
            Transform const& worldTransform = m_pCharacterComponent->GetCapsuleWorldTransform();
            m_debug_capsuleAfterTransform = worldTransform;
            float const characterRadius = m_pCharacterComponent->GetCapsuleRadius();
            float const characterHalfHeight = m_pCharacterComponent->GetCapsuleHalfHeight();
            debugRenderer.DrawCapsuleHeightX( m_debug_capsuleBeforeTransform, characterRadius, characterHalfHeight, Colors::Gold );
            debugRenderer.DrawCapsuleHeightX( m_debug_capsuleAfterTransform, characterRadius, characterHalfHeight, Colors::Lime );
        }

        if( m_debug_visualizeFloor )
        {
            Vector const floorPosition = m_pCharacterComponent->GetCapsuleBottom();
            debugRenderer.DrawPoint( floorPosition, Colors::Yellow );
            debugRenderer.DrawArrow( floorPosition, floorPosition + ( m_floorNormal * 0.5f ), Colors::Yellow );
            debugRenderer.DrawCircle( floorPosition, Axis::Z, m_pCharacterComponent->GetCapsuleRadius(), Colors::Yellow );
        }
        #endif

        //-------------------------------------------------------------------------

        return true;
    }

    Transform CharacterPhysicsController::SweepCharacterThroughWorld( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, Transform const& capsuleWorldTransform, Vector const& deltaTranslation )
    {
        Transform finalWorldTransform = capsuleWorldTransform;

        // Try project movement along the current floor
        //-------------------------------------------------------------------------

        Vector deltaMovement = deltaTranslation;
        if ( m_floorType == FloorType::Navigable && m_projectOntoFloor )
        {
            Vector const projection = Plane::FromNormal( m_floorNormal ).ProjectVectorVertically( deltaTranslation );
            deltaMovement = projection.GetNormalized3() * deltaTranslation.GetLength3();
        }

        // Calculate cylinder shape dimensions
        //-------------------------------------------------------------------------

        float const stepHeight = m_isStepHeightEnabled ? m_settings.m_stepHeight : 0.0f;
        float const cylinderRadius = m_pCharacterComponent->GetCapsuleRadius();
        float const cylinderHalfHeight = Math::Max( 0.01f, ( ( m_pCharacterComponent->GetCharacterHeight() - stepHeight ) / 2 ) ); // Step-height can only be so high

        Vector const characterTop = capsuleWorldTransform.GetTranslation() + Vector( 0, 0, m_pCharacterComponent->GetCharacterHalfHeight() );
        Vector const cylinderPosition( characterTop - Vector( 0, 0, cylinderHalfHeight ) );
        Vector const cylinderOffset( cylinderPosition - capsuleWorldTransform.GetTranslation() );

        int32_t moveRecursion = 0;
        auto moveResult = SweepCylinder( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, cylinderPosition, deltaMovement, moveRecursion );

        finalWorldTransform.SetTranslation( moveResult.GetFinalPosition() - cylinderOffset );
        return finalWorldTransform;
    }

    Transform CharacterPhysicsController::ApplyGravity( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, Transform const& capsuleWorldTransform )
    {
        Transform finalWorldTransform = capsuleWorldTransform;

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debug_verticalSweep )
        {
            m_debug_verticalSweepBeforePos = capsuleWorldTransform.GetTranslation();
        }
        #endif

        if ( !m_isGravityEnabled )
        {
            m_verticalSpeed = 0.f;
            return finalWorldTransform;
        }

        // Calculate vertical delta for this frame
        //-------------------------------------------------------------------------

        m_verticalSpeed -= ( m_settings.m_gravitationalAcceleration * ctx.GetDeltaTime() );
        Vector const verticalAdjustment = Vector::UnitZ * m_verticalSpeed * ctx.GetDeltaTime();

        // Collision Checks
        //-------------------------------------------------------------------------

        // Because we are using a mesh for the cylinder the capsule need to be slightly smaller to account for the discretization and be fully inside the cylinder
        float const capsuleRadius = m_pCharacterComponent->GetCapsuleRadius() - g_cylinderDiscretisationOffset;
        float const capsuleHalfHeight = m_pCharacterComponent->GetCapsuleHalfHeight();

        int32_t verticalMoveRecursion = 0;
        auto const verticalMoveResult = SweepCapsuleVertical( ctx, pPhysicsScene, capsuleHalfHeight, capsuleRadius, capsuleWorldTransform.GetTranslation(), verticalAdjustment, m_isStepHeightEnabled ? m_settings.m_stepHeight : 0.f, verticalMoveRecursion );
        finalWorldTransform.SetTranslation( verticalMoveResult.GetFinalPosition() );

        // Update ground state
        //-------------------------------------------------------------------------

        if ( verticalMoveResult.GetSweepResults().hasBlock )
        {
            // Collided with a "ceiling"
            if ( verticalAdjustment.m_z > Math::Epsilon )
            {
                m_floorType = FloorType::NoFloor;
                m_verticalSpeed = 0.0;
                m_floorNormal = Vector::Zero;
                m_timeWithoutFloor.Update( ctx.GetDeltaTime() );
            }
            else // Collided with a "floor"
            {
                Vector const normal = Physics::FromPx( verticalMoveResult.GetSweepResults().block.normal );
                if ( Math::GetAngleBetweenNormalizedVectors( normal, Vector::UnitZ ) < m_settings.m_maxNavigableSlopeAngle )
                {
                    m_floorType = FloorType::Navigable;
                    m_verticalSpeed = 0.0;
                    m_floorNormal = normal;
                    m_timeWithoutFloor.Reset();
                }
                else // Wall/Slope
                {
                    m_floorType = FloorType::Unnavigable;
                    m_floorNormal = Vector::Zero;
                    m_timeWithoutFloor.Reset();
                }
            }
        }
        else // In-Air
        {
            m_floorType = FloorType::NoFloor;
            m_floorNormal = Vector::Zero;
            m_timeWithoutFloor.Update( ctx.GetDeltaTime() );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debug_verticalSweep )
        {
            m_debug_verticalSweepAfterPos = finalWorldTransform.GetTranslation();
        }
        #endif

        return finalWorldTransform;
    }

    CharacterPhysicsController::MoveResult CharacterPhysicsController::SweepCylinder( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, float cylinderHalfHeight, float cylinderRadius, Vector const& startPosition, Vector const& deltaTranslation, int32_t& idx )
    {
        MoveResult moveResult( startPosition );
        idx++;

        // We should not reproject that many time, so just early-out
        if ( idx > 10 )
        {
            return moveResult;
        }

        // Extract direction and length
        Vector moveDirection;
        float distance = 0.0f;
        deltaTranslation.ToDirectionAndLength3( moveDirection, distance );

        // We need to force a direction and distance to test for initial penetration even if the displacement is null.
        bool onlyApplyDepenetration = false;
        if ( distance < Math::Epsilon )
        {
            moveDirection = -Vector::UnitZ;
            distance = 0.01f;
            onlyApplyDepenetration = true;
        }

        // Initialize query filter
        Physics::QueryFilter filter;
        filter.SetLayerMask( m_settings.m_physicsLayerMask );
        filter.AddIgnoredEntity( m_pCharacterComponent->GetEntityID() );
        for( auto const& ignoredActor : m_settings.m_ignoredActors )
        {
            filter.AddIgnoredEntity( ignoredActor );
        }

        Physics::SweepResults sweepResults;
        if ( pPhysicsScene->CylinderSweep( cylinderHalfHeight, cylinderRadius, m_pCharacterComponent->GetCapsuleOrientation(), startPosition, moveDirection, distance, filter, sweepResults ) )
        {
            if ( sweepResults.hasBlock && sweepResults.block.hadInitialOverlap() )
            {
                Vector const correctedStartPosition = CorrectOverlappingPosition( sweepResults );

                // Debug drawing
                #if EE_DEVELOPMENT_TOOLS
                if( m_debug_cylinderSweep )
                {
                    DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::initialPenetration, idx, DebugSweepShapeType::cylinder );
                }
                #endif

                // Re-sweep
                auto const depenetratedResult = SweepCylinder( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, correctedStartPosition, deltaTranslation, idx );
                moveResult.ApplyCorrectiveMove( depenetratedResult );
            }
            else
            {
                // We have no movement and only want to apply depenetration, we didn't have any penetration here, so return without moving
                if( onlyApplyDepenetration )
                {
                    return moveResult;
                }

                EE_ASSERT( sweepResults.hasBlock );

                float const initialHorizontalSpeed = deltaTranslation.GetLength2();
                Vector const collisionPosition = sweepResults.GetShapePosition();
                Vector const normal = Physics::FromPx( sweepResults.block.normal );

                // Collision with the floor
                Radians const slopeAngle = Math::GetAngleBetweenNormalizedVectors( normal, Vector::UnitZ );
                if( slopeAngle < m_settings.m_maxNavigableSlopeAngle )
                {
                    // Re-project the displacement vertically on the new ground
                    float const remainingDistance = sweepResults.GetRemainingDistance();
                    // The vertical projection is required since a simple projection tend to stir the direction along the perpendicular side of the slope
                    Vector const projectedDeltaTranslation = Plane::FromNormal( normal ).ProjectVectorVertically( deltaTranslation );
                    Vector const unitProjection = projectedDeltaTranslation.GetNormalized3();
                    Vector newDeltaMovement = unitProjection * remainingDistance;
                    EE_ASSERT( !deltaTranslation.IsEqual3( newDeltaMovement ) );

                    // Clamp the horizontal speed to the initial speed
                    float const newHorizontalSpeed = newDeltaMovement.GetLength2();
                    if( newHorizontalSpeed > initialHorizontalSpeed )
                    {
                        float const ratio = initialHorizontalSpeed / newHorizontalSpeed;
                        newDeltaMovement.m_x *= ratio;
                        newDeltaMovement.m_y *= ratio;
                    }

                    // Debug drawing
                    #if EE_DEVELOPMENT_TOOLS
                    if( m_debug_cylinderSweep )
                    {
                        DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::reprojection, idx, DebugSweepShapeType::cylinder );
                    }
                    #endif

                    auto const reprojectedResult = SweepCylinder( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, collisionPosition, newDeltaMovement, idx );
                    moveResult.ApplySubsequentMove( reprojectedResult );
                }
                // Collision with a wall / Unnavigable slope
                else
                {
                    // Only reproject if the direction is close to (60 degree +) perpendicular to the normal
                    Radians const normalAngle = Math::GetAngleBetweenNormalizedVectors( normal, -moveDirection );
                    if( normalAngle > m_settings.m_wallSlideAngle )
                    {
                        // Re-project the displacement along the wall
                        float const remainingDistance = sweepResults.GetRemainingDistance();

                        // Use a regular vector projection since we want to keep the vertically momentum when sliding along a wall;
                        Vector const projectedDeltaTranslation = Plane::FromNormal( normal ).ProjectVector( deltaTranslation );
                        Vector const unitProjection = projectedDeltaTranslation.GetNormalized3();
                        Vector newDeltaMovement = unitProjection * remainingDistance;
                        EE_ASSERT( !deltaTranslation.IsEqual3( newDeltaMovement ) );

                        // Clamp the horizontal speed to the initial speed
                        float const newHorizontalSpeed = newDeltaMovement.GetLength2();
                        if( newHorizontalSpeed > initialHorizontalSpeed )
                        {
                            float const ratio = initialHorizontalSpeed / newHorizontalSpeed;
                            newDeltaMovement.m_x *= ratio;
                            newDeltaMovement.m_y *= ratio;
                        }

                        // Debug drawing
                        #if EE_DEVELOPMENT_TOOLS
                        if( m_debug_cylinderSweep )
                        {
                            DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::reprojection, idx, DebugSweepShapeType::cylinder );
                        }
                        #endif

                        auto const reprojectedResult = SweepCylinder( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, collisionPosition, newDeltaMovement, idx );
                        moveResult.ApplySubsequentMove( reprojectedResult );
                    }
                    else
                    {
                        // Debug drawing
                        #if EE_DEVELOPMENT_TOOLS
                        if( m_debug_cylinderSweep )
                        {
                            DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::collision, idx, DebugSweepShapeType::cylinder );
                        }
                        #endif

                        moveResult.FinalizePosition( sweepResults );
                    }
                }
            }
        }
        else
        {
            // Debug drawing
            #if EE_DEVELOPMENT_TOOLS
            if( m_debug_cylinderSweep )
            {
                DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::success, idx, DebugSweepShapeType::cylinder );
            }
            #endif

            moveResult.FinalizePosition( sweepResults );
        }

        return moveResult;
    }

    CharacterPhysicsController::MoveResult CharacterPhysicsController::SweepCapsuleVertical( EntityWorldUpdateContext const& ctx, Physics::Scene* pPhysicsScene, float cylinderHalfHeight, float cylinderRadius, Vector const& startPosition, Vector const& deltaTranslation, float stepHeight, int32_t& idx )
    {
        MoveResult moveResult( startPosition );
        idx++;

        if( idx > 10 )
        {
            return moveResult;
        }

        // Sweep going up, this is a different than the regular sweep going down,
        // since we don't need to account for the step height.
        if( deltaTranslation.m_z > 0 )
        {
            // Extract direction and length
            Vector moveDirection;
            float distance = 0.0f;
            deltaTranslation.ToDirectionAndLength3( moveDirection, distance );

            // Initialize query filter
            Physics::QueryFilter filter;
            filter.SetLayerMask( m_settings.m_physicsLayerMask );
            filter.AddIgnoredEntity( m_pCharacterComponent->GetEntityID() );
            for( auto ignoredActor : m_settings.m_ignoredActors )
            {
                filter.AddIgnoredEntity( ignoredActor );
            }

            Physics::SweepResults sweepResults;
            if( pPhysicsScene->CapsuleSweep( cylinderHalfHeight, cylinderRadius, m_pCharacterComponent->GetCapsuleOrientation(), startPosition, moveDirection, distance, filter, sweepResults ) )
            {
                // Initial overlap
                if( sweepResults.hasBlock && sweepResults.block.hadInitialOverlap() )
                {
                    // This should not happen since we swept vertically and resolved collision earlier, but we should handle it just in case I'm proved wrong
                    EE_ASSERT( false ); // this assumption need to be validated

                    Vector const correctedStartPosition = CorrectOverlappingPosition( sweepResults );

                    // Debug drawing
                    #if EE_DEVELOPMENT_TOOLS
                    if( m_debug_verticalSweep )
                    {
                        DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::initialPenetration, idx, DebugSweepShapeType::capsule );
                    }
                    #endif

                    auto const depenetratedResult = SweepCapsuleVertical( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, correctedStartPosition, deltaTranslation, stepHeight, idx );
                    moveResult.ApplyCorrectiveMove( depenetratedResult );
                    return moveResult;
                }
                else // Collision
                {
                    // Debug drawing
                    #if EE_DEVELOPMENT_TOOLS
                    if( m_debug_verticalSweep )
                    {
                        DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::collision, idx, DebugSweepShapeType::capsule );
                    }
                    #endif

                    moveResult.FinalizePosition( sweepResults );
                    return moveResult;
                }
            }
            else // No collision
            {
                // Debug drawing
                #if EE_DEVELOPMENT_TOOLS
                if( m_debug_verticalSweep )
                {
                    DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::success, idx, DebugSweepShapeType::capsule );
                }
                #endif

                moveResult.FinalizePosition( sweepResults );
                return moveResult;
            }
        }
        else // sweep going down, we need to account for the step height
        {
            bool const onlySweepForStepHeight = deltaTranslation.IsNearZero3();

            Vector const stepHeightOffset( 0.f, 0.f, stepHeight );
            Vector const floorDetectionDistance( 0.f, 0.f, g_floorDetectionDistance ); // had some floor detection distance just to update the ground state
            Vector const downSweep = deltaTranslation - stepHeightOffset - floorDetectionDistance;

            // Extract direction and length
            Vector moveDirection;
            float distance = 0.0f;
            downSweep.ToDirectionAndLength3( moveDirection, distance );

            bool onlyApplyDepenetration = false;
            if( Math::IsNearEqual( distance, g_floorDetectionDistance ) )
            {
                // We need to force a direction and distance to test for initial penetration even if the displacement is null.
                moveDirection = -Vector::UnitZ;
                distance = 0.01f;
                onlyApplyDepenetration = true;
            }

            // Initialize query filter
            Physics::QueryFilter filter;
            filter.SetLayerMask( m_settings.m_physicsLayerMask );
            filter.AddIgnoredEntity( m_pCharacterComponent->GetEntityID() );
            for( auto IgnoredActor : m_settings.m_ignoredActors )
            {
                filter.AddIgnoredEntity( IgnoredActor );
            }

            Physics::SweepResults sweepResults;
            if( pPhysicsScene->CapsuleSweep( cylinderHalfHeight, cylinderRadius, m_pCharacterComponent->GetCapsuleOrientation(), startPosition, moveDirection, distance, filter, sweepResults ) )
            {
                if( sweepResults.hasBlock && sweepResults.block.hadInitialOverlap() )
                {
                    // This should not happen since we swept vertically and resolved collision earlier, but we should handle it just in case I'm proved wrong
                    //EE_ASSERT( false ); // this assumption need to be validated

                    Vector const correctedStartPosition = CorrectOverlappingPosition( sweepResults );

                    // Debug drawing
                    #if EE_DEVELOPMENT_TOOLS
                    if( m_debug_verticalSweep )
                    {
                        DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::initialPenetration, idx, DebugSweepShapeType::capsule );
                    }
                    #endif

                    auto const depenetratedResult = SweepCapsuleVertical( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, correctedStartPosition, deltaTranslation, stepHeight, idx );
                    moveResult.ApplyCorrectiveMove( depenetratedResult );
                    return moveResult;
                }
                else
                {
                    // We did not want try move, just resolve the initial overlaps.
                    if( onlyApplyDepenetration )
                    {
                        return moveResult;
                    }

                    // there was no movement but we need to adjust for the step height
                    if( onlySweepForStepHeight )
                    {
                        // Debug drawing
                        #if EE_DEVELOPMENT_TOOLS
                        if( m_debug_verticalSweep )
                        {
                            DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::collision, idx, DebugSweepShapeType::capsule );
                        }
                        #endif

                        moveResult.FinalizePosition( sweepResults );
                        return moveResult;
                    }

                    // Check if we collided in the extra floorDetectionDistance at the end of the sweep
                    // and adjust the position has if no collision had happen,
                    // but this will register the floor for the ground state.
                    float const collisionDistanceWithEndOfRealSweep = sweepResults.block.distance - (distance - g_floorDetectionDistance);
                    if( collisionDistanceWithEndOfRealSweep >= 0.f )
                    {
                        // Debug drawing
                        #if EE_DEVELOPMENT_TOOLS
                        if( m_debug_verticalSweep )
                        {
                            DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::success, idx, DebugSweepShapeType::capsule );
                        }
                        #endif

                        moveResult.FinalizePosition( sweepResults );
                        return moveResult;
                    }
                    else // this is a real collision
                    {
                        // We need to do a ray cast to get the actual normal of the collision,
                        // because of the round shape of the capsule we could detect sharp corner as slope 
                        // and invalidate a step-up move because of that.

                        Vector const collisionPos = Physics::FromPx( sweepResults.block.position );
                        Vector const SeparationOffset( 0, 0, Physics::Scene::s_sweepSeperationDistance );

                        Physics::RayCastResults rayCastResults;
                        pPhysicsScene->RayCast( collisionPos + SeparationOffset, collisionPos - SeparationOffset, filter, rayCastResults );

                        if( rayCastResults.hasBlock )
                        {
                            Vector const rayCastNormal = Physics::FromPx( rayCastResults.block.normal );
                            Radians const SlopeAngle = Math::GetAngleBetweenNormalizedVectors( rayCastNormal, Vector::UnitZ );

                            // Only reproject if the slope angle is not navigable
                            if( SlopeAngle > m_settings.m_maxNavigableSlopeAngle )
                            {
                                // Re-project the displacement along the wall
                                Vector const collisionPosition = sweepResults.GetShapePosition();
                                float const remainingDistance = sweepResults.GetRemainingDistance();

                                // Use a regular vector projection since we want to keep the vertically momentum when sliding along a wall;
                                Vector const projectedDeltaTranslation = Plane::FromNormal( rayCastNormal ).ProjectVector( downSweep );
                                Vector const unitProjection = projectedDeltaTranslation.GetNormalized3();
                                Vector const newDeltaMovement = unitProjection * remainingDistance;

                                // Debug drawing
                                #if EE_DEVELOPMENT_TOOLS
                                if( m_debug_verticalSweep )
                                {
                                    DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::reprojection, idx, DebugSweepShapeType::capsule );
                                }
                                #endif

                                auto const reprojectedResult = SweepCapsuleVertical( ctx, pPhysicsScene, cylinderHalfHeight, cylinderRadius, collisionPosition, newDeltaMovement, 0.f, idx );
                                moveResult.ApplySubsequentMove( reprojectedResult, stepHeightOffset );
                                return moveResult;
                            }
                            else // we this is a valid floor
                            {
                                // Debug drawing
                                #if EE_DEVELOPMENT_TOOLS
                                if( m_debug_verticalSweep )
                                {
                                    DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::collision, idx, DebugSweepShapeType::capsule );
                                }
                                #endif

                                moveResult.FinalizePosition( sweepResults );
                                return moveResult;
                            }
                        }
                        // Ray cast did not hit, meaning this was a punctual collision,
                        // likely the result of a collision with an edge, we consider this as a navigable surface.
                        else
                        {
                            // Debug drawing
                            #if EE_DEVELOPMENT_TOOLS
                            if( m_debug_verticalSweep )
                            {
                                DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::collision, idx, DebugSweepShapeType::capsule );
                            }
                            #endif

                            moveResult.FinalizePosition( sweepResults );
                            return moveResult;
                        }
                    }
                }
            }
            else
            {
                // We did not want try move, just resolve the initial overlaps.
                if( onlyApplyDepenetration )
                {
                    return moveResult;
                }

                if( onlySweepForStepHeight )
                {
                    // Debug drawing
                    #if EE_DEVELOPMENT_TOOLS
                    if( m_debug_verticalSweep )
                    {
                        DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::success, idx, DebugSweepShapeType::capsule );
                    }
                    #endif

                    moveResult.FinalizePosition( sweepResults );
                    return moveResult;
                }

                // Debug drawing
                #if EE_DEVELOPMENT_TOOLS
                if( m_debug_verticalSweep )
                {
                    DrawDebugSweep( ctx, cylinderHalfHeight, cylinderRadius, startPosition, sweepResults.GetShapePosition(), DebugSweepResultType::success, idx, DebugSweepShapeType::capsule );
                }
                #endif

                moveResult.FinalizePosition( sweepResults );
                return moveResult;
            }
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void CharacterPhysicsController::DrawDebugSweep( EntityWorldUpdateContext const& ctx, float cylinderHalfHeight, float cylinderRadius, Vector const& startPosition, Vector const& endPosition, DebugSweepResultType resultType, int32_t idx, DebugSweepShapeType shapeType )
    {
        Color startColor = idx == 1 ? Colors::GreenYellow : Colors::OrangeRed;
        Color endColor;
        switch( resultType )
        {
        case DebugSweepResultType::success:
            endColor = Colors::GreenYellow;
            break;
        case DebugSweepResultType::reprojection:
            endColor = Colors::OrangeRed;
            break;
        case DebugSweepResultType::collision:
            endColor = Colors::Red;
            break;
        case DebugSweepResultType::initialPenetration:
            endColor = Colors::MediumPurple;
            break;
        default:
            break;
        }

        Transform startTrans( m_pCharacterComponent->GetOrientation(), startPosition );
        Transform endTrans( m_pCharacterComponent->GetOrientation(), endPosition );

        Drawing::DrawContext drawContext = ctx.GetDrawingContext();
        if( resultType != DebugSweepResultType::initialPenetration )
        {
            if( shapeType == DebugSweepShapeType::cylinder )
            {
                drawContext.DrawCylinder( startTrans, cylinderRadius, cylinderHalfHeight, startColor );
                drawContext.DrawLine( startPosition, endPosition, endColor );
                drawContext.DrawCylinder( endTrans, cylinderRadius, cylinderHalfHeight, endColor );
            }
            else if( shapeType == DebugSweepShapeType::capsule )
            {
                drawContext.DrawCapsule( startTrans, cylinderRadius, cylinderHalfHeight, startColor );
                drawContext.DrawLine( startPosition, endPosition, endColor );
                drawContext.DrawCapsule( endTrans, cylinderRadius, cylinderHalfHeight, endColor );
            }
        }
        else
        {
            if( shapeType == DebugSweepShapeType::cylinder )
            {
                drawContext.DrawCylinder( startTrans, cylinderRadius, cylinderHalfHeight, endColor );
            }
            else if( shapeType == DebugSweepShapeType::capsule )
            {
                drawContext.DrawCapsule( startTrans, cylinderRadius, cylinderHalfHeight, endColor );
            }
        }
    }

    void CharacterPhysicsController::DrawDebugUI()
    {
        ImGuiX::TextSeparator( "Capsule" );
        ImGui::Checkbox( "Debug Capsule", &m_debug_characterCapsule );
        if ( m_debug_characterCapsule )
        {
            ImGui::Text( "Before: %s", Math::ToString( m_debug_capsuleBeforeTransform.GetTranslation() ).c_str() );
            ImGui::Text( "After: %s", Math::ToString( m_debug_capsuleAfterTransform.GetTranslation() ).c_str() );
        }

        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Character Sweep" );
        ImGui::Checkbox( "Debug Cylinder Sweep", &m_debug_cylinderSweep );

        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Vertical Sweep" );
        ImGui::Checkbox( "Debug Vertical Sweep", &m_debug_verticalSweep );
        if ( m_debug_verticalSweep )
        {
            ImGui::Text( "Before: %s", Math::ToString( m_debug_verticalSweepBeforePos ).c_str() );
            ImGui::Text( "After: %s", Math::ToString( m_debug_verticalSweepAfterPos ).c_str() );
        }

        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Floor" );
        ImGui::Checkbox( "Draw Floor", &m_debug_visualizeFloor );

        switch ( m_floorType )
        {
            case FloorType::Navigable:
            {
                ImGuiX::ScopedFont sf( ImGuiX::ImColors::Lime );
                ImGui::Text( "Floor Type: Navigable" );
            }
            break;

            case FloorType::Unnavigable:
            {
                ImGuiX::ScopedFont sf( ImGuiX::ImColors::Red );
                ImGui::Text( "Floor Type: Unnavigable" );
            }
            break;

            case FloorType::NoFloor:
            {
                ImGui::Text( "Floor Type: None" );
            }
            break;
        }
    }
    #endif
}