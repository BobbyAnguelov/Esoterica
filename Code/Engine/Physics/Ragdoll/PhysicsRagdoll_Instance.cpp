#include "PhysicsRagdoll_Instance.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    Ragdoll::Ragdoll( PhysicsWorld* pWorld, RagdollDefinition const* pDefinition, int32_t userID )
        : m_pWorld( pWorld )
        , m_pDefinition( pDefinition )
        , m_userID( userID )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( pDefinition != nullptr && pDefinition->IsValid() );

        ScopedWriteLock const sl( m_pWorld );

        // Create bodies
        //-------------------------------------------------------------------------

        b3BodyDef bodyDef = b3DefaultBodyDef();
        bodyDef.type = b3_dynamicBody;

        int32_t const numBodies = pDefinition->GetNumBodies();
        m_createdBodies.resize( numBodies );

        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            auto const& body = pDefinition->m_bodies[bodyIdx];

            // Body
            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            bodyDef.name = body.m_debugName.c_str();
            #endif

            bodyDef.type = body.m_isKinematic ? b3_kinematicBody : b3_dynamicBody;
            bodyDef.rotation = ToBox3D( body.m_initialGlobalTransform.GetRotation() );
            bodyDef.position = ToBox3D( body.m_initialGlobalTransform.GetTranslation() );

            m_createdBodies[bodyIdx].m_bodyID = b3CreateBody( m_pWorld->GetWorldID(), &bodyDef );
            m_createdBodies[bodyIdx].m_material.friction = body.m_friction;
            m_createdBodies[bodyIdx].m_material.restitution = body.m_restitution;
            m_createdBodies[bodyIdx].m_material.rollingResistance = body.m_rollingResistance;

            // Shape
            //-------------------------------------------------------------------------

            b3ShapeDef shapeDef = b3DefaultShapeDef();

            for ( RagdollShapeDefinition const& shape : body.m_shapes )
            {
                shapeDef.density = shape.m_density;
                shapeDef.materials = &m_createdBodies[bodyIdx].m_material;
                shapeDef.materialCount = 1;
                shapeDef.filter = ToBox3D( shape.m_collisionSettings );
                shapeDef.filter.groupIndex = -userID; // Negative means that we should not collide

                if ( shape.m_type == RagdollShapeDefinition::Type::Capsule )
                {
                    Vector const halfLengthOffset = ( shape.m_offsetTransform.GetUpVector() * shape.m_halfHeight );
                    Vector const origin = shape.m_offsetTransform.GetTranslation();
                    Vector const center0 = origin + halfLengthOffset;
                    Vector const center1 = origin - halfLengthOffset;

                    b3Capsule capsule = { ToBox3D( center0 ), ToBox3D( center1 ), shape.m_radius };
                    b3CreateCapsuleShape( m_createdBodies[bodyIdx].m_bodyID, &shapeDef, &capsule );
                }
                else if ( shape.m_type == RagdollShapeDefinition::Type::Sphere )
                {
                    Vector const origin = shape.m_offsetTransform.GetTranslation();
                    b3Sphere sphere = { ToBox3D( origin ), shape.m_radius };
                    b3CreateSphereShape( m_createdBodies[bodyIdx].m_bodyID, &shapeDef, &sphere );
                }
                else if( shape.m_type == RagdollShapeDefinition::Type::Box )
                {
                    b3BoxHull box = b3MakeTransformedBoxHull( shape.m_boxExtents.m_x, shape.m_boxExtents.m_y, shape.m_boxExtents.m_z, ToBox3D( shape.m_offsetTransform ) );
                    b3CreateHullShape( m_createdBodies[bodyIdx].m_bodyID, &shapeDef, &box.base );
                }
            }
        }

        // Create joints
        //-------------------------------------------------------------------------

        for ( int32_t bodyIdx = 1; bodyIdx < numBodies; bodyIdx++ )
        {
            auto const& body = pDefinition->m_bodies[bodyIdx];

            if ( body.m_joint.m_type == RagdollJointDefinition::Type::Revolute )
            {
                b3RevoluteJointDef jointDef = b3DefaultRevoluteJointDef();
                jointDef.base.bodyIdA = m_createdBodies[body.m_parentBodyIdx].m_bodyID;
                jointDef.base.bodyIdB = m_createdBodies[bodyIdx].m_bodyID;
                jointDef.base.localFrameA = ToBox3D( body.m_joint.m_frameA );
                jointDef.base.localFrameB = ToBox3D( body.m_joint.m_frameB );

                jointDef.enableLimit = body.m_joint.m_enableHingeLimit;
                jointDef.lowerAngle = Math::DegreesToRadians * body.m_joint.m_hingeLowerLimit.ToFloat();
                jointDef.upperAngle = Math::DegreesToRadians * body.m_joint.m_hingeUpperLimit.ToFloat();

                jointDef.enableSpring = body.m_joint.m_enableSpring;
                jointDef.hertz = body.m_joint.m_springStiffnessHertz;
                jointDef.dampingRatio = body.m_joint.m_dampingRatio;

                jointDef.enableMotor = body.m_joint.m_enableMotor;
                jointDef.maxMotorTorque = body.m_joint.m_maxMotorTorque;

                m_createdBodies[bodyIdx].m_jointID = b3CreateRevoluteJoint( m_pWorld->GetWorldID(), &jointDef );
            }
            else if ( body.m_joint.m_type == RagdollJointDefinition::Type::Spherical )
            {
                b3SphericalJointDef jointDef = b3DefaultSphericalJointDef();
                jointDef.base.bodyIdA = m_createdBodies[body.m_parentBodyIdx].m_bodyID;
                jointDef.base.bodyIdB = m_createdBodies[bodyIdx].m_bodyID;
                jointDef.base.localFrameA = ToBox3D( body.m_joint.m_frameA );
                jointDef.base.localFrameB = ToBox3D( body.m_joint.m_frameB );

                jointDef.enableConeLimit = body.m_joint.m_enableConeLimit;
                jointDef.coneAngle = Math::DegreesToRadians * body.m_joint.m_coneAngle.ToFloat();

                jointDef.enableTwistLimit = body.m_joint.m_enableTwistLimit;
                jointDef.lowerTwistAngle = Math::DegreesToRadians * body.m_joint.m_twistLowerLimit.ToFloat();
                jointDef.upperTwistAngle = Math::DegreesToRadians * body.m_joint.m_twistUpperLimit.ToFloat();

                jointDef.enableSpring = body.m_joint.m_enableSpring;
                jointDef.hertz = body.m_joint.m_springStiffnessHertz;
                jointDef.dampingRatio = body.m_joint.m_dampingRatio;

                jointDef.enableMotor = body.m_joint.m_enableMotor;
                jointDef.maxMotorTorque = body.m_joint.m_maxMotorTorque;

                m_createdBodies[bodyIdx].m_jointID = b3CreateSphericalJoint( m_pWorld->GetWorldID(), &jointDef );
            }
        }

        // Disable collisions
        //-------------------------------------------------------------------------

        for ( auto const& rule : pDefinition->m_disableCollisionRules )
        {
            b3FilterJointDef jointDef = b3DefaultFilterJointDef();
            jointDef.base.bodyIdA = m_createdBodies[rule.m_bodyIdxA].m_bodyID;
            jointDef.base.bodyIdB = m_createdBodies[rule.m_bodyIdxB].m_bodyID;

            m_createdFilterJoints.emplace_back( b3CreateFilterJoint( m_pWorld->GetWorldID(), &jointDef ) );
        }
    }

    Ragdoll::~Ragdoll()
    {
        for ( b3JointId jointID : m_createdFilterJoints )
        {
            b3DestroyJoint( jointID, false );
        }

        for ( auto i = int32_t( m_createdBodies.size() ) - 1; i >= 0; i-- )
        {
            if ( B3_IS_NON_NULL( m_createdBodies[i].m_jointID ) )
            {
                b3DestroyJoint( m_createdBodies[i].m_jointID, false );
            }

            b3DestroyBody( m_createdBodies[i].m_bodyID );
        }
    }

    //-------------------------------------------------------------------------

    bool Ragdoll::IsSleeping() const
    {
        EE_ASSERT( IsValid() );
        ScopedReadLock const sl( m_pWorld );

        for ( auto const& body : m_createdBodies )
        {
            if ( b3Body_IsAwake( body.m_bodyID ) )
            {
                return false;
            }
        }

        return true;
    }

    void Ragdoll::PutToSleep()
    {
         EE_ASSERT( IsValid() );
         ScopedWriteLock const sl( m_pWorld );
         for ( auto const& body : m_createdBodies )
         {
             b3Body_SetAwake( body.m_bodyID, false );
         }
    }

    void Ragdoll::WakeUp()
    {
        EE_ASSERT( IsValid() );
        ScopedWriteLock const sl( m_pWorld );
        for ( auto const& body : m_createdBodies )
        {
            b3Body_SetAwake( body.m_bodyID, true );
        }
    }

    void Ragdoll::SetGravityScale( float gravityScale )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( gravityScale >= 0.0f && gravityScale <= 1.0f );

        if ( m_gravityScale == gravityScale )
        {
            return;
        }

        m_gravityScale = gravityScale;

        //-------------------------------------------------------------------------

        ScopedWriteLock const sl( m_pWorld );
        for ( auto& createdBody : m_createdBodies )
        {
            b3Body_SetGravityScale( createdBody.m_bodyID, m_gravityScale );
        }
    }

    void Ragdoll::SetPoseFollowing( bool shouldFollowPose )
    {
        if ( m_shouldFollowPose == shouldFollowPose )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_shouldFollowPose = shouldFollowPose;
    }

    //-------------------------------------------------------------------------

    b3BodyCastResult Ragdoll::RayCastAgainstBodies( Vector const& rayStart, Vector const& rayEnd ) const
    {
        b3BodyCastResult bestResult = {};
        float fraction = FLT_MAX;

        ScopedReadLock const sl( m_pWorld );

        for ( CreatedBody const& cb : m_createdBodies )
        {
            b3Transform const bodyTransform = b3Body_GetTransform( cb.m_bodyID );
            b3BodyCastResult result = b3Body_CastRay( cb.m_bodyID, ToBox3D( rayStart ), ToBox3D( rayEnd - rayStart ), b3DefaultQueryFilter(), 1.0f, bodyTransform );
            if ( result.hit && result.fraction < fraction )
            {
                fraction = result.fraction;
                bestResult = result;
            }
        }

        return bestResult;
    }

    void Ragdoll::ApplyImpulse( Vector const& impulseOriginWS, Vector const& impulseForceWS )
    {
        b3BodyCastResult result = RayCastAgainstBodies( impulseOriginWS, impulseOriginWS + ( impulseForceWS.GetNormalized3() * 1000 ) );
        if( result.hit )
        {
            ScopedWriteLock const sl( m_pWorld );
            b3Body_ApplyLinearImpulse( b3Shape_GetBody( result.shapeId ), ToBox3D( impulseForceWS ), result.point, true );
        }
    }

    void Ragdoll::ApplyImpulseToBody( int32_t bodyIdx, Vector const& impulseOriginWS, Vector const& impulseForceWS )
    {
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_createdBodies.size() );

        ScopedWriteLock const sl( m_pWorld );
        b3BodyCastResult result = b3Body_CastRay( m_createdBodies[bodyIdx].m_bodyID, ToBox3D( impulseOriginWS ), ToBox3D( impulseOriginWS + ( impulseForceWS.GetNormalized3() * 1000 ) ), b3DefaultQueryFilter(), 1.0f, b3Body_GetTransform( m_createdBodies[bodyIdx].m_bodyID ) );
        if ( result.hit )
        {
            b3Body_ApplyLinearImpulse( m_createdBodies[bodyIdx].m_bodyID, ToBox3D( impulseForceWS ), result.point, true );
        }
    }

    void Ragdoll::ApplyImpulseToBody( StringID boneID, Vector const& impulseOriginWS, Vector const& impulseForceWS )
    {
        int32_t boneIdx = m_pDefinition->m_skeleton->GetBoneIndex( boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        // Body index can be invalid since we dont always have a body per bone
        int32_t bodyIdx = m_pDefinition->m_boneToBodyMap[boneIdx];
        if ( bodyIdx != InvalidIndex )
        {
            ApplyImpulseToBody( bodyIdx, impulseOriginWS, impulseForceWS );
        }
    }

    void Ragdoll::ApplyImpulseToBodyCOM( int32_t bodyIdx, Vector const& impulseForceWS )
    {
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_createdBodies.size() );
        ScopedWriteLock const sl( m_pWorld );
        b3Body_ApplyLinearImpulseToCenter( m_createdBodies[bodyIdx].m_bodyID, ToBox3D( impulseForceWS ), true );
    }

    void Ragdoll::ApplyImpulseToBodyCOM( StringID boneID, Vector const& impulseForceWS )
    {
        int32_t boneIdx = m_pDefinition->m_skeleton->GetBoneIndex( boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        // Body index can be invalid since we dont always have a body per bone
        int32_t bodyIdx = m_pDefinition->m_boneToBodyMap[boneIdx];
        if ( bodyIdx != InvalidIndex )
        {
            ApplyImpulseToBodyCOM( bodyIdx, impulseForceWS );
        }
    }

    //-------------------------------------------------------------------------

    void Ragdoll::Update( Seconds const deltaTime, Transform const& worldTransform, Animation::Pose* pPose, bool shouldInitializeBodies )
    {
        EE_ASSERT( IsValid() );

        #if EE_DEVELOPMENT_TOOLS
        m_lastUpdateWorldTransform = worldTransform;
        #endif

        //-------------------------------------------------------------------------

        if ( ( !m_shouldFollowPose && !shouldInitializeBodies ) && !m_pDefinition->m_hasKinematicBodies )
        {
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( pPose != nullptr );
        EE_ASSERT( pPose->GetSkeleton()->GetResourceID() == m_pDefinition->m_skeleton.GetResourceID() );
        EE_ASSERT( pPose->HasModelSpaceTransforms() );

        ScopedWriteLock const sl( m_pWorld );

        // Update bodies and joint Targets
        //-------------------------------------------------------------------------

        int32_t const numBodies = (int32_t) m_createdBodies.size();
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            auto const& bodyDefinition = m_pDefinition->m_bodies[bodyIdx];
            int32_t const boneIdx = m_pDefinition->m_bodyToBoneMap[bodyIdx];
            Transform const bodyWorldTransform = pPose->GetModelSpaceTransform( boneIdx ) * worldTransform;

            //-------------------------------------------------------------------------

            if ( shouldInitializeBodies )
            {
                if ( B3_IS_NON_NULL( m_createdBodies[bodyIdx].m_jointID ) )
                {
                    // disable spring for joint
                }

                b3Body_SetTransform( m_createdBodies[bodyIdx].m_bodyID, ToBox3D( bodyWorldTransform.GetTranslation() ), ToBox3D( bodyWorldTransform.GetRotation() ) );
            }

            if ( bodyDefinition.m_isKinematic )
            {
                b3Body_SetTargetTransform( m_createdBodies[bodyIdx].m_bodyID, ToBox3D( bodyWorldTransform ), deltaTime, true );
            }

            if( m_shouldFollowPose && B3_IS_NON_NULL( m_createdBodies[bodyIdx].m_jointID ) )
            {
                auto const& jointDefinition = m_pDefinition->m_bodies[bodyIdx].m_joint;
                int32_t const parentBoneIdx = m_pDefinition->m_bodyToBoneMap[bodyDefinition.m_parentBodyIdx];

                auto pSkeleton = pPose->GetSkeleton();

                // Get the desired rotation of the joint frame in body B relative to the joint frame in body A.
                Quaternion quatB = jointDefinition.m_frameB.GetRotation() * pPose->GetParentSpaceTransform( boneIdx ).GetRotation();
                int32_t parentBoneIndex = pSkeleton->GetParentBoneIndex( boneIdx );
                while ( parentBoneIndex != parentBoneIdx && parentBoneIndex != InvalidIndex )
                {
                    quatB = quatB * pSkeleton->GetParentSpaceReferencePose()[parentBoneIndex].GetRotation();
                    parentBoneIndex = pSkeleton->GetParentBoneIndex( parentBoneIndex );
                }

                Quaternion quatA = jointDefinition.m_frameA.GetRotation();
                Quaternion relQuat = Quaternion::Delta( quatA, quatB );
                relQuat.Normalize();

                // This is thread-safe because it doesn't wake the bodies
                SetAngularMotorTarget( m_createdBodies[bodyIdx].m_jointID, relQuat, jointDefinition.m_springStiffnessHertz );
            }
        }
    }

    bool Ragdoll::GetPose( Transform const& worldTransform, Animation::Pose* pPose ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( pPose->GetSkeleton()->GetResourceID() == m_pDefinition->m_skeleton.GetResourceID() );
        EE_ASSERT( pPose->HasModelSpaceTransforms() );

        // Get the ragdoll body transforms
        //-------------------------------------------------------------------------

        int32_t const numBodies = (int32_t) m_createdBodies.size();
        TInlineVector<Transform, 40> bodyTransforms;
        bodyTransforms.reserve( numBodies );

        {
            ScopedReadLock const sl( m_pWorld );
            for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
            {
                bodyTransforms.emplace_back( FromBox3D( b3Body_GetTransform( m_createdBodies[bodyIdx].m_bodyID ) ) );
            }
        }

        // Fill out model space transforms
        //-------------------------------------------------------------------------

        Animation::Skeleton const* pSkeleton = pPose->GetSkeleton();
        int32_t const numBones = pPose->GetNumBones();
        m_globalBoneTransforms.resize( numBones );
        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            int32_t const bodyIdx = m_pDefinition->m_boneToBodyMap[boneIdx];
            if ( bodyIdx != InvalidIndex )
            {
                Transform const boneWorldTranform = m_pDefinition->m_bodies[bodyIdx].m_inverseOffsetTransform * bodyTransforms[bodyIdx];
                m_globalBoneTransforms[boneIdx] = Transform::Delta( worldTransform, boneWorldTranform );
            }
            else
            {
                if ( boneIdx == 0 )
                {
                    m_globalBoneTransforms[boneIdx] = pPose->GetParentSpaceTransform( boneIdx );
                }
                else [[likely]]
                { 
                    int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( boneIdx );
                    EE_ASSERT( parentBoneIdx != InvalidIndex );
                    m_globalBoneTransforms[boneIdx] = pPose->GetParentSpaceTransform( boneIdx ) * m_globalBoneTransforms[parentBoneIdx];
                }
            }
        }

        // Calculate the local transforms and set back into the pose
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = m_pDefinition->m_skeleton->GetParentBoneIndex( i );
            if ( parentBoneIdx != InvalidIndex )
            {
                Transform const boneLocalTransform = Transform::Delta( m_globalBoneTransforms[parentBoneIdx], m_globalBoneTransforms[i] );
                pPose->SetTransform( i, boneLocalTransform );
            }
            else
            {
                pPose->SetTransform( i, m_globalBoneTransforms[i] );
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Ragdoll::DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform ) const
    {
        EE_ASSERT( IsValid() );

        // Get transforms
        //-------------------------------------------------------------------------

        int32_t const numBodies = m_pDefinition->GetNumBodies();
        TInlineVector<Transform, 40> pose;
        pose.resize( numBodies );

        {
            ScopedReadLock const sl( m_pWorld );
            for ( auto i = 0; i < numBodies; i++ )
            {
                pose[i] = FromBox3D( b3Body_GetTransform( m_createdBodies[i].m_bodyID ) );

                if ( !worldTransform.IsIdentity() )
                {
                    pose[i] = ( pose[i] * m_lastUpdateWorldTransform.GetInverse() ) * worldTransform;
                }
            }
        }

        // Draw Ragdoll Bodies
        //-------------------------------------------------------------------------

        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            auto const& body = m_pDefinition->m_bodies[bodyIdx];

            for ( auto const& shape : body.m_shapes )
            {
                shape.DrawDebug( ctx, pose[bodyIdx] );
            }

            //-------------------------------------------------------------------------

            if ( body.m_joint.m_type != RagdollJointDefinition::Type::None )
            {
                int32_t const parentBodyIdx = m_pDefinition->m_bodies[bodyIdx].m_parentBodyIdx;
                EE_ASSERT( parentBodyIdx != InvalidIndex );
                Transform const transformA = pose[parentBodyIdx];
                Transform const transformB = pose[bodyIdx];
                body.m_joint.DrawDebug( ctx, transformA, transformB );
            }
        }
    }
    #endif
}