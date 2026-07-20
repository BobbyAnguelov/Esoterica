#include "Component_Character.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE
{
    CharacterComponent::CharacterComponent()
        : Physics::CapsuleComponent()
    {
        m_mobility = Physics::Mobility::Kinematic;
    }

    CharacterComponent::CharacterComponent( StringID name )
        : Physics::CapsuleComponent( name )
    {
        m_mobility = Physics::Mobility::Kinematic;
    }

    void CharacterComponent::Initialize()
    {
        CapsuleComponent::Initialize();

        m_mobility = Physics::Mobility::Kinematic;

        m_queryRules.SetCategory( Physics::ObjectCategory::Character );
        m_queryRules.SetCollisionMask( m_collisionSettings.m_collisionMask );
        m_queryRules.AddIgnoredComponent( GetID() );
    }

    void CharacterComponent::Shutdown()
    {
        CapsuleComponent::Shutdown();
    }

    void CharacterComponent::SetStepHeight( float stepHeight )
    {
        EE_ASSERT( stepHeight > 0.0f );
        m_stepHeight = stepHeight;
    }

    void CharacterComponent::SetSlopeLimit( Degrees slopeLimit )
    {
        EE_ASSERT( slopeLimit > 0.0f );
        m_slopeLimit = slopeLimit;
    }

    void CharacterComponent::SetGravityScale( float gravityScale )
    {
        EE_ASSERT( gravityScale >= 0.0f );
        m_gravityScale = gravityScale;
    }

    void CharacterComponent::ResetGravityScale()
    {
        m_gravityScale = 1.0f;
    }

    #if EE_DEVELOPMENT_TOOLS
    void CharacterComponent::EnableGhostMode( bool isEnabled )
    {
        m_velocity = Vector::Zero;
        m_floorType = FloorType::None;
        m_floorNormal = Vector::WorldUp;
        m_floorContactPoint = Vector::Zero;
        m_isGhostModeEnabled = isEnabled;
    }
    #endif

    //-------------------------------------------------------------------------
    // Character Physics
    //-------------------------------------------------------------------------

    struct PlaneExtra
    {
        b3Vec3                                  m_point;
        b3ShapeId                               m_shapeID;
    };

    struct SolverContext
    {
        void ClearPlanes()
        {
            m_planes.clear();
            m_planeExtras.clear();
        }

        inline int32_t GetNumPlanes() const { return (int32_t) m_planes.size(); }

    public:

        CharacterComponent*                     m_pCC = nullptr;
        b3WorldId                               m_worldID = {};
        b3QueryFilter                           m_moverFilter = {};
        TInlineVector<b3CollisionPlane, 100>    m_planes;
        TInlineVector<PlaneExtra, 100>          m_planeExtras;
    };

    bool MoverFilterFcn( b3ShapeId shapeID, void* pUserContext )
    {
        SolverContext* pSolverContext = static_cast<SolverContext*>( pUserContext );

        b3BodyId const bodyID = b3Shape_GetBody( shapeID );
        if ( B3_ID_EQUALS( bodyID, pSolverContext->m_pCC->m_physicsBodyID ) )
        {
            return false;
        }

        return true;
    }

    static bool PlaneResultFcn( b3ShapeId shapeID, const b3PlaneResult* planeResults, int32_t planeCount, void* pUserContext )
    {
        if ( !MoverFilterFcn( shapeID, pUserContext ) )
        {
            // Ignore these planes but continue the search
            return true;
        }

        SolverContext* pSolverContext = static_cast<SolverContext*>( pUserContext );

        //-------------------------------------------------------------------------

        float maxPush = FLT_MAX;

        pSolverContext->m_planes.reserve( planeCount );
        pSolverContext->m_planeExtras.reserve( planeCount );

        for ( int32_t i = 0; i < planeCount; ++i )
        {
            if ( !b3IsValidPlane( planeResults[i].plane ) )
            {
                continue;
            }

            pSolverContext->m_planes.push_back
            (
                {
                    .plane = planeResults[i].plane,
                    .pushLimit = maxPush,
                    .push = 0.0f,
                    .clipVelocity = false,
                }
            );

            pSolverContext->m_planeExtras.push_back
            (
                {
                    .m_point = planeResults[i].point,
                    .m_shapeID = shapeID,
                }
            );
        }

        return true;
    }

    void CharacterComponent::MoveCharacter( Seconds deltaTime, Quaternion const& deltaRotation, Vector const& deltaTranslation )
    {
        EE_ASSERT( deltaTime > 0.0f );
        EE_PROFILE_FUNCTION_GAMEPLAY();

        Transform const startWorldTransform = GetWorldTransform();
        Transform endWorldTransform;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugCapsule )
        {
            m_debugPreMoveTransform = startWorldTransform;
        }
        #endif

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_isGhostModeEnabled )
        {
            endWorldTransform.SetRotation( deltaRotation * startWorldTransform.GetRotation() );
            endWorldTransform.SetTranslation( startWorldTransform.GetTranslation() + deltaTranslation );
            TeleportTo( endWorldTransform );
        }
        #endif

        //-------------------------------------------------------------------------

        SolverContext ctx;
        ctx.m_pCC = this;
        ctx.m_worldID = m_pPhysicsWorld->GetWorldID();
        ctx.m_moverFilter = Physics::ToBox3D( m_queryRules );

        Vector desiredDeltaTranslation = deltaTranslation;

        // Gravity
        //-------------------------------------------------------------------------

        b3Transform currentTransform = Physics::ToBox3D( startWorldTransform );

        Physics::CastQuery query = m_queryRules;
        m_pPhysicsWorld->RayCast( startWorldTransform.GetTranslation(), Vector( 0, 0, -1.f ), s_groundProbeLength, query );

        if ( query.HasHits() )
        {
            EE_ASSERT( !B3_ID_EQUALS( query.m_hits[0].m_bodyID, m_physicsBodyID ) );

            Vector const capsuleBottom = startWorldTransform.GetTranslation() + Vector( 0, 0, -GetCharacterHalfHeight() );

            m_floorType = FloorType::Floor;
            m_timeWithoutFloor.Reset();
            m_floorContactPoint = query.m_hits[0].m_contactPoint;
            m_floorNormal = query.m_hits[0].m_contactNormal;
            m_heightAboveGround = ( capsuleBottom - query.m_hits[0].m_contactPoint ).GetLength3();
        }
        else
        {
            m_floorType = FloorType::None;
            m_timeWithoutFloor.Update( deltaTime );
            m_heightAboveGround = s_groundProbeLength;
        }

        // Apply gravity
        if ( m_heightAboveGround > 0.001f && m_gravityScale > 0 )
        {
            Vector const worldGravity = Physics::FromBox3D( b3World_GetGravity( m_pPhysicsWorld->GetWorldID() ) );
            Vector const verticalDelta = worldGravity * ( m_gravityScale * deltaTime.ToFloat() );
            desiredDeltaTranslation += verticalDelta;
        }

        // Solve
        //-------------------------------------------------------------------------

        float const radius = GetCapsuleRadius();
        float const halfHeight = GetCapsuleHalfHeight();

        {
            Physics::ScopedReadLock srl( m_pPhysicsWorld );

            b3Vec3 target = currentTransform.p + Physics::ToBox3D( desiredDeltaTranslation );

            float tolerance = 0.01f;

            for ( int32_t iteration = 0; iteration < 5; ++iteration )
            {
                ctx.ClearPlanes();

                b3Capsule mover;
                mover.center1 = b3Vec3( 0, 0, halfHeight );
                mover.center2 = b3Vec3( 0, 0, -halfHeight );
                mover.radius = radius;

                b3World_CollideMover( ctx.m_worldID, currentTransform.p, &mover, ctx.m_moverFilter, PlaneResultFcn, &ctx );

                b3Vec3 targetDelta = target - currentTransform.p;
                b3PlaneSolverResult result = b3SolvePlanes( targetDelta, ctx.m_planes.data(), ctx.GetNumPlanes() );

                b3Vec3 delta = result.delta;
                float const fraction = b3World_CastMover( ctx.m_worldID, currentTransform.p, &mover, delta, ctx.m_moverFilter, MoverFilterFcn, &ctx );

                delta *= fraction;
                currentTransform.p += delta;

                if ( b3LengthSquared( delta ) < tolerance * tolerance )
                {
                    break;
                }
            }
        }

        // Update final end transform
        //-------------------------------------------------------------------------

        endWorldTransform.SetRotation( deltaRotation * startWorldTransform.GetRotation() );
        endWorldTransform.SetTranslation( Physics::FromBox3D( currentTransform.p ) );

        m_velocity = ( endWorldTransform.GetTranslation() - startWorldTransform.GetTranslation() ) / deltaTime;

        MoveTo( endWorldTransform );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugCapsule )
        {
            m_debugPostMoveTransform = endWorldTransform;
        }
        #endif
    }

    void CharacterComponent::TeleportCharacter( Transform const& targetTransform )
    {
        m_velocity = Vector::Zero;
        m_heightAboveGround = 0.0f;
        m_floorType = FloorType::None;
        m_floorContactPoint = Vector::Zero;
        m_floorNormal = Vector::Zero;
        m_timeWithoutFloor.Reset();

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugCapsule )
        {
            m_debugPreMoveTransform = targetTransform;
            m_debugPostMoveTransform = targetTransform;
        }
        #endif

        TeleportTo( targetTransform );
    }
}