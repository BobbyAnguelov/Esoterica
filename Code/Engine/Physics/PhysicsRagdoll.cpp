#include "PhysicsRagdoll.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Math/MathHelpers.h"
#include "PhysX.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool RagdollBodyMaterialSettings::IsValid() const
    {
        if ( m_staticFriction < 0 )
        {
            return false;
        }

        if ( m_dynamicFriction < 0 )
        {
            return false;
        }

        if ( m_restitution < 0 || m_restitution > 1 )
        {
            return false;
        }

        return true;
    }

    bool RagdollBodyMaterialSettings::CorrectSettingsToValidRanges()
    {
        bool settingsUpdated = false;

        if ( m_staticFriction < 0 )
        {
            m_staticFriction = 0.0f;
            settingsUpdated = true;
        }

        if ( m_dynamicFriction < 0 )
        {
            m_dynamicFriction = 0.0f;
            settingsUpdated = true;
        }

        if ( m_restitution < 0 || m_restitution > 1 )
        {
            m_restitution = 0.0f;
            settingsUpdated = true;
        }

        return settingsUpdated;
    }

    //-------------------------------------------------------------------------

    bool RagdollRootControlBodySettings::IsValid() const
    {
        return true;
    }

    //-------------------------------------------------------------------------

    bool RagdollBodySettings::IsValid() const
    {
        if ( m_mass <= 0.0f )
        {
            return false;
        }

        return true;
    }

    bool RagdollBodySettings::CorrectSettingsToValidRanges()
    {
        bool updated = false;

        if ( m_mass < 0 )
        {
            m_mass = 1.0f;
            updated = true;
        }

        return updated;
    }

    //-------------------------------------------------------------------------

    bool RagdollJointSettings::IsValid() const
    {
        if ( m_stiffness < 0 )
        {
            return false;
        }

        if ( m_damping < 0 )
        {
            return false;
        }

        if ( m_internalCompliance < Math::HugeEpsilon || m_internalCompliance > 1 )
        {
            return false;
        }

        if ( m_externalCompliance < Math::HugeEpsilon || m_externalCompliance > 1 )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( m_twistLimitMin < -180 || m_twistLimitMin > 180 )
        {
            return false;
        }

        if ( m_twistLimitMax < -180 || m_twistLimitMax > 180 )
        {
            return false;
        }

        if ( m_twistLimitMin >= m_twistLimitMax )
        {
            return false;
        }

        if ( m_twistLimitContactDistance < 0 || m_twistLimitContactDistance > 180 )
        {
            return false;
        }

        // The contact distance should be less than half the distance between the upper and lower limits.
        if ( m_twistLimitContactDistance > ( ( m_twistLimitMax - m_twistLimitMin ) / 2 ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( m_swingLimitY < 1 || m_swingLimitY > 180 )
        {
            return false;
        }

        if ( m_swingLimitZ < 1 || m_swingLimitZ > 180 )
        {
            return false;
        }

        if ( m_tangentialStiffness < 0 )
        {
            return false;
        }

        if ( m_tangentialDamping < 0 )
        {
            return false;
        }

        if ( m_swingLimitContactDistance < 0 || m_swingLimitContactDistance > 180 )
        {
            return false;
        }

        // The contact distance should be less than either limit angle
        if ( m_swingLimitContactDistance > m_swingLimitY || m_swingLimitContactDistance > m_swingLimitZ )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool RagdollJointSettings::CorrectSettingsToValidRanges()
    {
        bool settingsUpdated = false;

        //-------------------------------------------------------------------------

        if ( m_stiffness < 0 )
        {
            m_stiffness = 0.0f;
            settingsUpdated = true;
        }

        if ( m_damping < 0 )
        {
            m_damping = 0.0f;
            settingsUpdated = true;
        }

        if ( m_internalCompliance < Math::HugeEpsilon || m_internalCompliance > 1 )
        {
            m_internalCompliance = Math::HugeEpsilon;
            settingsUpdated = true;
        }

        if (  m_internalCompliance > 1 )
        {
            m_internalCompliance = 1.0f;
            settingsUpdated = true;
        }

        if ( m_externalCompliance < Math::HugeEpsilon || m_externalCompliance > 1 )
        {
            m_externalCompliance = Math::HugeEpsilon;
            settingsUpdated = true;
        }

        if ( m_externalCompliance > 1 )
        {
            m_externalCompliance = 1.0f;
            settingsUpdated = true;
        }

        //-------------------------------------------------------------------------

        if ( m_twistLimitMin < 1 )
        {
            m_twistLimitMin = 1;
            settingsUpdated = true;
        }

        if ( m_twistLimitMin > 180 )
        {
            m_twistLimitMin = 180;
            settingsUpdated = true;
        }

        if ( m_twistLimitMax < 1 )
        {
            m_twistLimitMax = 1;
            settingsUpdated = true;
        }

        if ( m_twistLimitMax > 180 )
        {
            m_twistLimitMax = 180;
            settingsUpdated = true;
        }

        if ( m_twistLimitMin >= m_twistLimitMax )
        {
            // Try to keep the max limit fixed and ensure at least 2 degrees of limit
            if ( m_twistLimitMax > 2 )
            {
                m_twistLimitMin = m_twistLimitMax - 2;
            }
            else
            {
                m_twistLimitMin = 1;
                m_twistLimitMax = 3;
            }

            settingsUpdated = true;
        }

        if ( m_twistLimitContactDistance < 0 )
        {
            m_twistLimitContactDistance = 0;
            settingsUpdated = true;
        }

        if ( m_twistLimitContactDistance > 180 )
        {
            m_twistLimitContactDistance = 180;
            settingsUpdated = true;
        }

        // The contact distance should be less than half the distance between the upper and lower limits.
        float const maxTwistContactDistance = ( m_twistLimitMax - m_twistLimitMin ) / 2;
        if ( m_twistLimitContactDistance >= maxTwistContactDistance )
        {
            m_twistLimitContactDistance = maxTwistContactDistance - 0.5f;
            settingsUpdated = true;
        }

        //-------------------------------------------------------------------------

        if ( m_swingLimitY < 1 )
        {
            m_swingLimitY = 1;
            settingsUpdated = true;
        }

        if( m_swingLimitY > 180 )
        {
            m_swingLimitY = 180;
            settingsUpdated = true;
        }

        if ( m_swingLimitZ < 1 )
        {
            m_swingLimitZ = 1;
            settingsUpdated = true;
        }

        if ( m_swingLimitZ > 180 )
        {
            m_swingLimitZ = 180;
            settingsUpdated = true;
        }

        if ( m_tangentialStiffness < 0 )
        {
            m_tangentialStiffness = 0.0f;
            settingsUpdated = true;
        }

        if ( m_tangentialDamping < 0 )
        {
            m_tangentialDamping = 0.0f;
            settingsUpdated = true;
        }

        if ( m_swingLimitContactDistance < 0 )
        {
            m_swingLimitContactDistance = 0;
            settingsUpdated = true;
        }

        if ( m_swingLimitContactDistance > 180 )
        {
            m_swingLimitContactDistance = 180;
            settingsUpdated = true;
        }

        // The contact distance should be less than either limit angle
        float const maxSwingContactDistance = Math::Min( m_swingLimitY, m_swingLimitZ );
        if ( m_swingLimitContactDistance >= maxSwingContactDistance )
        {
            m_swingLimitContactDistance = maxSwingContactDistance - 1.0f;
            settingsUpdated = true;
        }

        //-------------------------------------------------------------------------

        return settingsUpdated;
    }

    //-------------------------------------------------------------------------

    bool RagdollDefinition::Profile::IsValid( int32_t numBodies ) const
    {
        if ( !m_ID.IsValid() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( numBodies > 0 )
        {
            if ( m_jointSettings.size() != numBodies - 1 )
            {
                return false;
            }
        }

        if ( m_bodySettings.size() != numBodies )
        {
            return false;
        }

        if ( m_materialSettings.size() != numBodies )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( !m_rootControlBodySettings.IsValid() )
        {
            return false;
        }

        for ( auto& bodySettings : m_bodySettings )
        {
            if ( !bodySettings.IsValid() )
            {
                return false;
            }
        }

        for ( auto& jointSettings : m_jointSettings )
        {
            if ( !jointSettings.IsValid() )
            {
                return false;
            }
        }

        for ( auto& materialSettings : m_materialSettings )
        {
            if ( !materialSettings.IsValid() )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        if ( m_solverPositionIterations == 0 )
        {
            return false;
        }

        if ( m_solverVelocityIterations == 0 )
        {
            return false;
        }

        if ( m_stabilizationThreshold < 0 )
        {
            return false;
        }

        if ( m_sleepThreshold < 0 )
        {
            return false;
        }

        if ( m_separationTolerance < 0 )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool RagdollDefinition::IsValid() const
    {
        if ( !m_skeleton.IsSet() )
        {
            return false;
        }

        if ( m_profiles.empty() )
        {
            return false;
        }

        // Validate the profiles
        int32_t const numBodies = (int32_t) m_bodies.size();
        for ( auto& profile : m_profiles )
        {
            if ( !profile.IsValid( numBodies ) )
            {
                return false;
            }
        }

        return true;
    }

    bool RagdollDefinition::Profile::CorrectSettingsToValidRanges()
    {
        bool updated = false;

        for ( auto& bodySettings : m_bodySettings )
        {
            updated |= bodySettings.CorrectSettingsToValidRanges();
        }

        for ( auto& jointSettings : m_jointSettings )
        {
            updated |= jointSettings.CorrectSettingsToValidRanges();
        }

        for ( auto& materialSettings : m_materialSettings )
        {
            updated |= materialSettings.CorrectSettingsToValidRanges();
        }

        //-------------------------------------------------------------------------

        if ( m_solverPositionIterations <= 0 || m_solverPositionIterations > 200 )
        {
            m_solverPositionIterations = Math::Clamp( m_solverPositionIterations, 1u, 200u );
        }

        if ( m_solverVelocityIterations <= 0 || m_solverVelocityIterations > 200 )
        {
            m_solverVelocityIterations = Math::Clamp( m_solverVelocityIterations, 1u, 200u );
        }

        if ( m_maxProjectionIterations <= 0 || m_maxProjectionIterations > 200 )
        {
            m_maxProjectionIterations = Math::Clamp( m_maxProjectionIterations, 1u, 200u );
        }

        if ( m_internalDriveIterations <= 0 || m_internalDriveIterations > 200 )
        {
            m_internalDriveIterations = Math::Clamp( m_internalDriveIterations, 1u, 200u );
        }

        if ( m_externalDriveIterations <= 0 || m_externalDriveIterations > 200 )
        {
            m_externalDriveIterations = Math::Clamp( m_externalDriveIterations, 1u, 200u );
        }

        if ( m_separationTolerance < 0.0f )
        {
            m_separationTolerance = Math::Max( m_separationTolerance, 0.0f );
        }

        if ( m_stabilizationThreshold < 0.0f )
        {
            m_stabilizationThreshold = Math::Max( m_stabilizationThreshold, 0.0f );
        }

        if ( m_sleepThreshold < 0.0f )
        {
            m_sleepThreshold = Math::Max( m_sleepThreshold, 0.0f );
        }

        //-------------------------------------------------------------------------

        return updated;
    }

    RagdollDefinition::Profile::Profile()
    {
        m_stabilizationThreshold = 0.01f * Constants::s_speedScale * Constants::s_speedScale;
    }

    RagdollDefinition::Profile* RagdollDefinition::GetProfile( StringID profileID )
    {
        for ( auto& profile : m_profiles )
        {
            if ( profile.m_ID == profileID )
            {
                return &profile;
            }
        }

        return nullptr;
    }

    int32_t RagdollDefinition::GetBodyIndexForBoneID( StringID boneID ) const
    {
        for ( auto bodyIdx = 0; bodyIdx < m_bodies.size(); bodyIdx++ )
        {
            if ( m_bodies[bodyIdx].m_boneID == boneID )
            {
                return bodyIdx;
            }
        }

        return InvalidIndex;
    }

    int32_t RagdollDefinition::GetBodyIndexForBoneIdx( int32_t boneIdx ) const
    {
        auto const& boneID = m_skeleton->GetBoneID( boneIdx );
        return GetBodyIndexForBoneID( boneID );
    }

    void RagdollDefinition::CreateRuntimeData()
    {
        EE_ASSERT( m_skeleton.IsLoaded() );

        int32_t const numBones = m_skeleton->GetNumBones();
        int32_t const numBodies = (int32_t) m_bodies.size();

        TInlineVector<Transform, 30> inverseBodyTransforms;

        // Calculate bone mappings and runtime helper transforms
        //-------------------------------------------------------------------------

        m_boneToBodyMap.clear();
        m_bodyToBoneMap.clear();

        m_boneToBodyMap.resize( numBones, InvalidIndex );
        m_bodyToBoneMap.resize( numBodies, InvalidIndex );
        inverseBodyTransforms.resize( numBodies );

        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            auto const& boneID = m_skeleton->GetBoneID( boneIdx );
            for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
            {
                if ( m_bodies[bodyIdx].m_boneID == boneID )
                {
                    m_bodies[bodyIdx].m_initialGlobalTransform = m_bodies[bodyIdx].m_offsetTransform * m_skeleton->GetBoneGlobalTransform( boneIdx );
                    m_bodies[bodyIdx].m_inverseOffsetTransform = m_bodies[bodyIdx].m_offsetTransform.GetInverse();

                    inverseBodyTransforms[bodyIdx] = m_bodies[bodyIdx].m_initialGlobalTransform.GetInverse();

                    m_boneToBodyMap[boneIdx] = bodyIdx;
                    m_bodyToBoneMap[bodyIdx] = boneIdx;
                    break;
                }
            }
        }

        // Calculate parent bone indices and joint relative transforms
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            auto& body = m_bodies[bodyIdx];
            body.m_parentBodyIdx = InvalidIndex;

            // Traverse hierarchy and set parent body idx to the first parent bone that has a body
            int32_t boneIdx = m_skeleton->GetBoneIndex( body.m_boneID );
            int32_t parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
            while ( parentBoneIdx != InvalidIndex )
            {
                int32_t const parentBodyIdx = m_boneToBodyMap[parentBoneIdx];
                if ( parentBodyIdx != InvalidIndex )
                {
                    body.m_parentBodyIdx = parentBodyIdx;
                    break;
                }
                else
                {
                    parentBoneIdx = m_skeleton->GetParentBoneIndex( parentBoneIdx );
                }
            }

            //-------------------------------------------------------------------------

            if ( body.m_parentBodyIdx != InvalidIndex )
            {
                body.m_parentRelativeJointTransform = body.m_jointTransform * inverseBodyTransforms[body.m_parentBodyIdx];
                body.m_bodyRelativeJointTransform = body.m_jointTransform * inverseBodyTransforms[bodyIdx];
            }
        }

        // Calculate self-collision rules
        //-------------------------------------------------------------------------

        for ( auto& profile : m_profiles )
        {
            uint64_t const noCollisionMask = 0;

            // Generate collision mask
            uint64_t hasCollisionMask = 0;
            for ( auto i = 0; i < numBodies; i++ )
            {
                if ( profile.m_bodySettings[i].m_enableSelfCollision )
                {
                    hasCollisionMask |= ( 1ULL << i );
                }
            }

            // Set collision masks
            profile.m_selfCollisionRules.resize( numBodies, 0 );
            for ( auto i = 0; i < numBodies; i++ )
            {
                if ( profile.m_bodySettings[i].m_enableSelfCollision )
                {
                    profile.m_selfCollisionRules[i] = hasCollisionMask;
                }
                else
                {
                    profile.m_selfCollisionRules[i] = noCollisionMask;
                }
            }
        }

        // Validate body parenting
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        int32_t numRootBodies = 0;
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            EE_ASSERT( m_bodies[bodyIdx].m_parentBodyIdx != bodyIdx);

            if ( m_bodies[bodyIdx].m_parentBodyIdx == InvalidIndex )
            {
                numRootBodies++;
            }
        }
        EE_ASSERT( numRootBodies == 1 );
        #endif
    }
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    Ragdoll::Ragdoll( PxPhysics* pPhysics, RagdollDefinition const* pDefinition, StringID const profileID, uint64_t userID )
        : m_pPhysics( pPhysics )
        , m_pDefinition( pDefinition )
        , m_userID( userID )
    {
        EE_ASSERT( pPhysics != nullptr );
        EE_ASSERT( pDefinition != nullptr && pDefinition->IsValid() );

        // Get profile
        //-------------------------------------------------------------------------

        EE_ASSERT( profileID.IsValid() ? pDefinition->HasProfile( profileID ) : true );

        if ( profileID.IsValid() )
        {
            m_pProfile = pDefinition->GetProfile( profileID );
        }
        else
        {
            m_pProfile = pDefinition->GetDefaultProfile();
        }
        EE_ASSERT( m_pProfile != nullptr );

        // Create articulation
        //-------------------------------------------------------------------------

        m_pArticulation = pPhysics->createArticulation();
        EE_ASSERT( m_pArticulation != nullptr );
        m_pArticulation->userData = this;

        #if EE_DEVELOPMENT_TOOLS
        m_ragdollName.sprintf( "%s - %s", pDefinition->GetResourceID().c_str(), m_pProfile->m_ID.c_str() );
        m_pArticulation->setName( m_ragdollName.c_str() );
        #endif

        UpdateSolverSettings();

        // Create links
        //-------------------------------------------------------------------------

        int32_t const numBodies = pDefinition->GetNumBodies();
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            auto const& bodyDefinition = pDefinition->m_bodies[bodyIdx];

            PxTransform const linkPose = ToPx( bodyDefinition.m_initialGlobalTransform );
            PxArticulationLink* pParentLink = ( bodyIdx == 0 ) ? nullptr : m_links[bodyDefinition.m_parentBodyIdx];
            PxArticulationLink* pLink = m_pArticulation->createLink( pParentLink, linkPose );
            pLink->userData = (void*) uintptr_t( bodyIdx );
            m_links.emplace_back( pLink );

            #if EE_DEVELOPMENT_TOOLS
            pLink->setName( bodyDefinition.m_boneID.c_str() );
            #endif

            // Set Mass
            EE_ASSERT( m_pProfile->m_bodySettings[bodyIdx].m_mass > 0.0f );
            PxRigidBodyExt::setMassAndUpdateInertia( *pLink, m_pProfile->m_bodySettings[bodyIdx].m_mass );

            // Create material
            auto const& materialSettings = m_pProfile->m_materialSettings[bodyIdx];
            auto pMaterial = pPhysics->createMaterial( materialSettings.m_staticFriction, materialSettings.m_dynamicFriction, materialSettings.m_restitution );
            pMaterial->setFrictionCombineMode( (PxCombineMode::Enum) materialSettings.m_frictionCombineMode );
            pMaterial->setRestitutionCombineMode( (PxCombineMode::Enum) materialSettings.m_restitutionCombineMode );

            // Create shape
            PxCapsuleGeometry const capsuleGeo( bodyDefinition.m_radius, bodyDefinition.m_halfHeight );
            /*auto pShape =*/ PxRigidActorExt::createExclusiveShape( *pLink, capsuleGeo, *pMaterial );
            //pShape->setSimulationFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
            //pShape->setQueryFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
            //pShape->userData = this;

            pMaterial->release();
        }

        // Create joints
        //-------------------------------------------------------------------------

        for ( int32_t bodyIdx = 1; bodyIdx < numBodies; bodyIdx++ )
        {
            PxArticulationJoint* pJoint = static_cast<PxArticulationJoint*>( m_links[bodyIdx]->getInboundJoint() );
            pJoint->setParentPose( ToPx( pDefinition->m_bodies[bodyIdx].m_parentRelativeJointTransform ) );
            pJoint->setChildPose( ToPx( pDefinition->m_bodies[bodyIdx].m_bodyRelativeJointTransform ) );
            pJoint->setTargetVelocity( PxZero );
            pJoint->setTargetOrientation( PxIdentity );
        }

        UpdateJointSettings();
    }

    Ragdoll::~Ragdoll()
    {
        EE_ASSERT( m_pArticulation->getScene() == nullptr );
        EE_ASSERT( m_pRootControlActor == nullptr );

        m_pArticulation->release();
        m_pArticulation = nullptr;

        m_links.empty();
        m_pDefinition = nullptr;
        m_pPhysics = nullptr;
    }

    //-------------------------------------------------------------------------

    void Ragdoll::AddToScene( physx::PxScene* pScene )
    {
        EE_ASSERT( pScene != nullptr && m_pArticulation->getScene() == nullptr );
        EE_ASSERT( m_pRootControlActor == nullptr );

        pScene->lockWrite();
        pScene->addArticulation( *m_pArticulation );
        TryCreateRootControlBody();
        pScene->unlockWrite();
    }

    void Ragdoll::RemoveFromScene()
    {
        auto pScene = m_pArticulation->getScene();
        EE_ASSERT( pScene != nullptr );

        pScene->lockWrite();
        DestroyRootControlBody();
        pScene->removeArticulation( *m_pArticulation );
        pScene->unlockWrite();
    }

    //-------------------------------------------------------------------------

    void Ragdoll::TryCreateRootControlBody()
    {
        EE_ASSERT( m_pArticulation != nullptr );
        EE_ASSERT( m_pRootControlActor == nullptr && m_pRootTargetJoint == nullptr );

        //-------------------------------------------------------------------------

        auto pScene = m_pArticulation->getScene();
        if ( pScene == nullptr )
        {
            return;
        }

        if ( m_pProfile->m_rootControlBodySettings.m_driveType == RagdollRootControlBodySettings::None || m_pProfile->m_rootControlBodySettings.m_driveType == RagdollRootControlBodySettings::Kinematic )
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Target Actor
        //-------------------------------------------------------------------------

        auto pMaterial = m_pPhysics->createMaterial( 0.0f, 0.0f, 0.0f );
        pMaterial->setFrictionCombineMode( PxCombineMode::eMIN );
        pMaterial->setRestitutionCombineMode( PxCombineMode::eMIN );

        auto const& rootBodyDefinition = m_pDefinition->m_bodies[0];
        PxCapsuleGeometry const capsuleGeo( rootBodyDefinition.m_radius, rootBodyDefinition.m_halfHeight );
        auto pShape = m_pPhysics->createShape( capsuleGeo, &pMaterial, 1, true, PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSIMULATION_SHAPE );
        pMaterial->release();

        //pShape->setSimulationFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
        //pShape->setQueryFilterData( PxFilterData( pComponent->m_layers.Get(), 0, 0, 0 ) );
        //pShape->userData = this;

        #if EE_DEVELOPMENT_TOOLS
        pShape->setName( "Root Body Target" );
        #endif

        //-------------------------------------------------------------------------

        // Create rigid body actor - target actor isnt affected by gravity
        m_pRootControlActor = m_pPhysics->createRigidDynamic( ToPx( rootBodyDefinition.m_initialGlobalTransform ) );
        m_pRootControlActor->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, true );
        m_pRootControlActor->setRigidBodyFlag( physx::PxRigidBodyFlag::eKINEMATIC, true );
        m_pRootControlActor->attachShape( *pShape );
        pShape->release();


        //-------------------------------------------------------------------------
        // Joint
        //-------------------------------------------------------------------------

        static PxTransform const I( PxIdentity );

        ScopedWriteLock const sl( this );
        pScene->addActor( *m_pRootControlActor );

        PxDistanceJointFlags flags = PxDistanceJointFlag::eMAX_DISTANCE_ENABLED;
        if( m_pProfile->m_rootControlBodySettings.m_driveType == RagdollRootControlBodySettings::Spring )
        {
            flags |= PxDistanceJointFlag::eSPRING_ENABLED;
        }

        m_pRootTargetJoint = PxDistanceJointCreate( *m_pPhysics, m_pRootControlActor, I, m_links[0], I );
        m_pRootTargetJoint->setDistanceJointFlags( flags );

        auto const& controlBodySettings = m_pProfile->m_rootControlBodySettings;
        m_pRootTargetJoint->setMaxDistance( controlBodySettings.m_maxDistance );
        m_pRootTargetJoint->setTolerance( controlBodySettings.m_tolerance );
        m_pRootTargetJoint->setStiffness( controlBodySettings.m_stiffness );
        m_pRootTargetJoint->setDamping( controlBodySettings.m_damping );
    }

    void Ragdoll::DestroyRootControlBody()
    {
        if ( m_pRootControlActor != nullptr )
        {
            EE_ASSERT( m_pRootTargetJoint != nullptr );

            ScopedWriteLock const sl( this );
            m_pRootTargetJoint->release();
            m_pRootTargetJoint = nullptr;

            m_pRootControlActor->release();
            m_pRootControlActor = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void Ragdoll::SwitchProfile( StringID newProfileID )
    {
        ScopedWriteLock const sl( this );

        DestroyRootControlBody();

        m_pProfile = m_pDefinition->GetProfile( newProfileID );
        EE_ASSERT( m_pProfile != nullptr );

        UpdateSolverSettings();
        UpdateBodySettings();
        UpdateJointSettings();

        TryCreateRootControlBody();
    }

    void Ragdoll::UpdateSolverSettings()
    {
        EE_ASSERT( IsValid() );

        ScopedWriteLock const sl( this );
        m_pArticulation->setSolverIterationCounts( m_pProfile->m_solverPositionIterations, m_pProfile->m_solverVelocityIterations );
        m_pArticulation->setStabilizationThreshold( m_pProfile->m_stabilizationThreshold );
        m_pArticulation->setMaxProjectionIterations( m_pProfile->m_maxProjectionIterations );
        m_pArticulation->setSleepThreshold( m_pProfile->m_sleepThreshold );
        m_pArticulation->setSeparationTolerance( m_pProfile->m_separationTolerance );
        m_pArticulation->setInternalDriveIterations( m_pProfile->m_internalDriveIterations );
        m_pArticulation->setExternalDriveIterations( m_pProfile->m_externalDriveIterations );
    }

    void Ragdoll::UpdateBodySettings()
    {
        int32_t const numBodies = (int32_t) m_links.size();

        EE_ASSERT( IsValid() );

        ScopedWriteLock const sl( this );
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            if ( bodyIdx == 0 )
            {
                bool const isGravityDisabled = ( m_pRootControlActor != nullptr ) || !m_gravityEnabled;
                m_links[bodyIdx]->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, isGravityDisabled );
            }
            else
            {
                bool const isGravityDisabled = ( m_pProfile->m_jointSettings[bodyIdx - 1].m_driveType == RagdollJointSettings::Kinematic ) || !m_gravityEnabled;
                m_links[bodyIdx]->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, isGravityDisabled );
            }

            // Set Mass and other properties
            PxRigidBodyExt::setMassAndUpdateInertia( *m_links[bodyIdx], m_pProfile->m_bodySettings[bodyIdx].m_mass );
            m_links[bodyIdx]->setRigidBodyFlag( PxRigidBodyFlag::eENABLE_CCD, m_pProfile->m_bodySettings[bodyIdx].m_enableCCD );
            m_links[bodyIdx]->setMaxAngularVelocity( PX_MAX_F32 );
            m_links[bodyIdx]->setMaxLinearVelocity( PX_MAX_F32 );

            // Create new material
            auto const& materialSettings = m_pProfile->m_materialSettings[bodyIdx];
            auto& physics = m_pArticulation->getScene()->getPhysics();
            PxMaterial* pMaterial = physics.createMaterial( materialSettings.m_staticFriction, materialSettings.m_dynamicFriction, materialSettings.m_restitution );
            pMaterial->setFrictionCombineMode( (PxCombineMode::Enum) materialSettings.m_frictionCombineMode );
            pMaterial->setRestitutionCombineMode( (PxCombineMode::Enum) materialSettings.m_restitutionCombineMode );

            // Update materials
            PxShape* pShape = nullptr;
            int32_t const numShapesFound = m_links[bodyIdx]->getShapes( &pShape, 1 );
            EE_ASSERT( numShapesFound == 1 );
            pShape->setMaterials( &pMaterial, 1 );
            pMaterial->release();
        }
    }

    void Ragdoll::UpdateJointSettings()
    {
        EE_ASSERT( IsValid() );

        ScopedWriteLock const sl( this );

        // Root body
        //-------------------------------------------------------------------------

        if ( m_pRootControlActor != nullptr )
        {
            auto const& controlBodySettings = m_pProfile->m_rootControlBodySettings;

            PxDistanceJointFlags flags = PxDistanceJointFlag::eMAX_DISTANCE_ENABLED;
            if ( m_pProfile->m_rootControlBodySettings.m_driveType == RagdollRootControlBodySettings::Spring )
            {
                flags |= PxDistanceJointFlag::eSPRING_ENABLED;
            }

            m_pRootTargetJoint->setDistanceJointFlags( flags );
            m_pRootTargetJoint->setMaxDistance( controlBodySettings.m_maxDistance );
            m_pRootTargetJoint->setTolerance( controlBodySettings.m_tolerance );
            m_pRootTargetJoint->setStiffness( controlBodySettings.m_stiffness );
            m_pRootTargetJoint->setDamping( controlBodySettings.m_damping );
        }

        // Body and joint setting
        //-------------------------------------------------------------------------

        int32_t const numBodies = (int32_t) m_links.size();
        for ( int32_t bodyIdx = 1; bodyIdx < numBodies; bodyIdx++ )
        {
            int32_t const jointIdx = bodyIdx - 1;
            auto const& jointSettings = m_pProfile->m_jointSettings[jointIdx];
            PxArticulationJoint* pJoint = static_cast<PxArticulationJoint*>( m_links[bodyIdx]->getInboundJoint() );

            // Shared Settings
            //-------------------------------------------------------------------------

            pJoint->setInternalCompliance( jointSettings.m_internalCompliance );
            pJoint->setExternalCompliance( jointSettings.m_externalCompliance );
            pJoint->setDriveType( PxArticulationJointDriveType::eTARGET );
            pJoint->setDamping( jointSettings.m_damping );
            pJoint->setStiffness( jointSettings.m_stiffness );
            pJoint->setTangentialStiffness( jointSettings.m_tangentialStiffness );
            pJoint->setTangentialDamping( jointSettings.m_tangentialDamping );

            // Drive specific Settings
            //-------------------------------------------------------------------------

            if ( jointSettings.m_driveType == RagdollJointSettings::DriveType::Kinematic )
            {
                pJoint->setTwistLimitEnabled( false );
                pJoint->setSwingLimitEnabled( false );
            }
            else
            {
                pJoint->setTwistLimitEnabled( jointSettings.m_twistLimitEnabled );
                pJoint->setTwistLimit( Math::DegreesToRadians * jointSettings.m_twistLimitMin, Math::DegreesToRadians * jointSettings.m_twistLimitMax );
                pJoint->setTwistLimitContactDistance( Math::DegreesToRadians * jointSettings.m_twistLimitContactDistance );

                pJoint->setSwingLimitEnabled( jointSettings.m_swingLimitEnabled );
                pJoint->setSwingLimit( Math::DegreesToRadians * jointSettings.m_swingLimitZ, Math::DegreesToRadians * jointSettings.m_swingLimitY );
                pJoint->setSwingLimitContactDistance( Math::DegreesToRadians * jointSettings.m_swingLimitContactDistance );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool Ragdoll::IsSleeping() const
    {
        EE_ASSERT( IsValid() );
        ScopedReadLock const sl( this );
        return m_pArticulation->isSleeping();
    }

    void Ragdoll::PutToSleep()
    {
        EE_ASSERT( IsValid() );
        ScopedWriteLock const sl( this );
        m_pArticulation->putToSleep();
    }

    void Ragdoll::WakeUp()
    {
        EE_ASSERT( IsValid() );
        ScopedWriteLock const sl( this );
        m_pArticulation->wakeUp();
    }

    void Ragdoll::SetGravityEnabled( bool isGravityEnabled )
    {
        EE_ASSERT( IsValid() );

        if ( m_gravityEnabled == isGravityEnabled )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_gravityEnabled = isGravityEnabled;

        ScopedWriteLock const sl( this );
        int32_t const numBodies = (int32_t) m_links.size();
        for ( auto bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            if ( bodyIdx == 0 )
            {
                bool const isBodyGravityDisabled = ( m_pProfile->m_rootControlBodySettings.m_driveType != RagdollRootControlBodySettings::None ) || !m_gravityEnabled;
                m_links[bodyIdx]->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, isBodyGravityDisabled );
            }
            else
            {
                int32_t const jointIdx = bodyIdx - 1;
                auto const& jointSettings = m_pProfile->m_jointSettings[jointIdx];
                bool const isBodyGravityDisabled = ( jointSettings.m_driveType == RagdollJointSettings::Kinematic) || !m_gravityEnabled;
                m_links[bodyIdx]->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, isBodyGravityDisabled );
            }
        }
    }

    void Ragdoll::SetPoseFollowingEnabled( bool shouldFollowPose )
    {
        if ( m_shouldFollowPose == shouldFollowPose )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_shouldFollowPose = shouldFollowPose;

        if ( m_shouldFollowPose ) 
        {
            DestroyRootControlBody();
            TryCreateRootControlBody();
        }
        else // Clear any set targets when disabling pose following
        {
            ScopedWriteLock const sl( this );
            int32_t const numBodies = (int32_t) m_links.size();

            // Destroy root control body
            DestroyRootControlBody();

            // Clear joint velocities
            for ( int32_t i = 1; i < numBodies; i++ )
            {
                auto pJoint = static_cast<PxArticulationJoint*>( m_links[i]->getInboundJoint() );
                pJoint->setTargetVelocity( PxZero );
                pJoint->setTargetOrientation( PxIdentity );
            }
        }
    }

    //-------------------------------------------------------------------------

    void Ragdoll::Update( Seconds const deltaTime, Transform const& worldTransform, Animation::Pose* pPose, bool initializeBodies )
    {
        EE_ASSERT( IsValid() );

        //-------------------------------------------------------------------------

        if ( m_shouldFollowPose )
        {
            EE_ASSERT( pPose != nullptr );
            EE_ASSERT( pPose->GetSkeleton()->GetResourceID() == m_pDefinition->m_skeleton.GetResourceID() );
            EE_ASSERT( pPose->HasGlobalTransforms() );

            ScopedWriteLock const sl( this );

            m_pArticulation->wakeUp();

            // Update root control body
            //-------------------------------------------------------------------------

            bool const isKinematicRoot = m_pProfile->m_rootControlBodySettings.m_driveType == RagdollRootControlBodySettings::Kinematic;
            if ( m_pRootControlActor != nullptr )
            {
                int32_t const rootBoneIdx = m_pDefinition->m_bodyToBoneMap[0];
                Transform const rootAnimWorldTransform = pPose->GetGlobalTransform( rootBoneIdx ) * worldTransform;
                Transform const rootBodyWorldTransform = m_pDefinition->m_bodies[0].m_offsetTransform * rootAnimWorldTransform;
                m_pRootControlActor->setKinematicTarget( ToPx( rootBodyWorldTransform ) );
            }

            // Update root body
            //-------------------------------------------------------------------------

            // drive via velocity/angular velocity

            if ( initializeBodies || isKinematicRoot )
            {
                int32_t const boneIdx = m_pDefinition->m_bodyToBoneMap[0];
                Transform const boneWorldTransform = pPose->GetGlobalTransform( boneIdx ) * worldTransform;
                Transform const bodyWorldTransform = m_pDefinition->m_bodies[0].m_offsetTransform * boneWorldTransform;
                m_links[0]->setGlobalPose( ToPx( bodyWorldTransform ) );
            }

            // Update bodies and joint Targets
            //-------------------------------------------------------------------------

            int32_t const numBodies = (int32_t) m_links.size();
            for ( int32_t bodyIdx = 1; bodyIdx < numBodies; bodyIdx++ )
            {
                int32_t const boneIdx = m_pDefinition->m_bodyToBoneMap[bodyIdx];
                Transform const boneWorldTransform = pPose->GetGlobalTransform( boneIdx ) * worldTransform;
                Transform const desiredBodyWorldTransform = m_pDefinition->m_bodies[bodyIdx].m_offsetTransform * boneWorldTransform;

                if ( initializeBodies )
                {
                    m_links[bodyIdx]->setGlobalPose( ToPx( desiredBodyWorldTransform ) );
                    m_links[bodyIdx]->setLinearVelocity( PxZero );
                    m_links[bodyIdx]->setAngularVelocity( PxZero );
                }

                //-------------------------------------------------------------------------

                int32_t const jointIdx = bodyIdx - 1;
                auto pJoint = static_cast<PxArticulationJoint*>( m_links[bodyIdx]->getInboundJoint() );
                auto const& jointSettings = m_pProfile->m_jointSettings[jointIdx];

                // Kinematic
                if ( jointSettings.m_driveType == RagdollJointSettings::Kinematic )
                {
                    Transform const currentBodyTransform = FromPx( m_links[bodyIdx]->getGlobalPose() );

                    Vector const linearVelocity = ( desiredBodyWorldTransform.GetTranslation() - currentBodyTransform.GetTranslation() ) / deltaTime;
                    m_links[bodyIdx]->setLinearVelocity( ToPx( linearVelocity ) );

                    Vector const angularVelocity = Math::CalculateAngularVelocity( currentBodyTransform.GetRotation(), desiredBodyWorldTransform.GetRotation(), deltaTime );
                    m_links[bodyIdx]->setAngularVelocity( ToPx( angularVelocity ) );

                    pJoint->setTargetVelocity( PxZero );
                }
                else // Driven
                {
                    int32_t const parentBoneIdx = m_pDefinition->m_bodyToBoneMap[m_pDefinition->m_bodies[bodyIdx].m_parentBodyIdx];
                    Transform const parentBoneWorldTransform = pPose->GetGlobalTransform( parentBoneIdx ) * worldTransform;
                    Transform const parentBodyWorldTransform = m_pDefinition->m_bodies[m_pDefinition->m_bodies[bodyIdx].m_parentBodyIdx].m_offsetTransform * parentBoneWorldTransform;

                    Transform const jointWorldTransformRelativeToBody = m_pDefinition->m_bodies[bodyIdx].m_bodyRelativeJointTransform * desiredBodyWorldTransform;
                    Transform const jointWorldTransformRelativeToParentBody = m_pDefinition->m_bodies[bodyIdx].m_parentRelativeJointTransform * parentBodyWorldTransform;

                    // Get the delta rotation and ensure that it's the shortest path rotation
                    Quaternion jointDeltaRotation = Quaternion::Delta( jointWorldTransformRelativeToParentBody.GetRotation(), jointWorldTransformRelativeToBody.GetRotation() );
                    if ( jointDeltaRotation.m_w < 0.0f )
                    {
                        jointDeltaRotation.Negate();
                    }

                    if ( jointSettings.m_driveType == RagdollJointSettings::Spring )
                    {
                        pJoint->setTargetOrientation( ToPx( jointDeltaRotation ) );
                        pJoint->setTargetVelocity( PxZero );
                    }
                    else if ( jointSettings.m_driveType == RagdollJointSettings::Velocity )
                    {
                        pJoint->setTargetOrientation( ToPx( jointDeltaRotation ) );

                        Vector const angularVelocity = Math::CalculateAngularVelocity( jointWorldTransformRelativeToParentBody.GetRotation(), jointWorldTransformRelativeToBody.GetRotation(), deltaTime );
                        pJoint->setTargetVelocity( ToPx( angularVelocity ) * 0.1f );
                    }
                }
            }
        }
    }

    bool Ragdoll::GetPose( Transform const& worldTransform, Animation::Pose* pPose ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( pPose->GetSkeleton()->GetResourceID() == m_pDefinition->m_skeleton.GetResourceID() );
        EE_ASSERT( pPose->HasGlobalTransforms() );

        // Copy the global bone transforms here
        //-------------------------------------------------------------------------

        int32_t const numBones = pPose->GetNumBones();
        m_globalBoneTransforms.resize( numBones );
        m_globalBoneTransforms = pPose->GetGlobalTransforms();

        //-------------------------------------------------------------------------

        bool invalidTransformDetected = false;

        {
            ScopedReadLock const sl( this );
            int32_t const numBodies = (int32_t) m_links.size();
            for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
            {
                PxTransform const ragdollBodyTransform = m_links[bodyIdx]->getGlobalPose();
                if ( !ragdollBodyTransform.isSane() )
                {
                    invalidTransformDetected = true;
                    pPose->ClearGlobalTransforms();
                    break;
                }

                // Convert from world space to character space
                int32_t const boneIdx = m_pDefinition->m_bodyToBoneMap[bodyIdx];
                Transform const bodyWorldTransform = FromPx( ragdollBodyTransform );
                Transform const boneWorldTranform = m_pDefinition->m_bodies[bodyIdx].m_inverseOffsetTransform * bodyWorldTransform;
                m_globalBoneTransforms[boneIdx] = Transform::Delta( worldTransform, boneWorldTranform );
            }
        }

        if ( invalidTransformDetected )
        {
            return false;
        }

        // Calculate the local transforms and set back into the pose
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < numBones; i++ )
        {
            // Keep original local transforms for non-body bones
            if ( m_pDefinition->m_boneToBodyMap[i] == InvalidIndex )
            {
                continue;
            }

            //-------------------------------------------------------------------------

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

    void Ragdoll::GetRagdollPose( RagdollPose& pose ) const
    {
        EE_ASSERT( IsValid() );

        int32_t const numBodies = m_pDefinition->GetNumBodies();
        pose.resize( numBodies );

        ScopedReadLock const sl( this );
        for ( auto i = 0; i < numBodies; i++ )
        {
            pose[i] = FromPx( m_links[i]->getGlobalPose() );
        }
    }

    //-------------------------------------------------------------------------

    void Ragdoll::LockWriteScene()
    {
        if ( auto pScene = m_pArticulation->getScene() )
        {
            pScene->lockWrite();
        }
    }

    void Ragdoll::UnlockWriteScene()
    {
        if ( auto pScene = m_pArticulation->getScene() )
        {
            pScene->unlockWrite();
        }
    }

    void Ragdoll::LockReadScene() const
    {
        if ( auto pScene = m_pArticulation->getScene() )
        {
            pScene->lockRead();
        }
    }

    void Ragdoll::UnlockReadScene() const
    {
        if ( auto pScene = m_pArticulation->getScene() )
        {
            pScene->unlockRead();
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Ragdoll::RefreshSettings()
    {
        ScopedWriteLock const sl( this );

        // TODO: handle root changes

        UpdateSolverSettings();
        UpdateJointSettings();
        UpdateBodySettings();
    }

    void Ragdoll::RecreateRootControlBody()
    {
        ScopedWriteLock const sl( this );
        DestroyRootControlBody();
        TryCreateRootControlBody();
    }

    void Ragdoll::ResetState()
    {
        ScopedWriteLock const sl( this );
        int32_t const numBodies = (int32_t) m_links.size();
        for ( int32_t i = 0; i < numBodies; i++ )
        {
            m_links[i]->setLinearVelocity( PxZero );
            m_links[i]->setAngularVelocity( PxZero );

            if ( i > 0 )
            {
                auto pJoint = static_cast<PxArticulationJoint*>( m_links[i]->getInboundJoint() );
                pJoint->setTargetVelocity( PxZero );
                pJoint->setTargetOrientation( PxIdentity );
            }
        }
    }

    void Ragdoll::DrawDebug( Drawing::DrawContext& ctx ) const
    {
        RagdollPose pose;
        Transform rootTargetWorldTransform;

        // Get transforms
        //-------------------------------------------------------------------------

        {
            ScopedReadLock const sl( this );
            if ( m_pRootControlActor != nullptr )
            {
                rootTargetWorldTransform = FromPx( m_pRootControlActor->getGlobalPose() );
            }
            GetRagdollPose( pose );
        }

        // Draw Root Target
        //-------------------------------------------------------------------------

        if ( m_pRootControlActor != nullptr )
        {
            auto const& bodySettings = m_pDefinition->m_bodies[0];
            ctx.DrawCapsuleHeightX( rootTargetWorldTransform, bodySettings.m_radius, bodySettings.m_halfHeight, Colors::Lime, 2.0f );
        }

        // Draw Ragdoll Bodies
        //-------------------------------------------------------------------------

        int32_t const numBodies = m_pDefinition->GetNumBodies();
        for ( int32_t i = 0; i < numBodies; i++ )
        {
            auto const& bodySettings = m_pDefinition->m_bodies[i];
            ctx.DrawCapsuleHeightX( pose[i], bodySettings.m_radius, bodySettings.m_halfHeight, Colors::Yellow, 2.0f );

            if ( i > 0 )
            {
                Transform const jointTransformC = bodySettings.m_bodyRelativeJointTransform * pose[i];
                ctx.DrawArrow( jointTransformC.GetTranslation(), jointTransformC.GetTranslation() + ( jointTransformC.GetAxisX() * 0.2f ), Colors::Pink, 2.0f );
            }
        }
    }
    #endif
}