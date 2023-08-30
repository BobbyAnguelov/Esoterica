#include "Animation_Task_LookAtIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSerializer.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    LookAtIKTask::LookAtIKTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, Vector const& worldSpaceTarget )
        : Task( sourceID, TaskUpdateStage::Any, { sourceTaskIdx } )
        , m_worldSpaceTarget( worldSpaceTarget )
    {
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
    }

    void LookAtIKTask::Execute( TaskContext const& context )
    {
        //EE_PROFILE_FUNCTION_ANIMATION();
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );

        // Generate Effector Data
        //-------------------------------------------------------------------------

        if ( m_generateEffectorData )
        {
            m_characterSpaceTarget = context.m_worldTransformInverse.TransformVector( m_worldSpaceTarget );

            // project onto sphere, relative to the head height and clamp angles

            // Normalize vector to projected value relative to head height
        }

        // Do IKz
        //-------------------------------------------------------------------------

        // Nothing to "see" here... Move along :P

        //-------------------------------------------------------------------------

        MarkTaskComplete( context );
    }

    void LookAtIKTask::Serialize( TaskSerializer& serializer ) const
    {
        serializer.WriteDependencyIndex( m_dependencies[0] );

        //serializer.WriteFloat( m_characterSpaceTarget.GetX() );
        //serializer.WriteFloat( m_characterSpaceTarget.GetY() );
        //serializer.WriteFloat( m_characterSpaceTarget.GetZ() );
    }

    void LookAtIKTask::Deserialize( TaskSerializer& serializer )
    {
        m_dependencies.resize( 1 );
        m_dependencies[0] = serializer.ReadDependencyIndex();

        m_generateEffectorData = false;
    }
}