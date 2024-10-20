#include "Animation_Task_IK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Engine/Animation/IK/IKRig.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    IKRigTask::IKRigTask( int8_t sourceTaskIdx, IKRig* pRig, TInlineVector<Target, 6> const& targets )
        : Task( TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_pRig( pRig )
        , m_effectorTargets( targets )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
        EE_ASSERT( pRig != nullptr );
    }

    void IKRigTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();

        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        auto pPose = pSourceBuffer->GetPrimaryPose();
        pPose->CalculateModelSpaceTransforms();

        for ( int32_t i = 0; i < m_effectorTargets.size(); i++ )
        {
            Transform targetTransform;
            if ( m_effectorTargets[i].IsTargetSet() && m_effectorTargets[i].TryGetTransform( pPose, targetTransform ) )
            {
                m_pRig->SetEffectorTarget( i, targetTransform );
            }
        }

        m_pRig->Solve( pPose );

        //-------------------------------------------------------------------------

        MarkTaskComplete( context );
    }

    void IKRigTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );
    }

    void IKRigTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 1 );
        m_dependencies[0] = serializer.ReadDependencyIndex();
    }
}