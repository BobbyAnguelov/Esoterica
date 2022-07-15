#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class DefaultPoseTask : public Task
    {

    public:

        DefaultPoseTask( TaskSourceID sourceID, Pose::Type type );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const { return Colors::LightGray; }
        #endif

    private:

        Pose::Type m_type;
    };
}