#include "IKRig.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/ThirdParty/RKIK/rksolver.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_FORCE_INLINE RkVector3 ToRk( Vector const& v )
    {
        return RkVector3( v.GetX(), v.GetY(), v.GetZ() );
    }

    EE_FORCE_INLINE Vector FromRk( RkVector3 const& v )
    {
        return Vector( v.X, v.Y, v.Z );
    }

    EE_FORCE_INLINE RkQuaternion ToRk( Quaternion const& q )
    {
        return RkQuaternion( q.GetX(), q.GetY(), q.GetZ(), q.GetW() );
    }

    EE_FORCE_INLINE Transform FromRk( RkTransform const& t )
    {
        return Transform( Quaternion( t.Rotation.X, t.Rotation.Y, t.Rotation.Z, t.Rotation.W ), Vector( t.Translation.X, t.Translation.Y, t.Translation.Z ) );
    }

    EE_FORCE_INLINE RkTransform ToRk( Transform const& t )
    {
        RkTransform rT;
        rT.Rotation = ToRk( t.GetRotation() );
        rT.Translation = ToRk( t.GetTranslation() );
        return rT;
    }

    //-------------------------------------------------------------------------

    bool IKRigDefinition::IsValid() const
    {
        if ( !m_skeleton.IsSet() )
        {
            return false;
        }

        if ( m_iterations <= 0 )
        {
            return false;
        }

        if ( m_links.empty() )
        {
            return false;
        }

        for ( auto const& link : m_links )
        {
            if ( !link.IsValid() )
            {
                return false;
            }
        }

        return true;
    }

    void IKRigDefinition::CreateRuntimeData()
    {
        EE_ASSERT( m_skeleton.IsLoaded() );

        int32_t const numBones = m_skeleton->GetNumBones();
        int32_t const numBodies = (int32_t) m_links.size();

        //-------------------------------------------------------------------------

        m_numEffectors = 0;

        for ( auto const& link : m_links )
        {
            m_numEffectors += link.m_isEffector ? 1 : 0;
        }

        //-------------------------------------------------------------------------

        m_bodyToBoneMap.clear();
        m_bodyToBoneMap.resize( numBodies, InvalidIndex );

        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            m_bodyToBoneMap[bodyIdx] = m_skeleton->GetBoneIndex( m_links[bodyIdx].m_boneID );
        }

        //-------------------------------------------------------------------------

        m_boneToBodyMap.clear();
        m_boneToBodyMap.resize( numBones, InvalidIndex );

        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
            {
                if ( m_links[bodyIdx].m_boneID == m_skeleton->GetBoneID( boneIdx ) )
                {
                    m_boneToBodyMap[boneIdx] = bodyIdx;
                    break;
                }
            }
        }
    }

    int32_t IKRigDefinition::GetLinkIndexForBoneID( StringID boneID ) const
    {
        EE_ASSERT( boneID.IsValid() );

        int32_t const numBodies = (int32_t) m_links.size();
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            if ( m_links[bodyIdx].m_boneID == boneID )
            {
                return bodyIdx;
            }
        }

        return InvalidIndex;
    }

    int32_t IKRigDefinition::GetParentLinkIndexForBoneID( StringID boneID ) const
    {
        EE_ASSERT( m_skeleton.IsLoaded() );

        int32_t const boneIdx = m_skeleton->GetBoneIndex( boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
        EE_ASSERT( parentBoneIdx != InvalidIndex );

        StringID const parentBoneID = m_skeleton->GetBoneID( parentBoneIdx );
        return GetLinkIndexForBoneID( parentBoneID );
    }

    int32_t IKRigDefinition::GetLinkIndexForEffector( int32_t effectorIdx ) const
    {
        EE_ASSERT( effectorIdx >= 0 );

        int32 i = 0;
        int32_t const numBodies = GetNumLinks();
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            if ( m_links[bodyIdx].m_isEffector )
            {
                if ( effectorIdx == i )
                {
                    return bodyIdx;
                }
                else
                {
                    i++;
                }
            }
        }

        return InvalidIndex;
    }

    int32_t IKRigDefinition::GetParentLinkIndex( int32_t linkIdx ) const
    {
        EE_ASSERT( m_skeleton.IsLoaded() );

        EE_ASSERT( linkIdx >= 0 && linkIdx < m_links.size() );

        int32_t const boneIdx = m_skeleton->GetBoneIndex( m_links[linkIdx].m_boneID );

        // Traverse the hierarchy to try to find the first parent bone that has a body
        int32_t parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
        while ( parentBoneIdx != InvalidIndex )
        {
            // Do we have a body for this parent bone?
            StringID const parentBoneID = m_skeleton->GetBoneID( parentBoneIdx );
            for ( int32_t i = 0; i < m_links.size(); i++ )
            {
                if ( m_links[i].m_boneID == parentBoneID )
                {
                    return i;
                }
            }

            parentBoneIdx = m_skeleton->GetParentBoneIndex( parentBoneIdx );
        }

        return InvalidIndex;
    }

    Vector IKRigDefinition::CalculateCOM( int32_t linkIdx ) const
    {
        IKRigDefinition::Link const& link = m_links[linkIdx];

        int32_t const boneIdx = m_skeleton->GetBoneIndex( link.m_boneID );
        int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
        EE_ASSERT( parentBoneIdx != InvalidIndex );

        Transform const boneTransform = m_skeleton->GetBoneModelSpaceTransform( boneIdx );
        Transform const parentBoneTransform = m_skeleton->GetBoneModelSpaceTransform( parentBoneIdx );

        Vector boneDirection;
        float length = 0.0f;
        ( parentBoneTransform.GetTranslation() - boneTransform.GetTranslation() ).ToDirectionAndLength3( boneDirection, length );
        Vector const com = boneTransform.InverseTransformPoint( boneTransform.GetTranslation() + ( boneDirection * ( length / 2.0f ) ) );

        return com;
    }

    //-------------------------------------------------------------------------

    IKRig::IKRig( IKRigDefinition const* pDefinition )
        : m_pDefinition( pDefinition )
    {
        EE_ASSERT( m_pDefinition != nullptr && m_pDefinition->IsValid() );

        // Setup bodies and effectors
        //-------------------------------------------------------------------------

        int32_t effectorIdx = 0;
        m_pEffectors = EE::New< RkArray<RkIkEffector>>( m_pDefinition->GetNumEffectors() );

        RkArray< RkIkBody > bodies;
        int32_t const numBodies = m_pDefinition->GetNumLinks();
        for ( int32_t linkIdx = 0; linkIdx < numBodies; linkIdx++ )
        {
            IKRigDefinition::Link const& link = m_pDefinition->m_links[linkIdx];

            // Setup body
            RkIkBody body;
            body.CenterOfMass = ToRk( m_pDefinition->CalculateCOM( linkIdx ) );
            body.Mass = link.m_mass;
            body.Radius = ToRk( link.m_radius );
            bodies.PushBack( body );

            // Setup effector
            if ( link.m_isEffector )
            {
                ( *m_pEffectors )[effectorIdx].BodyIndex = linkIdx;
                effectorIdx++;
            }
        }

        // Allocate storage for body transforms
        m_pBodyTransforms = EE::New< RkArray<RkTransform>>( numBodies );

        // Setup Joints
        //-------------------------------------------------------------------------
        // Note: root does not have a joint!

        RkArray< RkIkJoint > joints;
        for ( int32_t linkIdx = 1; linkIdx < numBodies; linkIdx++ )
        {
            IKRigDefinition::Link const& link = m_pDefinition->m_links[linkIdx];

            int32_t const bodyIdx = pDefinition->GetLinkIndexForBoneID( link.m_boneID );
            int32_t const boneIdx = m_pDefinition->m_bodyToBoneMap[linkIdx];
            Transform const bodyTransform = m_pDefinition->m_skeleton->GetBoneModelSpaceTransform( boneIdx );

            int32_t const parentBodyIdx = pDefinition->GetParentLinkIndexForBoneID( link.m_boneID );
            int32_t const parentBoneIdx = m_pDefinition->m_bodyToBoneMap[parentBodyIdx];
            Transform const parentBodyTransform = m_pDefinition->m_skeleton->GetBoneModelSpaceTransform( parentBoneIdx );

            RkIkJoint joint;
            joint.ParentBodyIndex = parentBodyIdx;
            joint.LocalFrame = ToRk( Transform::Delta( parentBodyTransform, bodyTransform ) );
            joint.BodyIndex = bodyIdx;

            joint.SwingLimit = link.m_swingLimit.ToFloat();
            joint.MinTwistLimit = link.m_minTwistLimit.ToFloat();
            joint.MaxTwistLimit = link.m_maxTwistLimit.ToFloat();

            joints.PushBack( joint );
        }

        // Create Solver
        //-------------------------------------------------------------------------

        m_pSolver = EE::New<RkSolver>( bodies, joints );
    }

    IKRig::~IKRig()
    {
        EE::Delete( m_pEffectors );
        EE::Delete( m_pBodyTransforms );
        EE::Delete( m_pSolver );
    }

    void IKRig::Solve( Pose* pPose )
    {
        if ( m_pSolver == nullptr )
        {
            return;
        }

        EE_ASSERT( pPose != nullptr && pPose->IsPoseSet() );
        EE_ASSERT( pPose->GetSkeleton() == m_pDefinition->GetSkeleton() );
        EE_ASSERT( pPose->HasModelSpaceTransforms() );

        // Transfer animation transforms to bodies
        //-------------------------------------------------------------------------

        m_modelSpacePoseTransforms.resize( pPose->GetNumBones() );
        memcpy( m_modelSpacePoseTransforms.data(), pPose->GetModelSpaceTransforms().data(), pPose->GetNumBones() * sizeof( Transform ) );

        int32_t const numBodies = m_pDefinition->GetNumLinks();
        for ( int32 bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            Transform const& boneTransform = m_modelSpacePoseTransforms[ m_pDefinition->m_bodyToBoneMap[bodyIdx] ];
            ( *m_pBodyTransforms )[bodyIdx].Rotation = ToRk( boneTransform.GetRotation() );
            ( *m_pBodyTransforms )[bodyIdx].Translation = ToRk( boneTransform.GetTranslation() );
        }

        // Run Solver
        //-------------------------------------------------------------------------

        m_pSolver->Solve( *m_pEffectors, *m_pBodyTransforms, m_pDefinition->m_iterations );

        // Copy the results back
        //-------------------------------------------------------------------------

        int32_t const numBones = pPose->GetNumBones();
        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            // Keep original local transforms for non-body bones
            int32_t const bodyIdx = m_pDefinition->m_boneToBodyMap[boneIdx];
            if ( bodyIdx == InvalidIndex )
            {
                int32_t const parentBoneIdx = m_pDefinition->m_skeleton->GetParentBoneIndex( boneIdx );
                if ( parentBoneIdx != InvalidIndex )
                {
                    m_modelSpacePoseTransforms[boneIdx] = pPose->GetTransform( boneIdx ) * m_modelSpacePoseTransforms[parentBoneIdx];
                }
                else
                {
                    m_modelSpacePoseTransforms[boneIdx] = pPose->GetTransform( boneIdx );
                }
            }
            else // Update transform
            {
                m_modelSpacePoseTransforms[boneIdx] = FromRk( ( *m_pBodyTransforms )[bodyIdx] );
            }
        }

        pPose->ClearModelSpaceTransforms();
        TVector<Transform> const& parentSpaceTransforms = pPose->GetTransforms();

        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            int32_t const parentBoneIdx = m_pDefinition->m_skeleton->GetParentBoneIndex( boneIdx );
            if ( parentBoneIdx != InvalidIndex )
            {
                pPose->SetTransform( boneIdx, m_modelSpacePoseTransforms[boneIdx] * m_modelSpacePoseTransforms[parentBoneIdx].GetInverse() );
            }
            else
            {
                pPose->SetTransform( boneIdx, m_modelSpacePoseTransforms[boneIdx] );
            }
        }

        // Disable all effectors
        //-------------------------------------------------------------------------

        int32_t const numEffectors = (int32_t) m_pEffectors->Size();
        for ( int32 effectorIdx = 0; effectorIdx < numEffectors; effectorIdx++ )
        {
            ( *m_pEffectors )[effectorIdx].Enabled = false;
        }
    }

    void IKRig::SetEffectorTarget( int32_t effectorIdx, Transform target )
    {
        EE_ASSERT( m_pSolver != nullptr );
        int32_t const numEffectors = (int32_t) m_pEffectors->Size();
        EE_ASSERT( effectorIdx >= 0 && effectorIdx < numEffectors );
        RkIkEffector& effector = ( *m_pEffectors )[effectorIdx];

        effector.TargetOrientation = ToRk( target.GetRotation() );
        effector.TargetPosition = ToRk( target.GetTranslation() );
        effector.Enabled = true;
    }

    #if EE_DEVELOPMENT_TOOLS
    void IKRig::DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform ) const
    {
        EE_ASSERT( m_pBodyTransforms != nullptr );
        EE_ASSERT( m_pEffectors != nullptr );

        //-------------------------------------------------------------------------

        RkArray<RkTransform> const& bodyTransforms = *m_pBodyTransforms;
        int32_t const numBodies = m_pDefinition->GetNumLinks();
        for ( int32_t i = 0; i < numBodies; i++ )
        {
            int32_t const parentBodyIdx = m_pDefinition->GetParentLinkIndex( i );

            Transform const bodyTransform = FromRk( bodyTransforms[i] );
            Transform const parentTransform = ( parentBodyIdx != InvalidIndex ) ? FromRk( bodyTransforms[parentBodyIdx] ) : Transform::Identity;

            // Draw body
            //-------------------------------------------------------------------------

            RkIkBody const& body = m_pSolver->GetBodies()[i - 1];
            Vector const com = bodyTransform.TransformPoint( FromRk( body.CenterOfMass ) );

            ctx.DrawLine( bodyTransform.GetTranslation(), parentTransform.GetTranslation(), Colors::Cyan, 5.0f );
            ctx.DrawAxis( bodyTransform, Colors::HotPink, Colors::SeaGreen, Colors::RoyalBlue, 0.035f, 4.0f );
            ctx.DrawPoint( com, Colors::Yellow, 15.0f );

            // Draw joint
            //-------------------------------------------------------------------------

            if ( i == 0 )
            {
                continue;
            }

            RkIkJoint const& joint = m_pSolver->GetJoints()[i - 1];
            ctx.DrawAxis( parentTransform, 0.025f, 6.0f );

            //ctx.DrawTwistLimits( jointTransform, m_pDefinition->m_links[i].m_minTwistLimit, m_pDefinition->m_links[i].m_maxTwistLimit, Colors::Yellow, 3.0f, 0.01f );
            //ctx.DrawSwingLimits( jointTransform, m_pDefinition->m_links[i].m_swingLimit, m_pDefinition->m_links[i].m_swingLimit, Colors::Orange, 3.0f, 0.02f );
        }
    }
    #endif
}