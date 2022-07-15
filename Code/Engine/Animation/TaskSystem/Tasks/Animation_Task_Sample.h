#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class SampleTask : public Task
    {

    public:

        SampleTask( TaskSourceID sourceID, AnimationClip const* pAnimation, Percentage time );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual Color GetDebugColor() const { return Colors::SpringGreen; }
        virtual String GetDebugText() const override;
        #endif

    private:

        AnimationClip const*    m_pAnimation;
        Percentage              m_time;
    };
}