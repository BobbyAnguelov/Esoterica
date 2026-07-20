#pragma once
#include "PhysicsRagdoll_Definition.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Ragdoll Instance
    //-------------------------------------------------------------------------
    // By default ragdolls with the same userID will not collide together
    // Ragdoll self-collision is user specified in the definition

    class EE_ENGINE_API Ragdoll
    {
        struct CreatedBody
        {
            b3BodyId            m_bodyID = {};
            b3JointId           m_jointID = {};
            b3SurfaceMaterial   m_material = {};
        };

    public:

        // Create a new ragdoll instance. The userID is used to prevent collisions between the ragdolls on the same user
        Ragdoll( PhysicsWorld* pWorld, RagdollDefinition const* pDefinition, int32_t ragdollUserID = 0 );
        ~Ragdoll();

        bool IsValid() const { return !m_createdBodies.empty(); }

        // Ragdoll API
        //-------------------------------------------------------------------------

        void SetGravityScale( float gravityScale );

        void SetPoseFollowing( bool shouldFollowPose );

        bool IsSleeping() const;
        void PutToSleep();
        void WakeUp();

        // Cast a ray against all the ragdoll bodies
        b3BodyCastResult RayCastAgainstBodies( Vector const& rayStart, Vector const& rayEnd ) const;

        // Apply an impulse from a specific source and direction. Will raycast to find affected body and apply impulse to it directly
        void ApplyImpulse( Vector const& impulseOriginWS, Vector const& impulseForceWS );

        // Apply an impulse from a specific source and direction. Will raycast to find affected body and apply impulse to it directly
        inline void ApplyImpulse( Vector const& impulseOriginWS, Vector const& impulseDirWS, float strength )
        {
            EE_ASSERT( strength >= 0 );
            EE_ASSERT( impulseDirWS.IsNormalized3() );
            ApplyImpulse( impulseOriginWS, impulseDirWS * strength );
        }

        // Apply an impulse directly to a body - the impulse might not affect though
        void ApplyImpulseToBody( int32_t bodyIdx, Vector const& impulseOriginWS, Vector const& impulseForceWS );

        // Apply an impulse directly to a body - the impulse might not affect though
        void ApplyImpulseToBody( int32_t bodyIdx, Vector const& impulseOriginWS, Vector const& impulseDirWS, float strength )
        {
            EE_ASSERT( strength >= 0 );
            EE_ASSERT( impulseDirWS.IsNormalized3() );
            ApplyImpulse( impulseOriginWS, impulseDirWS * strength );
        }

        // Apply an impulse directly to a body - the impulse might not affect though
        void ApplyImpulseToBody( StringID boneID, Vector const& impulseOriginWS, Vector const& impulseForceWS );

        // Apply an impulse directly to a body - the impulse might not affect though
        inline void ApplyImpulseToBody( StringID boneID, Vector const& impulseOriginWS, Vector const& impulseDirWS, float strength )
        {
            EE_ASSERT( strength >= 0 );
            EE_ASSERT( impulseDirWS.IsNormalized3() );
            ApplyImpulseToBody( boneID, impulseOriginWS, impulseDirWS * strength );
        }

        // Apply an impulse directly to a body's COM
        void ApplyImpulseToBodyCOM( int32_t bodyIdx, Vector const& impulseForceWS );

        // Apply an impulse directly to a body's COM
        inline void ApplyImpulseToBodyCOM( int32_t bodyIdx, Vector const& impulseDirWS, float strength )
        {
            EE_ASSERT( strength >= 0 );
            EE_ASSERT( impulseDirWS.IsNormalized3() );
            ApplyImpulseToBodyCOM( bodyIdx, impulseDirWS * strength );
        }

        // Apply an impulse directly to a body's COM
        void ApplyImpulseToBodyCOM( StringID boneID, Vector const& impulseForceWS );

        // Apply an impulse directly to a body's COM
        inline void ApplyImpulseToBodyCOM( StringID boneID, Vector const& impulseDirWS, float strength )
        {
            EE_ASSERT( strength >= 0 );
            EE_ASSERT( impulseDirWS.IsNormalized3() );
            ApplyImpulseToBodyCOM( boneID, impulseDirWS * strength );
        }

        // Update / Results
        //-------------------------------------------------------------------------

        void Update( Seconds const deltaTime, Transform const& worldTransform, Animation::Pose* pPose = nullptr, bool initializeBodies = false );
        bool GetPose( Transform const& worldTransform, Animation::Pose* pPose ) const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform = Transform::Identity ) const;
        #endif

    private:

        PhysicsWorld*                                           m_pWorld = nullptr;
        RagdollDefinition const*                                m_pDefinition = nullptr;
        int32_t                                                 m_userID;

        // Be careful with this array as we feed ptrs into the created shapes
        // DO NOT remove elements or reorder elements!!!
        TInlineVector<CreatedBody, 30>                          m_createdBodies;
        TInlineVector<b3JointId, 10>                            m_createdFilterJoints;

        mutable TVector<Transform>                              m_globalBoneTransforms;
        float                                                   m_gravityScale = true;
        bool                                                    m_shouldFollowPose = false;

        #if EE_DEVELOPMENT_TOOLS
        Transform                                               m_lastUpdateWorldTransform;
        TInlineString<100>                                      m_ragdollName;
        #endif
    };
}