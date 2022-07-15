#include "Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    Task::Task( TaskSourceID sourceID, TaskUpdateStage updateRequirements, TaskDependencies const& dependencies )
        : m_sourceID( sourceID )
        , m_updateStage( updateRequirements )
        , m_dependencies( dependencies )
    {
        EE_ASSERT( sourceID != InvalidIndex );
    }
}