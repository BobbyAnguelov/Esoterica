#include "PhysicsRagdoll_Definition.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool RagdollJointDefinition::IsValid( Log* pLog ) const
    {
        if ( m_type == Type::None )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        if ( m_enableSpring )
        {
            // TODO: check values
            /*if ( !Math::IsInRangeInclusive( m_springStiffnessHertz, 0.0f, 1.0f ) )
            {
                return false;
            }

            if ( !Math::IsInRangeInclusive( m_dampingRatio, 0.0f, 1.0f ) )
            {
                return false;
            }*/
        }

        if ( m_enableMotor )
        {
            // TODO: check values
           /* if ( !Math::IsInRangeInclusive( m_maxMotorTorque, 0.0f, 1.0f ) )
            {
                return false;
            }*/
        }

        if ( m_type == Type::Revolute )
        {
            if ( m_enableHingeLimit )
            {
                if ( !Math::IsInRangeInclusive( m_hingeLowerLimit.ToFloat(), -180.0f, 180.0f) )
                {
                    if ( pLog ) pLog->LogError( "Invalid Hinge Lower Limit" );
                    return false;
                }

                if ( !Math::IsInRangeInclusive( m_hingeUpperLimit.ToFloat(), -180.0f, 180.0f ) )
                {
                    if ( pLog ) pLog->LogError( "Invalid Hinge Upper Limit" );
                    return false;
                }

                if ( m_hingeLowerLimit > m_hingeUpperLimit )
                {
                    if ( pLog ) pLog->LogError( "Invalid Hinge Limits" );
                    return false;
                }
            }
        }
        else if ( m_type == Type::Spherical )
        {
            if ( m_enableTwistLimit )
            {
                if ( !Math::IsInRangeInclusive( m_twistLowerLimit.ToFloat(), -180.0f, 180.0f ) )
                {
                    if ( pLog ) pLog->LogError( "Invalid Twist Lower Limit" );
                    return false;
                }

                if ( !Math::IsInRangeInclusive( m_twistUpperLimit.ToFloat(), -180.0f, 180.0f ) )
                {
                    if ( pLog ) pLog->LogError( "Invalid Twist Upper Limit" );
                    return false;
                }

                if ( m_twistLowerLimit > m_twistUpperLimit )
                {
                    if ( pLog ) pLog->LogError( "Invalid Twist Limits" );
                    return false;
                }
            }

            if ( m_enableConeLimit )
            {
                if ( !Math::IsInRangeInclusive( m_coneAngle.ToFloat(), 0.0f, 180.0f ) )
                {
                    if ( pLog ) pLog->LogError( "Invalid Cone Limits" );
                    return false;
                }
            }
        }

        return true;
    }

    void RagdollJointDefinition::FinalizeJoint( Transform const& transformA, Transform const& transformB )
    {
        Transform relativeTransform = Transform::Delta( transformA, transformB );

        if( m_type == Type::Revolute )
        {
            Quaternion quatA = m_orientation;
            Quaternion frameQuatA = quatA * transformA.GetRotation().GetInverse();
            m_frameA = Transform( frameQuatA, relativeTransform.GetTranslation() );

            Quaternion frameQuatB = quatA * transformB.GetRotation().GetInverse();
            m_frameB = Transform( frameQuatB );
        }
        else if ( m_type == Type::Spherical )
        {
            Quaternion quatA = m_orientation;
            Quaternion frameQuatA = quatA * transformA.GetRotation().GetInverse();
            m_frameA = Transform( frameQuatA, relativeTransform.GetTranslation() );

            Quaternion quatB = m_orientation;
            Quaternion frameQuatB = quatB * transformB.GetRotation().GetInverse();
            m_frameB = Transform( frameQuatB );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void RagdollJointDefinition::DrawDebug( DebugDrawContext& ctx, Transform const& transformA, Transform const& transformB ) const
    {
        Transform const frameA = m_frameA * transformA;
        Transform const frameB = m_frameB * transformB;

        Quaternion quatA = frameA.GetRotation();
        Quaternion quatB = frameB.GetRotation();

        // This keeps the twist angle in the range [-pi, pi]
        if ( Quaternion::Dot( quatA, quatB ).GetX() < 0.0f )
        {
            quatB.Negate();
        }

        Quaternion relQ = Quaternion::Delta( quatA, quatB );

        ctx.DrawAxis( frameA, 0.1f, 2.0f );

        //-------------------------------------------------------------------------

        auto DrawTwistLimit = [&] ( float const wedgeRadius, float const lowerAngle, float const upperAngle )
        {
            constexpr int32_t const sliceCount = 16;
            for ( int32_t index = 0; index < sliceCount; ++index )
            {
                float t1 = static_cast<float>( index + 0 ) / sliceCount;
                float alpha1 = ( 1.0f - t1 ) * lowerAngle + t1 * upperAngle;
                float t2 = static_cast<float>( index + 1 ) / sliceCount;
                float alpha2 = ( 1.0f - t2 ) * lowerAngle + t2 * upperAngle;

                Vector vertex1 = { wedgeRadius * Math::Cos( alpha1 ), wedgeRadius * Math::Sin( alpha1 ), 0.0f };
                Vector vertex2 = { wedgeRadius * Math::Cos( alpha2 ), wedgeRadius * Math::Sin( alpha2 ), 0.0f };

                if ( index == 0 )
                {
                    ctx.DrawLine( frameA.GetTranslation(), frameA.TransformPoint( vertex1 ), Colors::LightSalmon, 3.0f );
                }

                if ( index == sliceCount - 1 )
                {
                    ctx.DrawLine( frameA.TransformPoint( vertex2 ), frameA.GetTranslation(), Colors::LightSalmon, 3.0f );
                }

                ctx.DrawLine( frameA.TransformPoint( vertex1 ), frameA.TransformPoint( vertex2 ), Colors::LightSalmon, 3.0f );

                //-------------------------------------------------------------------------

                float twistAngle = relQ.GetTwistAngle().ToFloat();
                Vector p2 = { wedgeRadius * Math::Cos( twistAngle ), wedgeRadius * Math::Sin( twistAngle ), 0.0f };
                ctx.DrawArrow( frameA.GetTranslation(), frameA.TransformPoint( p2 ), Colors::Gold, 3.0f );
            }
        };

        //-------------------------------------------------------------------------

        if ( m_type == RagdollJointDefinition::Type::Revolute )
        {
            if ( m_enableTwistLimit )
            {
                float const lowerAngle = Math::DegreesToRadians * m_hingeLowerLimit.ToFloat();
                float const upperAngle = Math::DegreesToRadians * m_hingeUpperLimit.ToFloat();
                DrawTwistLimit( 0.2f, lowerAngle, upperAngle );
            }
        }
        else if ( m_type == RagdollJointDefinition::Type::Spherical )
        {
            ctx.DrawArrow( frameB.GetTranslation(), frameB.GetTranslation() + ( frameB.GetAxisZ() * 0.15f ), Colors::DeepPink, 3.0f );

            // Twist limit
            if ( m_enableTwistLimit )
            {
                float const lowerAngle = Math::DegreesToRadians * m_twistLowerLimit.ToFloat();
                float const upperAngle = Math::DegreesToRadians * m_twistUpperLimit.ToFloat();
                DrawTwistLimit( 0.1f, lowerAngle, upperAngle );
            }

            // Swing limit
            if ( m_enableConeLimit )
            {
                float const radius = 0.1f;
                float coneRadius = radius * Math::Sin( m_coneAngle.ToRadians().ToFloat() );
                float coneHeight = radius * Math::Cos( m_coneAngle.ToRadians().ToFloat() );

                constexpr int32_t const sectionCount = 16;
                for ( int32_t index = 0; index < sectionCount; ++index )
                {
                    float const phi1 = 2.0f * ( index + 0 ) / sectionCount * Math::Pi;
                    float const phi2 = 2.0f * ( index + 1 ) / sectionCount * Math::Pi;

                    Vector const vertex1( coneRadius * Math::Cos( phi1 ), coneRadius * Math::Sin( phi1 ), coneHeight );
                    Vector const vertex2( coneRadius * Math::Cos( phi2 ), coneRadius * Math::Sin( phi2 ), coneHeight );

                    ctx.DrawLine( frameA.GetTranslation(), frameA.TransformPoint( vertex1 ), Colors::HotPink );
                    ctx.DrawLine( frameA.TransformPoint( vertex1 ), frameA.TransformPoint( vertex2 ), Colors::HotPink );
                }
            }
        }
    }
    #endif

    //-------------------------------------------------------------------------

    bool RagdollShapeDefinition::IsValid( Log* pLog ) const
    {
        if ( m_radius <= 0.0f )
        {
            if ( pLog ) pLog->LogError( "Invalid radius" );
            return false;
        }

        if ( m_halfHeight <= 0.0f )
        {
            if ( pLog ) pLog->LogError( "Invalid half-height" );
            return false;
        }

        if ( m_density <= 0.0f )
        {
            if ( pLog ) pLog->LogError( "Invalid density" );
            return false;
        }

        return true;
    }

    #if EE_DEVELOPMENT_TOOLS
    void RagdollShapeDefinition::DrawDebug( DebugDrawContext& ctx, Transform const& bodyTransform, Color color, float thickness ) const
    {
        Transform const shapeTransform = m_offsetTransform * bodyTransform;

        switch ( m_type )
        {
            case RagdollShapeDefinition::Type::Box:
            {
                ctx.DrawWireBox( shapeTransform, m_boxExtents, color, thickness );
            }
            break;

            case RagdollShapeDefinition::Type::Sphere:
            {
                ctx.DrawWireSphere( shapeTransform, m_radius, color, thickness );
            }
            break;

            case RagdollShapeDefinition::Type::Capsule:
            {
                ctx.DrawWireCapsule( shapeTransform, m_radius, m_halfHeight, color, thickness );
            }
            break;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    bool RagdollBodyDefinition::IsValid( Log* pLog ) const
    {
        if ( !m_boneID.IsValid() )
        {
            if ( pLog ) pLog->LogError( "Invalid bone ID" );
            return false;
        }

        if ( m_shapes.empty() )
        {
            if ( pLog ) pLog->LogError( "No shapes set (%s)", m_boneID.c_str() );
            return false;
        }

        if ( !m_joint.IsValid( pLog ) )
        {
            if ( pLog ) pLog->LogError( "Invalid joint (%s)", m_boneID.c_str() );
            return false;
        }

        if ( !Math::IsInRangeInclusive( m_friction, 0.0f, 1.0f ) )
        {
            if ( pLog ) pLog->LogError( "Invalid friction (%s)", m_boneID.c_str() );
            return false;
        }

        if ( !Math::IsInRangeInclusive( m_restitution, 0.0f, 1.0f ) )
        {
            if ( pLog ) pLog->LogError( "Invalid restitution (%s)", m_boneID.c_str() );
            return false;
        }

        if ( !Math::IsInRangeInclusive( m_rollingResistance, 0.0f, 1.0f ) )
        {
            if ( pLog ) pLog->LogError( "Invalid rolling resistance (%s)", m_boneID.c_str() );
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

     bool RagdollDisableCollisionRule::IsValid() const
    {
        if ( !m_boneA.IsValid() || !m_boneB.IsValid() )
        {
            return false;
        }

        if ( m_boneA == m_boneB )
        {
            return false;
        }

        return true;
    }

    bool RagdollDisableCollisionRule::operator==( RagdollDisableCollisionRule const& rhs ) const
    {
        if ( m_boneA == rhs.m_boneA && m_boneB == rhs.m_boneB )
        {
            return true;
        }

        if ( m_boneA == rhs.m_boneB && m_boneB == rhs.m_boneA )
        {
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    bool RagdollDefinition::IsValid() const
    {
        if ( !m_skeleton.IsLoaded() )
        {
            return false;
        }

        return PerformValidation();
    }

    bool RagdollDefinition::PerformValidation( Log* pLog ) const
    {
        if ( m_bodies.empty() )
        {
            return false;
        }

        for ( RagdollBodyDefinition const &body : m_bodies )
        {
            if ( !body.IsValid( pLog ) )
            {
                return false;
            }
        }

        for ( auto const& rule : m_disableCollisionRules )
        {
            if ( !rule.m_boneA.IsValid() || !rule.m_boneB.IsValid() )
            {
                if ( pLog ) pLog->LogError( "Invalid disable collision rules" );
                return false;
            }

            if ( GetBodyIndexForBoneID( rule.m_boneA ) == InvalidIndex )
            {
                if ( pLog ) pLog->LogError( "Invalid bone ID (%s) in self-collision rules", rule.m_boneA.c_str() );
                return false;
            }

            if ( GetBodyIndexForBoneID( rule.m_boneA ) == InvalidIndex )
            {
                if ( pLog ) pLog->LogError( "Invalid bone ID (%s) in self-collision rules", rule.m_boneB.c_str() );
                return false;
            }
        }

        return true;
    }

    void RagdollDefinition::Finalize()
    {
        EE_ASSERT( m_skeleton.IsLoaded() );

        int32_t const numBones = m_skeleton->GetNumBones();
        int32_t const numBodies = (int32_t) m_bodies.size();

        TInlineVector<Transform, 30> inverseBodyTransforms;

        m_hasKinematicBodies = false;

        // Calculate bone mappings and runtime helper transforms
        //-------------------------------------------------------------------------

        m_boneToBodyMap.clear();
        m_bodyToBoneMap.clear();

        m_boneToBodyMap.resize( numBones, InvalidIndex );
        m_bodyToBoneMap.resize( numBodies, InvalidIndex );
        inverseBodyTransforms.resize( numBodies );

        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            RagdollBodyDefinition& body = m_bodies[bodyIdx];

            if ( body.m_isKinematic )
            {
                m_hasKinematicBodies = true;
            }

            // Get initial transform
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                auto const& boneID = m_skeleton->GetBoneID( boneIdx );
                if ( body.m_boneID == boneID )
                {
                    body.m_initialGlobalTransform = m_skeleton->GetBoneModelSpaceTransform( boneIdx );

                    m_boneToBodyMap[boneIdx] = bodyIdx;
                    m_bodyToBoneMap[bodyIdx] = boneIdx;

                    break;
                }
            }

            inverseBodyTransforms[bodyIdx] = m_bodies[bodyIdx].m_initialGlobalTransform.GetInverse();
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
        }

        // Joints
        //-------------------------------------------------------------------------

        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            if ( m_bodies[bodyIdx].m_joint.m_type != RagdollJointDefinition::Type::None )
            {
                EE_ASSERT( m_bodies[bodyIdx].m_parentBodyIdx != InvalidIndex );
                Transform const transformA = m_bodies[m_bodies[bodyIdx].m_parentBodyIdx].m_initialGlobalTransform;
                Transform const transformB = m_bodies[bodyIdx].m_initialGlobalTransform;
                m_bodies[bodyIdx].m_joint.FinalizeJoint( transformA, transformB );
            }
        }

        // Disable collision rules
        //-------------------------------------------------------------------------

        for ( auto &rule : m_disableCollisionRules )
        {
            rule.m_bodyIdxA = GetBodyIndexForBoneID( rule.m_boneA );
            rule.m_bodyIdxB = GetBodyIndexForBoneID( rule.m_boneB );
            EE_ASSERT( rule.m_bodyIdxA != InvalidIndex );
            EE_ASSERT( rule.m_bodyIdxB != InvalidIndex );
            EE_ASSERT( rule.m_bodyIdxA != rule.m_bodyIdxB );
        }

        // Validate body parenting
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        int32_t numRootBodies = 0;
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            EE_ASSERT( m_bodies[bodyIdx].m_boneID.IsValid() );
            m_bodies[bodyIdx].m_debugName = m_bodies[bodyIdx].m_boneID.c_str();

            EE_ASSERT( m_bodies[bodyIdx].m_parentBodyIdx != bodyIdx );

            if ( m_bodies[bodyIdx].m_parentBodyIdx == InvalidIndex )
            {
                numRootBodies++;
            }
        }
        #endif
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
}