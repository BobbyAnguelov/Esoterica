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
    class PxScene;
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

    struct EE_ENGINE_API RagdollBodyMaterialSettings : public IRegisteredType
    {
        EE_SERIALIZE( m_staticFriction, m_dynamicFriction, m_restitution, m_frictionCombineMode, m_restitutionCombineMode );
        EE_REGISTER_TYPE( RagdollBodyMaterialSettings );

    public:

        bool IsValid() const;

        // Ensures that all material settings are within the required limits. Returns true if any changes were made!
        bool CorrectSettingsToValidRanges();

    public:

        EE_EXPOSE float                                    m_staticFriction = PhysicsMaterial::DefaultStaticFriction;
        EE_EXPOSE float                                    m_dynamicFriction = PhysicsMaterial::DefaultDynamicFriction;
        EE_EXPOSE float                                    m_restitution = PhysicsMaterial::DefaultRestitution;
        EE_EXPOSE PhysicsCombineMode                       m_frictionCombineMode = PhysicsCombineMode::Average;
        EE_EXPOSE PhysicsCombineMode                       m_restitutionCombineMode = PhysicsCombineMode::Average;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollRootControlBodySettings : public IRegisteredType
    {
        EE_SERIALIZE( m_driveType, m_maxDistance, m_tolerance, m_stiffness, m_damping );
        EE_REGISTER_TYPE( RagdollRootControlBodySettings );

        enum DriveType : uint8_t
        {
            EE_REGISTER_ENUM

            None = 0,
            Distance = 1,
            Spring = 2
        };

    public:

        bool IsValid() const;

    public:

        EE_EXPOSE DriveType                                m_driveType = DriveType::None;
        EE_EXPOSE float                                    m_maxDistance = 0.05f;
        EE_EXPOSE float                                    m_tolerance = 0.01f;
        EE_EXPOSE float                                    m_stiffness = 0.0f;
        EE_EXPOSE float                                    m_damping = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollBodySettings : public IRegisteredType
    {
        EE_SERIALIZE( m_mass, m_enableCCD, m_enableSelfCollision );
        EE_REGISTER_TYPE( RagdollBodySettings );

        // Are all the settings valid
        bool IsValid() const;

        // Ensures that all body settings are within the required limits. Returns true if any changes were made!
        bool CorrectSettingsToValidRanges();

    public:

        EE_REGISTER float                                  m_mass = 1.0f;
        EE_REGISTER bool                                   m_enableCCD = false;
        EE_REGISTER bool                                   m_enableSelfCollision = false;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollJointSettings : public IRegisteredType
    {
        EE_SERIALIZE(  m_driveType, m_stiffness, m_damping, m_internalCompliance, m_externalCompliance,
                                m_twistLimitEnabled, m_twistLimitMin, m_twistLimitMax, m_twistLimitContactDistance,
                                m_swingLimitEnabled, m_swingLimitY, m_swingLimitZ, m_tangentialStiffness, m_tangentialDamping, m_swingLimitContactDistance );
        EE_REGISTER_TYPE( RagdollJointSettings );

        enum DriveType : uint8_t
        {
            EE_REGISTER_ENUM

            Kinematic = 0,
            Spring = 1,
            Velocity = 2,
        };

    public:

        // Are all the settings valid
        bool IsValid() const;

        // Ensures that all joint settings are within the required limits. Returns true if any changes were made!
        bool CorrectSettingsToValidRanges();

        // Reset all settings to default
        void Reset() { *this = RagdollJointSettings(); }

    public:

        EE_REGISTER DriveType                                m_driveType = DriveType::Kinematic;
        EE_REGISTER bool                                     m_twistLimitEnabled = false;
        EE_REGISTER bool                                     m_swingLimitEnabled = false;

        // General
        EE_REGISTER float                                    m_stiffness = 0.0f;
        EE_REGISTER float                                    m_damping = 0.0f;
        EE_REGISTER float                                    m_internalCompliance = 0.01f;
        EE_REGISTER float                                    m_externalCompliance = 0.01f;

        // Twist
        EE_REGISTER float                                    m_twistLimitMin = -45;
        EE_REGISTER float                                    m_twistLimitMax = 45;
        EE_REGISTER float                                    m_twistLimitContactDistance = 3.0f;

        // Swing
        EE_REGISTER float                                    m_swingLimitY = 45.0f;
        EE_REGISTER float                                    m_swingLimitZ = 45.0f;
        EE_REGISTER float                                    m_tangentialStiffness = 0.0f; // Only for swing limit
        EE_REGISTER float                                    m_tangentialDamping = 0.0f; // Only for swing limit
        EE_REGISTER float                                    m_swingLimitContactDistance = 3.0f;
    };

    //-------------------------------------------------------------------------
    // Ragdoll Definition
    //-------------------------------------------------------------------------
    // This defines a ragdoll: The bodies and joints as well as the various joint profiles
    // Note: The root body control concept is an additional copy of the root body of the articulation 
    //       which is used to help drive the articulation relative to input pose.

    struct EE_ENGINE_API RagdollDefinition : public Resource::IResource, public IRegisteredType
    {
        EE_SERIALIZE( m_skeleton, m_bodies, m_profiles );
        EE_REGISTER_TYPE_RESOURCE( 'rgdl', "Physics Ragdoll", RagdollDefinition );

    public:

        constexpr static uint32_t const s_maxNumBodies = 64;

        struct EE_ENGINE_API BodyDefinition : public IRegisteredType
        {
            EE_SERIALIZE( m_boneID, m_offsetTransform, m_radius, m_halfHeight, m_jointTransform );
            EE_REGISTER_TYPE( BodyDefinition );

            EE_REGISTER StringID                            m_boneID;
            int32_t                                         m_parentBodyIdx = InvalidIndex;
            EE_EXPOSE float                                 m_radius = 0.075f;
            EE_EXPOSE float                                 m_halfHeight = 0.025f;
            EE_EXPOSE Transform                             m_offsetTransform;
            Transform                                       m_initialGlobalTransform;
            Transform                                       m_inverseOffsetTransform;
            EE_EXPOSE Transform                             m_jointTransform; // Global joint transform
            Transform                                       m_bodyRelativeJointTransform; // The joint transform relative to the current body
            Transform                                       m_parentRelativeJointTransform; // The joint transform relative to the parent body
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API Profile : public IRegisteredType
        {
            EE_SERIALIZE
            (
                m_ID, m_rootControlBodySettings, m_bodySettings, m_jointSettings, m_materialSettings,
                m_solverPositionIterations, m_solverVelocityIterations, m_maxProjectionIterations, m_internalDriveIterations, m_externalDriveIterations, m_separationTolerance, m_stabilizationThreshold, m_sleepThreshold
            );
            EE_REGISTER_TYPE( Profile );

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

        public:

            EE_REGISTER StringID                                m_ID = StringID( s_defaultProfileID );
            EE_REGISTER RagdollRootControlBodySettings          m_rootControlBodySettings;
            EE_REGISTER TVector<RagdollBodySettings>            m_bodySettings;
            EE_REGISTER TVector<RagdollJointSettings>           m_jointSettings;
            EE_REGISTER TVector<RagdollBodyMaterialSettings>    m_materialSettings;

            // Solver Settings
            EE_EXPOSE uint32_t                                  m_solverPositionIterations = 4;
            EE_EXPOSE uint32_t                                  m_solverVelocityIterations = 4;
            EE_EXPOSE uint32_t                                  m_maxProjectionIterations = 4;
            EE_EXPOSE uint32_t                                  m_internalDriveIterations = 4;
            EE_EXPOSE uint32_t                                  m_externalDriveIterations = 4;
            EE_EXPOSE float                                     m_separationTolerance = 0.1f;
            EE_EXPOSE float                                     m_stabilizationThreshold;
            EE_EXPOSE float                                     m_sleepThreshold = 1;

            TVector<uint64_t>                                   m_selfCollisionRules;
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

        EE_REGISTER TResourcePtr<Animation::Skeleton>           m_skeleton;
        EE_REGISTER TVector<BodyDefinition>                     m_bodies;
        EE_REGISTER TVector<Profile>                            m_profiles;

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
        friend class PhysicsWorldSystem;

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
        Ragdoll( physx::PxPhysics* pPhysics, RagdollDefinition const* pDefinition, StringID const profileID = StringID(), uint64_t userID = 0 );
        ~Ragdoll();

        bool IsValid() const { return m_pArticulation != nullptr; }

        void AddToScene( physx::PxScene* pScene );
        void RemoveFromScene();

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

        // Update / Results
        //-------------------------------------------------------------------------

        void Update( Seconds const deltaTime, Transform const& worldTransform, Animation::Pose* pPose = nullptr, bool initializeBodies = false );
        bool GetPose( Transform const& worldTransform, Animation::Pose* pPose ) const;
        void GetRagdollPose( RagdollPose& pose ) const;

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void RefreshSettings();
        void RecreateRootControlBody();
        void ResetState();
        void DrawDebug( Drawing::DrawContext& ctx ) const;
        #endif

    private:

        void TryCreateRootControlBody();
        void DestroyRootControlBody();

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
        physx::PxArticulation*                                  m_pArticulation = nullptr;
        uint64_t                                                m_userID = 0;
        TVector<physx::PxArticulationLink*>                     m_links;

        physx::PxRigidDynamic*                                  m_pRootControlActor = nullptr;
        physx::PxDistanceJoint*                                 m_pRootTargetJoint = nullptr;

        bool                                                    m_shouldFollowPose = false;
        bool                                                    m_gravityEnabled = true;
        mutable TVector<Transform>                              m_globalBoneTransforms;

        #if EE_DEVELOPMENT_TOOLS
        TInlineString<100>                                      m_ragdollName;
        #endif
    };
}