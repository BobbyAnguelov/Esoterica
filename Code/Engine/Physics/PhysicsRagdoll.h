#pragma once

#include "PhysicsMaterial.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "System/Resource/ResourcePtr.h"
#include "System/Math/Transform.h"
#include "System/Math/NumericRange.h"
#include "System/Types/Arrays.h"
#include "System/Types/StringID.h"
#include "System/Time/Time.h"

//-------------------------------------------------------------------------

namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxArticulationReducedCoordinate;
    class PxArticulationLink;
    class PxDistanceJoint;
    class PxRigidDynamic;
}

namespace EE::Animation
{
    class Pose;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Ragdoll Settings
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollBodyMaterialSettings : public IReflectedType
    {
        EE_SERIALIZE( m_staticFriction, m_dynamicFriction, m_restitution, m_frictionCombineMode, m_restitutionCombineMode );
        EE_REFLECT_TYPE( RagdollBodyMaterialSettings );

    public:

        bool IsValid() const;

        // Ensures that all material settings are within the required limits. Returns true if any changes were made!
        bool CorrectSettingsToValidRanges();

    public:

        EE_REFLECT() float                             m_staticFriction = MaterialSettings::s_defaultStaticFriction;
        EE_REFLECT() float                             m_dynamicFriction = MaterialSettings::s_defaultDynamicFriction;
        EE_REFLECT() float                             m_restitution = MaterialSettings::s_defaultRestitution;
        EE_REFLECT() CombineMode                       m_frictionCombineMode = CombineMode::Average;
        EE_REFLECT() CombineMode                       m_restitutionCombineMode = CombineMode::Average;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollBodySettings : public IReflectedType
    {
        EE_SERIALIZE( m_mass, m_enableCCD, m_enableSelfCollision, m_followPose );
        EE_REFLECT_TYPE( RagdollBodySettings );

        // Are all the settings valid
        bool IsValid() const;

        // Ensures that all body settings are within the required limits. Returns true if any changes were made!
        bool CorrectSettingsToValidRanges();

    public:

        EE_REFLECT() float                                  m_mass = 1.0f;
        EE_REFLECT() bool                                   m_enableCCD = false;
        EE_REFLECT() bool                                   m_enableSelfCollision = false;
        EE_REFLECT() bool                                   m_followPose = false; // Should this body be externally driven towards to the pose
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollJointSettings : public IReflectedType
    {
        EE_SERIALIZE(   m_useVelocity, m_twistLimitEnabled, m_swingLimitEnabled,
                        m_stiffness, m_damping, m_internalCompliance, m_externalCompliance,
                        m_twistLimitMin, m_twistLimitMax, m_twistLimitContactDistance,
                        m_swingLimitY, m_swingLimitZ, m_tangentialStiffness, m_tangentialDamping, m_swingLimitContactDistance );

        EE_REFLECT_TYPE( RagdollJointSettings );

    public:

        // Are all the settings valid
        bool IsValid() const;

        // Ensures that all joint settings are within the required limits. Returns true if any changes were made!
        bool CorrectSettingsToValidRanges();

        // Reset all settings to default
        void Reset() { *this = RagdollJointSettings(); }

    public:

        EE_REFLECT() bool                                     m_useVelocity = true;
        EE_REFLECT() bool                                     m_twistLimitEnabled = false;
        EE_REFLECT() bool                                     m_swingLimitEnabled = false;

        // General
        EE_REFLECT() float                                    m_stiffness = 0.0f;
        EE_REFLECT() float                                    m_damping = 0.0f;
        EE_REFLECT() float                                    m_internalCompliance = 0.01f;
        EE_REFLECT() float                                    m_externalCompliance = 0.01f;

        // Twist
        EE_REFLECT() float                                    m_twistLimitMin = -45;
        EE_REFLECT() float                                    m_twistLimitMax = 45;
        EE_REFLECT() float                                    m_twistLimitContactDistance = 3.0f;

        // Swing
        EE_REFLECT() float                                    m_swingLimitY = 45.0f;
        EE_REFLECT() float                                    m_swingLimitZ = 45.0f;
        EE_REFLECT() float                                    m_tangentialStiffness = 0.0f; // Only for swing limit
        EE_REFLECT() float                                    m_tangentialDamping = 0.0f; // Only for swing limit
        EE_REFLECT() float                                    m_swingLimitContactDistance = 3.0f;
    };

    //-------------------------------------------------------------------------
    // Ragdoll Definition
    //-------------------------------------------------------------------------
    // This defines a ragdoll: The bodies and joints as well as the various joint profiles
    // Note: The root body control concept is an additional copy of the root body of the articulation 
    //       which is used to help drive the articulation relative to input pose.

    struct EE_ENGINE_API RagdollDefinition : public Resource::IResource, public IReflectedType
    {
        EE_SERIALIZE( m_skeleton, m_bodies, m_profiles );
        EE_REFLECTED_RESOURCE( 'rgdl', "Physics Ragdoll", RagdollDefinition );

    public:

        constexpr static uint32_t const s_maxNumBodies = 64;

        struct EE_ENGINE_API BodyDefinition : public IReflectedType
        {
            EE_SERIALIZE( m_boneID, m_offsetTransform, m_radius, m_halfHeight, m_jointTransform );
            EE_REFLECT_TYPE( BodyDefinition );

            EE_REFLECT() StringID                           m_boneID;
            int32_t                                         m_parentBodyIdx = InvalidIndex;
            EE_REFLECT() float                              m_radius = 0.075f;
            EE_REFLECT() float                              m_halfHeight = 0.025f;
            EE_REFLECT() Transform                          m_offsetTransform;
            Transform                                       m_initialGlobalTransform;
            Transform                                       m_inverseOffsetTransform;
            EE_REFLECT() Transform                          m_jointTransform; // Global joint transform
            Transform                                       m_bodyRelativeJointTransform; // The joint transform relative to the current body
            Transform                                       m_parentRelativeJointTransform; // The joint transform relative to the parent body
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API Profile : public IReflectedType
        {
            EE_SERIALIZE
            (
                m_ID, m_bodySettings, m_jointSettings, m_materialSettings,
                m_solverPositionIterations, m_solverVelocityIterations, m_maxProjectionIterations, m_internalDriveIterations, m_externalDriveIterations, m_separationTolerance, m_stabilizationThreshold, m_sleepThreshold
            );
            EE_REFLECT_TYPE( Profile );

            constexpr static char const* const s_defaultProfileID = "Default";

        public:

            Profile();

            bool IsValid( int32_t numBodies ) const;

            // Ensures that all body settings are within the required limits. Returns true if any changes were made!
            bool CorrectSettingsToValidRanges();

            // Checks whether two bodies in the same ragdoll should collide with one another
            inline bool ShouldBodiesCollides( int32_t bodyIdx0, int32_t bodyIdx1 ) const
            {
                EE_ASSERT( bodyIdx0 >= 0 && bodyIdx1 >= 0 );
                EE_ASSERT( bodyIdx0 < m_selfCollisionRules.size() && bodyIdx1 < m_selfCollisionRules.size() );
                uint64_t const result = m_selfCollisionRules[bodyIdx0] & ( 1ULL << bodyIdx1 );
                return result != 0;
            }

            // Copy ragdoll settings
            inline void CopySettingsFrom( Profile const& other )
            {
                m_bodySettings = other.m_bodySettings;
                m_jointSettings = other.m_jointSettings;
                m_materialSettings = other.m_materialSettings;

                m_solverPositionIterations = other.m_solverPositionIterations;
                m_solverVelocityIterations = other.m_solverVelocityIterations;
                m_maxProjectionIterations = other.m_maxProjectionIterations;
                m_internalDriveIterations = other.m_internalDriveIterations;
                m_externalDriveIterations = other.m_externalDriveIterations;
                m_separationTolerance = other.m_separationTolerance;
                m_stabilizationThreshold = other.m_stabilizationThreshold;
                m_sleepThreshold = other.m_sleepThreshold;

                m_selfCollisionRules = other.m_selfCollisionRules;
            }

        public:

            EE_REFLECT() StringID                                   m_ID = StringID( s_defaultProfileID );
            EE_REFLECT() TVector<RagdollBodySettings>               m_bodySettings;
            EE_REFLECT() TVector<RagdollJointSettings>              m_jointSettings;
            EE_REFLECT() TVector<RagdollBodyMaterialSettings>       m_materialSettings;

            // Solver Settings
            EE_REFLECT() uint32_t                                   m_solverPositionIterations = 4;
            EE_REFLECT() uint32_t                                   m_solverVelocityIterations = 4;
            EE_REFLECT() uint32_t                                   m_maxProjectionIterations = 4;
            EE_REFLECT() uint32_t                                   m_internalDriveIterations = 4;
            EE_REFLECT() uint32_t                                   m_externalDriveIterations = 4;
            EE_REFLECT() float                                      m_separationTolerance = 0.1f;
            EE_REFLECT() float                                      m_stabilizationThreshold;
            EE_REFLECT() float                                      m_sleepThreshold = 1;

            TVector<uint64_t>                                       m_selfCollisionRules;
        };

    public:

        virtual bool IsValid() const override;

        // Creates all the necessary additional runtime data needed to instantiate this definition (i.e. bone mappings, etc...)
        void CreateRuntimeData();

        // Bodies + Joints
        //-------------------------------------------------------------------------

        int32_t GetNumBodies() const { return (int32_t) m_bodies.size(); }
        int32_t GetNumJoints() const { return (int32_t) m_bodies.size() - 1; }
        int32_t GetBodyIndexForBoneID( StringID boneID ) const;
        int32_t GetBodyIndexForBoneIdx( int32_t boneIdx ) const;

        // Profiles
        //-------------------------------------------------------------------------

        inline int32_t GetNumProfiles() const { return (int32_t) m_profiles.size(); }
        Profile const* GetDefaultProfile() const { return &m_profiles[0]; }
        Profile* GetDefaultProfile() { return &m_profiles[0]; }
        Profile* GetProfile( StringID profileID );
        Profile const* GetProfile( StringID profileID ) const { return const_cast<RagdollDefinition*>( this )->GetProfile( profileID ); }
        bool HasProfile( StringID profileID ) const { return GetProfile( profileID ) != nullptr; }

    public:

        EE_REFLECT() TResourcePtr<Animation::Skeleton>          m_skeleton;
        EE_REFLECT() TVector<BodyDefinition>                    m_bodies;
        EE_REFLECT() TVector<Profile>                           m_profiles;

        // Runtime Data
        TVector<int32_t>                                        m_boneToBodyMap;
        TVector<int32_t>                                        m_bodyToBoneMap;
    };

    //-------------------------------------------------------------------------
    // Ragdoll Instance
    //-------------------------------------------------------------------------
    // By default ragdolls with the same userID will not collide together
    // Ragdoll self-collision is user specified in the definition

    using RagdollPose = TInlineVector<Transform, 40>;

    class EE_ENGINE_API Ragdoll
    {
        friend class PhysicsWorld;

        class [[nodiscard]] ScopedWriteLock
        {
        public:

            ScopedWriteLock( Ragdoll* pRagdoll )
                : m_pRagdoll( pRagdoll )
            {
                EE_ASSERT( pRagdoll != nullptr );
                m_pRagdoll->LockWriteScene();
            }

            ~ScopedWriteLock()
            {
                m_pRagdoll->UnlockWriteScene();
            }

        private:

            Ragdoll*  m_pRagdoll = nullptr;
        };

        class [[nodiscard]] ScopedReadLock
        {
        public:

            ScopedReadLock( Ragdoll const* pRagdoll )
                : m_pRagdoll( pRagdoll )
            {
                EE_ASSERT( pRagdoll != nullptr );
                m_pRagdoll->LockReadScene();
            }

            ~ScopedReadLock()
            {
                m_pRagdoll->UnlockReadScene();
            }

        private:

            Ragdoll const*  m_pRagdoll = nullptr;
        };

    public:

        // Create a new ragdoll instance. The userID is used to prevent collisions between the ragdolls on the same user
        Ragdoll( RagdollDefinition const* pDefinition, StringID const profileID = StringID(), uint64_t userID = 0 );
        ~Ragdoll();

        bool IsValid() const { return m_pArticulation != nullptr; }

        // Collision Rules
        //-------------------------------------------------------------------------

        inline uintptr_t GetUserID() const { return m_userID; }

        inline bool ShouldBodiesCollides( int32_t bodyIdx0, int32_t bodyIdx1 ) const
        {
            EE_ASSERT( IsValid() );
            return m_pProfile->ShouldBodiesCollides( bodyIdx0, bodyIdx1 );
        }

        // Ragdoll API
        //-------------------------------------------------------------------------

        void SwitchProfile( StringID newProfileID );

        void SetGravityEnabled( bool isGravityEnabled );
        void SetPoseFollowingEnabled( bool shouldFollowPose );

        bool IsSleeping() const;
        void PutToSleep();
        void WakeUp();

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
        void GetRagdollPose( RagdollPose& pose ) const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void RefreshSettings();
        void ResetState();
        void DrawDebug( Drawing::DrawContext& ctx ) const;
        #endif

    private:

        void AddToScene( physx::PxScene* pScene );
        void RemoveFromScene();

        void UpdateBodySettings();
        void UpdateSolverSettings();
        void UpdateJointSettings();

        void LockWriteScene();
        void UnlockWriteScene();
        void LockReadScene() const;
        void UnlockReadScene() const;

    private:

        physx::PxPhysics*                                       m_pPhysics = nullptr;
        RagdollDefinition const*                                m_pDefinition = nullptr;
        RagdollDefinition::Profile const*                       m_pProfile = nullptr;
        physx::PxArticulationReducedCoordinate*                 m_pArticulation = nullptr;
        uint64_t                                                m_userID = 0;
        TVector<physx::PxArticulationLink*>                     m_links;

        bool                                                    m_shouldFollowPose = false;
        bool                                                    m_gravityEnabled = true;
        mutable TVector<Transform>                              m_globalBoneTransforms;

        #if EE_DEVELOPMENT_TOOLS
        TInlineString<100>                                      m_ragdollName;
        #endif
    };
}