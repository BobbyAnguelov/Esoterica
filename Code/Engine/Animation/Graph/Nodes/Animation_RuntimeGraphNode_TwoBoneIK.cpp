#include "Animation_RuntimeGraphNode_TwoBoneIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_TwoBoneIK.h"

//-------------------------------------------------------------------------

//{
//    /*
// * A		: Copy of ModelSpace Pose of Joint A
// * B		: Copy of ModelSpace Pose of Joint B
// * C		: Copy of ModelSpace Pose of Joint C (Current End Effector)
// * T		: Copy of ModelSpace Pose of the Target for Joint C (Target End Effector)
// * ALocal	: Reference to the Local Pose of Joint A -> to be updated;
// * BLocal	: Reference to the Local Pose of Joint B -> to be updated;
// * CLocal	: Reference to the Local Pose of Joint C -> to be updated;
//*/
//void TwoJointInverseKinematics( Transform A, Transform B, Transform C, Transform T,
//    Transform& ALocal, Transform& BLocal, Transform& CLocal )
//{
//
//    // WARNING : The parity of the resulting vector from Vector3CrossProduct (is it v or -v?) depends on the "handedness" of the implementation
//    // If you are not getting the expected output, try to swap : C = Vector3CrossProduct(A, B) to C = Vector3CrossProduct(B, A) wherever Vector3CrossProduct is used.
//
//    // Reach Effector Location : Update ALocal and BLocal
//    {
//        float ik_eps = 0; // Epsilon to prevent full extension (if desired)
//
//        float lenAB = Vector3Length( Vector3Subtract( B.translation, A.translation ) ); // Can be precomputed and passed as an argument since it should be constant
//        float lenBC = Vector3Length( Vector3Subtract( C.translation, B.translation ) ); // Can be precomputed and passed as an argument since it should be constant
//        float lenAT = std::clamp( Vector3Length( Vector3Subtract( T.translation, A.translation ) ), ik_eps, lenAB + lenBC - ik_eps );
//
//        // Precompute directions to avoid any redundancies
//        Vector3 A_to_C_direction = Vector3Normalize( Vector3Subtract( C.translation, A.translation ) );
//        Vector3 A_to_B_direction = Vector3Normalize( Vector3Subtract( B.translation, A.translation ) );
//        Vector3 B_to_A_direction = Vector3Normalize( { -A_to_B_direction.x, -A_to_B_direction.y, -A_to_B_direction.z } );
//        Vector3 B_to_C_direction = Vector3Normalize( Vector3Subtract( C.translation, B.translation ) );
//        Vector3 A_to_T_direction = Vector3Normalize( Vector3Subtract( T.translation, A.translation ) );
//
//        // Angle Between AC and AB
//        float ac_ab_0 = std::acosf(
//            std::clamp(
//            Vector3DotProduct(
//            A_to_C_direction,
//            A_to_B_direction
//        ),
//            -1.0f, 1.0f )
//        );
//
//        // Angle Between BA and BC
//        float ba_bc_0 = std::acosf(
//            std::clamp(
//            Vector3DotProduct(
//            B_to_A_direction,
//            B_to_C_direction
//        ),
//            -1.0f, 1.0f
//        )
//        );
//
//        // Angle Between AC and AT
//        float ac_at_0 = std::acosf(
//            std::clamp(
//            Vector3DotProduct(
//            A_to_C_direction,
//            A_to_T_direction
//        ),
//            -1.0f, 1.0f
//        )
//        );
//
//        // Target Angle Between AC and AB
//        float ac_ab_1 = std::acosf(
//            std::clamp(
//            ( ( ( lenBC * lenBC ) - ( lenAB * lenAB ) - ( lenAT * lenAT ) ) / ( -2.0f * lenAB * lenAT ) ),
//            -1.0f, 1.0f
//        )
//        );
//
//        // Target Angle Between BA and BC
//        float ba_bc_1 = std::acosf(
//            std::clamp(
//            ( ( ( lenAT * lenAT ) - ( lenAB * lenAB ) - ( lenBC * lenBC ) ) / ( -2.0f * lenAB * lenBC ) ),
//            -1.0f, 1.0f
//        )
//        );
//
//        // Compute Target Angle Differences
//        float r0_angle_diff = ( ac_ab_1 - ac_ab_0 );
//        float r1_angle_diff = ( ba_bc_1 - ba_bc_0 );
//        float r2_angle_diff = ( ac_at_0 );
//
//        Vector3 axis0 = Vector3CrossProduct(
//            A_to_C_direction,
//            A_to_B_direction
//        );
//
//        Vector3 axis1 = Vector3CrossProduct(
//            A_to_C_direction,
//            A_to_T_direction
//        );
//
//        Quaternion r0 = QuaternionFromAxisAngle(
//            Vector3RotateByQuaternion( axis0, QuaternionInvert( A.rotation ) ), r0_angle_diff
//        );
//
//        Quaternion r1 = QuaternionFromAxisAngle(
//            Vector3RotateByQuaternion( axis0, QuaternionInvert( B.rotation ) ), r1_angle_diff
//        );
//
//        Quaternion r2 = QuaternionFromAxisAngle(
//            Vector3RotateByQuaternion( axis1, QuaternionInvert( A.rotation ) ), r2_angle_diff
//        );
//
//        Quaternion r3 = QuaternionMultiply( r2, r0 );
//
//        // Update ALocal and BLocal
//        ALocal.rotation = QuaternionMultiply( ALocal.rotation, r3 );
//        BLocal.rotation = QuaternionMultiply( BLocal.rotation, r1 );
//
//        // End effector now reaches the target location in the world space.
//        // We now want to match the end effector orientation to the target orientation...
//        // But we specifically want to distribute the hinge rotation of the end effector (joint C) to joint A
//        // Overwrite the ModelSpace poses (copies) with the new poses to proceed
//
//        // ModelSpace Updates
//        {
//            // Update A into ModelSpace
//            A.rotation = QuaternionMultiply( A.rotation, r3 );
//
//            // Compose updated B into ModelSpace
//            B = BLocal; // Overwrite with BLocal
//            B.rotation = QuaternionMultiply( A.rotation, B.rotation );
//            B.scale = Vector3Multiply( B.scale, A.scale );
//            B.translation = Vector3Multiply( B.translation, A.scale );
//            B.translation = Vector3RotateByQuaternion( B.translation, A.rotation );
//            B.translation = Vector3Add( B.translation, A.translation );
//
//            // Compose updated C into ModelSpace
//            C = CLocal; // Overwrite with CLocal
//            C.rotation = QuaternionMultiply( B.rotation, C.rotation );
//            C.scale = Vector3Multiply( C.scale, B.scale );
//            C.translation = Vector3Multiply( C.translation, B.scale );
//            C.translation = Vector3RotateByQuaternion( C.translation, B.rotation );
//            C.translation = Vector3Add( C.translation, B.translation );
//        }
//    }
//
//    // Reach Effector Orientation : Update ALocal and CLocal
//    {
//        // Get the rotation difference between the effector and the target
//        Quaternion CT_RotationDelta = QuaternionMultiply( QuaternionInvert( C.rotation ), T.rotation );
//
//        // Identify the axes that span the plane of the arm
//        Vector3 AC_axis = Vector3Normalize( Vector3Subtract( C.translation, A.translation ) ); // Shoulder to Effector
//        Vector3 BC_axis = Vector3Normalize( Vector3Subtract( C.translation, B.translation ) ); // Down the Arm
//
//        // Compute the orthogonal axis of the plane; to then compute the 
//        Vector3 orthogonal_arm_plane_axis = Vector3CrossProduct( AC_axis, BC_axis );
//        // Orthogonalize the hinge axis from the BC_axis
//        Vector3 hinge_axis = Vector3CrossProduct( BC_axis, orthogonal_arm_plane_axis );
//
//        // Put hinge_axis into the local space of C so it can interact with CT_RotationDelta
//        hinge_axis = Vector3RotateByQuaternion( hinge_axis, QuaternionInvert( C.rotation ) );
//
//        // NOTE : We can also just manually provide hinge_axis. The above way attempts to infer it.
//
//        // Given CT_RotationDelta, project the angle of its angle-axis decomposition onto the hinge axis
//        float projection = Vector3DotProduct( hinge_axis, { CT_RotationDelta.x, CT_RotationDelta.y, CT_RotationDelta.z } );
//        float w = CT_RotationDelta.w / std::sqrtf( ( projection * projection ) + ( CT_RotationDelta.w * CT_RotationDelta.w ) );
//        float angle = 2.0f * std::acos( std::clamp( w, -1.0f, 1.0f ) );
//        if ( projection < 0 ) angle = -angle;
//
//        // Derive the quaternion from the AC_axis and the projected angle
//        Quaternion A_roll = QuaternionFromAxisAngle( AC_axis, angle );
//
//        // Apply the roll onto Joint A (Since A_roll is a left multiply, we have to do the expansion below)
//        Quaternion ALocal_RightMultiply = QuaternionMultiply( QuaternionInvert( A.rotation ), QuaternionMultiply( A_roll, A.rotation ) );
//        ALocal.rotation = QuaternionMultiply( ALocal.rotation, ALocal_RightMultiply );
//
//        // Apply Remaining Rotation onto Joint C (Non-Hinge Rotations : Twist and Side-to-Side)
//        Quaternion CLocal_RightMultiply = QuaternionMultiply( QuaternionInvert( QuaternionMultiply( A_roll, C.rotation ) ), T.rotation );
//        CLocal.rotation = QuaternionMultiply( CLocal.rotation, CLocal_RightMultiply );
//    }
//
//}
//}


namespace EE::Animation::GraphNodes
{
    void TwoBoneIKNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TwoBoneIKNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_effectorTargetNodeIdx, pNode->m_pEffectorTarget );
    }

    void TwoBoneIKNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pEffectorTarget->Initialize( context );

        auto pDefinition = GetDefinition<TwoBoneIKNode>();
        EE_ASSERT( pDefinition->m_effectorBoneID.IsValid() );

        // Get and validate effector chain
        //-------------------------------------------------------------------------

        m_effectorBoneIdx = context.m_pSkeleton->GetBoneIndex( pDefinition->m_effectorBoneID );
        if ( m_effectorBoneIdx == InvalidIndex )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Cant find specified bone ID ('%s') for two bone effector node", pDefinition->m_effectorBoneID.c_str() );
            #endif
        }

        // Validate that there are at least two bones in the chain
        int32_t parentCount = 0;
        int32_t parentIdx = context.m_pSkeleton->GetParentBoneIndex( m_effectorBoneIdx );
        while ( parentIdx != InvalidIndex && parentCount < 2 )
        {
            parentCount++;
            parentIdx = context.m_pSkeleton->GetParentBoneIndex( parentIdx );
        }

        if ( parentCount != 2 )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Invalid effector bone ID ('%s') specified, there are not enough bones in the chain to solve the IK request", pDefinition->m_effectorBoneID.c_str() );
            #endif

            m_effectorBoneIdx = InvalidIndex;
        }
    }

    void TwoBoneIKNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        m_effectorBoneIdx = InvalidIndex;
        m_pEffectorTarget->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult TwoBoneIKNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        result = PassthroughNode::Update( context, pUpdateRange );

        if ( result.HasRegisteredTasks() && m_effectorBoneIdx != InvalidIndex )
        {
            Target const effectorTarget = m_pEffectorTarget->GetValue<Target>( context );
            if ( effectorTarget.IsTargetSet() )
            {
                bool shouldRegisterTask = true;

                // Validate input target
                if ( effectorTarget.IsBoneTarget() )
                {
                    StringID const effectorTargetBoneID = effectorTarget.GetBoneID();
                    if ( !effectorTargetBoneID.IsValid() )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogError( GetNodeIndex(), "No effector bone ID specified in input target" );
                        #endif
                        shouldRegisterTask = false;
                    }

                    if( context.m_pSkeleton->GetBoneIndex( effectorTargetBoneID ) == InvalidIndex )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogError( GetNodeIndex(), "Invalid effector bone ID ('%s') specified in input target", effectorTarget.GetBoneID().c_str() );
                        #endif
                        shouldRegisterTask = false;
                    }
                }

                if ( shouldRegisterTask )
                {
                    auto pDefinition = GetDefinition<TwoBoneIKNode>();
                    result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::TwoBoneIKTask>( GetNodeIndex(), result.m_taskIdx, m_effectorBoneIdx, pDefinition->m_allowedStretchPercentage.ToFloat(), pDefinition->m_isTargetInWorldSpace, effectorTarget );
                }
            }
        }

        return result;
    }
}