#include "Animation_Task_ChainSolver.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Engine/Animation/IK/IKChainSolver.h"
#include "Base/Profiling.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/Plane.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    ChainSolverTask::ChainSolverTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, int32_t chainLength, bool isTargetInWorldSpace, Target const& effectorTarget )
        : Task( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_effectorBoneIdx( effectorBoneIdx )
        , m_chainLength( chainLength )
        , m_effectorTarget( effectorTarget )
        , m_isTargetInWorldSpace( isTargetInWorldSpace )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( m_effectorBoneIdx != InvalidIndex );
        EE_ASSERT( m_effectorTarget.IsTargetSet() );
    }

    void ChainSolverTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pPose = pSourceBuffer->GetPrimaryPose();
        auto pSkeleton = pPose->GetSkeleton();
        int32_t const numBones = pSkeleton->GetNumBones();
        EE_ASSERT( m_effectorBoneIdx < numBones );

        // Set up bone indices for the chain
        //-------------------------------------------------------------------------

        TInlineVector<int32_t, 12> boneIndices;
        boneIndices.reserve( m_chainLength );

        boneIndices.emplace_back( m_effectorBoneIdx );
        int32_t parentIdx = pSkeleton->GetParentBoneIndex( m_effectorBoneIdx );
        while ( parentIdx != InvalidIndex && boneIndices.size() < m_chainLength )
        {
            boneIndices.emplace_back( parentIdx );
            parentIdx = pSkeleton->GetParentBoneIndex( parentIdx );
        }

        eastl::reverse( boneIndices.begin(), boneIndices.end() );

        // Setup nodes
        //-------------------------------------------------------------------------

        int32_t const numNodes = (int32_t) boneIndices.size();
        int32_t const startBoneIdx = boneIndices[0];

        TInlineVector<IKChainSolver::ChainNode,10> nodes;
        nodes.resize( numNodes );

        // Get the parent of the start of the chain, this is needed to calculate the local transforms
        int32_t const baseParentIndex = pSkeleton->GetParentBoneIndex( startBoneIdx );
        Transform const baseParentTransform = ( baseParentIndex != InvalidIndex ) ? pPose->GetModelSpaceTransform( baseParentIndex ) : Transform::Identity;

        float totalChainLength = 0.0f;
        Transform prevTransform = baseParentTransform;
        for ( int32_t i = 0; i < numNodes; i++ )
        {
            Transform const& parentSpaceTransform = pPose->GetTransform( boneIndices[i] );

            nodes[i].m_transform = parentSpaceTransform * prevTransform;
            nodes[i].m_weight = 1.0f;

            totalChainLength += parentSpaceTransform.GetTranslation().GetLength3();
            prevTransform = nodes[i].m_transform;
        }

        #if EE_DEVELOPMENT_TOOLS
        m_chainStartTransformMS = nodes[0].m_transform;
        #endif

        // Set up target transform
        //-------------------------------------------------------------------------

        if ( !m_isRunningFromDeserializedData )
        {
            bool const result = m_effectorTarget.TryGetTransform( pPose, m_targetTransform );
            EE_ASSERT( result );

            // Convert to character space
            if ( !m_effectorTarget.IsBoneTarget() && m_isTargetInWorldSpace )
            {
                m_targetTransform = m_targetTransform * context.m_worldTransformInverse;
            }

            Vector dir;
            float length;
            ( m_targetTransform.GetTranslation() - nodes[0].m_transform.GetTranslation() ).ToDirectionAndLength3( dir, length );

            if ( length > totalChainLength )
            {
                Vector const clampedEffectorTargetPos = nodes[0].m_transform.GetTranslation() + ( dir * totalChainLength );
                m_targetTransform.SetTranslation( clampedEffectorTargetPos );
            }

            #if EE_DEVELOPMENT_TOOLS
            m_debugRequestedTargetTransformMS = m_targetTransform;
            m_debugTotalChainLength = totalChainLength;
            #endif
        }

        // Perform solve
        //-------------------------------------------------------------------------

        IKChainSolver::SolveChain( nodes, m_targetTransform, 0 );

        // Set pose to the solved transforms
        //-------------------------------------------------------------------------

        // Compute relative transforms in reverse order
        for ( int32_t i = numNodes - 1; i > 0; i-- )
        {
            pPose->SetTransform( boneIndices[i], nodes[i].m_transform.GetDeltaFromOther( nodes[i - 1].m_transform ) );
        }

        pPose->SetTransform( startBoneIdx, nodes[0].m_transform.GetDeltaFromOther( baseParentTransform ) );
        pPose->ClearModelSpaceTransforms();

        //-------------------------------------------------------------------------

        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString ChainSolverTask::GetDebugTextInfo( bool isDetailedModeEnabled ) const
    {
        return InlineString( InlineString::CtorSprintf(), "Chain Solver: %s", m_debugEffectorBoneID.c_str() );
    }

    void ChainSolverTask::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const
    {
        Task::DrawDebug( drawingContext, worldTransform, lod, pRecordedPoseBuffer, isDetailedViewEnabled );

        if ( isDetailedViewEnabled && !m_isRunningFromDeserializedData )
        {
            Transform tmp = m_targetTransform * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 4.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Actual Target", Colors::LimeGreen );

            tmp = m_debugRequestedTargetTransformMS * worldTransform;
            drawingContext.DrawAxis( tmp, 0.1f, 3.0f );
            drawingContext.DrawTextBox3D( tmp.GetTranslation(), "Requested Target", Colors::Yellow );

            tmp = m_chainStartTransformMS * worldTransform;
            drawingContext.DrawSphere( tmp, m_debugTotalChainLength, Colors::Green );
;       }
    }
    #endif

    void ChainSolverTask::Serialize( TaskSerializer& serializer ) const
    {
        EE_ASSERT( !m_isRunningFromDeserializedData );
        EE_ASSERT( Math::GetMaxNumberOfBitsForValue( m_chainLength ) <= 5 );

        serializer.WriteDependencyIndex( m_dependencies[0] );
        serializer.WriteBoneIndex( m_effectorBoneIdx );
        serializer.WriteRotation( m_targetTransform.GetRotation() );
        serializer.WriteTranslation( m_targetTransform.GetTranslation() );

        serializer.WriteUInt( m_chainLength, 5 );
    }

    void ChainSolverTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 1 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
        m_effectorBoneIdx = serializer.ReadBoneIndex();

        Quaternion const q = serializer.ReadRotation();
        Float3 const t = serializer.ReadTranslation();
        m_targetTransform = Transform( q, t );

        m_chainLength = (uint32_t) serializer.ReadUInt( 5 );

        m_isRunningFromDeserializedData = true;
    }
}