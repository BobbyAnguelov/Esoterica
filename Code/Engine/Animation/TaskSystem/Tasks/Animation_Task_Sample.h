#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class SampleTask : public Task
    {
        EE_ANIMATION_TASK( SampleTask );

    public:

        SampleTask( TaskSourceID sourceID, AnimationClip const* pAnimation, Percentage time );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual Color GetDebugColor() const { return Colors::SpringGreen; }
        virtual String GetDebugText() const override;
        #endif

    private:

        SampleTask() : Task( 0xFF ) {}

    private:

        AnimationClip const*    m_pAnimation;
        Percentage              m_time;
    };
}