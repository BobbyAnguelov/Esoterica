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

    #if EE_DEVELOPMENT_TOOLS
    void Task::DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const
    {
        pRecordedPose->DrawDebug( drawingContext, worldTransform, GetDebugColor(), 3.0f );
    }
    #endif
}