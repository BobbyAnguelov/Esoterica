#pragma once

#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsCollisionSettings.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Joints
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollJointDefinition : public IReflectedType
    {
        EE_REFLECT_TYPE( RagdollJointDefinition );
        EE_SERIALIZE( m_type, m_orientation, m_enableHingeLimit, m_hingeLowerLimit, m_hingeUpperLimit, m_enableTwistLimit, m_enableConeLimit, m_twistLowerLimit, m_twistUpperLimit, m_coneAngle, m_enableSpring, m_springStiffnessHertz, m_dampingRatio, m_enableMotor, m_maxMotorTorque );

    public:

        enum class Type : uint8_t
        {
            EE_REFLECT_ENUM

            None,
            Spherical,
            Revolute
        };

    public:

        bool IsValid( Log* pLog = nullptr ) const;

        void FinalizeJoint( Transform const& transformA, Transform const& transformB );

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx, Transform const& transformA, Transform const& transformB ) const;
        #endif

    public:

        EE_REFLECT();
        Type                                            m_type = Type::Spherical;

        // The orientation of the joint's reference axis (the twist/hinge axis)
        EE_REFLECT();
        Quaternion                                      m_orientation = Quaternion::Identity;

        EE_REFLECT( Category = "Revolute" );
        bool                                            m_enableHingeLimit = true;

        EE_REFLECT( Category = "Revolute", Min = "-180", Max = "180" );
        Degrees                                         m_hingeLowerLimit = -45.0f;

        EE_REFLECT( Category = "Revolute", Min = "-180", Max = "180" );
        Degrees                                         m_hingeUpperLimit = 45.0f;

        EE_REFLECT( Category = "Spherical" );
        bool                                            m_enableTwistLimit = true;

        EE_REFLECT( Category = "Spherical" );
        bool                                            m_enableConeLimit = true;

        EE_REFLECT( Category = "Spherical", Min = "-180", Max ="180" );
        Degrees                                         m_twistLowerLimit = -45.0f;

        EE_REFLECT( Category = "Spherical", Min = "-180", Max = "180" );
        Degrees                                         m_twistUpperLimit = 45.0f;

        EE_REFLECT( Category = "Spherical", Min = "0", Max = "180" );
        Degrees                                         m_coneAngle = 45.0f;

        EE_REFLECT( Category = "Spring" );
        bool                                            m_enableSpring = true;

        EE_REFLECT( Category = "Spring" );
        float                                           m_springStiffnessHertz = 1.0f;

        EE_REFLECT( Category = "Spring" );
        float                                           m_dampingRatio = 0.7f;

        EE_REFLECT( Category = "Motor" );
        bool                                            m_enableMotor = false;

        EE_REFLECT( Category = "Motor" );
        float                                           m_maxMotorTorque = 1.0f;

        // Runtime Only
        //-------------------------------------------------------------------------

        Transform                                       m_frameA;
        Transform                                       m_frameB;
    };

    // Shape
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollShapeDefinition : public IReflectedType
    {
        EE_REFLECT_TYPE( RagdollShapeDefinition );
        EE_SERIALIZE( m_type, m_offsetTransform, m_radius, m_halfHeight, m_boxExtents, m_density );

    public:

        enum class Type : uint8_t
        {
            EE_REFLECT_ENUM

            Capsule,
            Sphere,
            Box,
        };

    public:

        bool IsValid( Log* pLog = nullptr ) const;

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx, Transform const& bodyTransform, Color color = Colors::LightGreen, float thickness = 2.0f ) const;
        #endif

    public:

        EE_REFLECT();
        Type                                            m_type = Type::Capsule;

        EE_REFLECT();
        Transform                                       m_offsetTransform;

        EE_REFLECT( Min = "0.001" );
        float                                           m_radius = 0.05f;

        EE_REFLECT( Min = "0.001" );
        float                                           m_halfHeight = 0.1f;

        EE_REFLECT();
        Float3                                          m_boxExtents = Float3::One;

        EE_REFLECT( Min = "0.001" );
        float                                           m_density = Constants::BaseDensity;

        EE_REFLECT( Category = "Collision" )
        CollisionSettings                               m_collisionSettings;
    };

    // Body
    //-------------------------------------------------------------------------
    // Note: body transform is identical to the bone transform!

    struct EE_ENGINE_API RagdollBodyDefinition : public IReflectedType
    {
        EE_REFLECT_TYPE( RagdollBodyDefinition );
        EE_SERIALIZE( m_boneID, m_shapes, m_isKinematic, m_friction, m_restitution, m_rollingResistance, m_joint );

        bool IsValid( Log* pLog = nullptr ) const;

    public:

        EE_REFLECT( Category = "Body", Hidden );
        StringID                                        m_boneID;

        EE_REFLECT( Category = "Body", Hidden );
        TVector<RagdollShapeDefinition>                 m_shapes;

        EE_REFLECT( Category = "Body" );
        bool                                            m_isKinematic = false;

        // The static friction coefficients - [0, 1]
        EE_REFLECT( Category = "Body", Min = "0.0f", Max = "1.0f" );
        float                                           m_friction = 0.5f;

        // The amount of restitution (bounciness) - [0,1]
        EE_REFLECT( Category = "Body", Min = "0.0f", Max = "1.0f" );
        float                                           m_restitution = 0.5f;

        // The rolling resistance for spheres and capsules - [0, 1]
        EE_REFLECT( Category = "Body", Min = "0.0f", Max = "1.0f" );
        float                                           m_rollingResistance = 0.5f;

        EE_REFLECT( Category = "Joint", Hidden );
        RagdollJointDefinition                          m_joint;

        // Runtime Only
        //-------------------------------------------------------------------------

        int32_t                                         m_parentBodyIdx = InvalidIndex;
        Transform                                       m_initialGlobalTransform;
        Transform                                       m_inverseOffsetTransform;
         
        #if EE_DEVELOPMENT_TOOLS
        String                                          m_debugName;
        #endif
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API RagdollDisableCollisionRule : public IReflectedType
    {
        EE_REFLECT_TYPE( RagdollDisableCollisionRule );
        EE_SERIALIZE( m_boneA, m_boneB );

    public:

        bool IsValid() const;

        bool operator==( RagdollDisableCollisionRule const& rhs ) const;
        bool operator!=( RagdollDisableCollisionRule const& rhs ) const { return !operator==( rhs ); }

    public:

        EE_REFLECT();
        StringID    m_boneA;

        EE_REFLECT();
        StringID    m_boneB;

        // Runtime Only
        //-------------------------------------------------------------------------

        int32_t     m_bodyIdxA = InvalidIndex;
        int32_t     m_bodyIdxB = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Ragdoll Definition
    //-------------------------------------------------------------------------
    // This defines a ragdoll: The bodies and joints as well as the various joint profiles
    // Note: The root body control concept is an additional copy of the root body of the articulation 
    //       which is used to help drive the articulation relative to input pose.

    struct EE_ENGINE_API RagdollDefinition : public Resource::IResource
    {
        EE_RESOURCE( "ragdoll", "Physics Ragdoll", Colors::DeepSkyBlue, 12, false );
        EE_SERIALIZE( m_skeleton, m_bodies );

        friend class RagdollLoader;

    public:

        constexpr static uint32_t const s_maxNumBodies = 64;

    public:

        virtual bool IsValid() const override;

        bool PerformValidation( Log* pLog = nullptr ) const;

        // Bodies + Joints
        //-------------------------------------------------------------------------

        int32_t GetNumBodies() const { return (int32_t) m_bodies.size(); }
        int32_t GetNumJoints() const { return (int32_t) m_bodies.size() - 1; }
        int32_t GetBodyIndexForBoneID( StringID boneID ) const;
        int32_t GetBodyIndexForBoneIdx( int32_t boneIdx ) const;

    private:

        // Generate all runtime data needed to create instances of this definition
        void Finalize();

    public:

        TResourcePtr<Animation::Skeleton>                       m_skeleton;
        TVector<RagdollBodyDefinition>                          m_bodies;
        TVector<RagdollDisableCollisionRule>                    m_disableCollisionRules;

        // Runtime Data
        TVector<int32_t>                                        m_boneToBodyMap;
        TVector<int32_t>                                        m_bodyToBoneMap;
        bool                                                    m_hasKinematicBodies;
    };
}