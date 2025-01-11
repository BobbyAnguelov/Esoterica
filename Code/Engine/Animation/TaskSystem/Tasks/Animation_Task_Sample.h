#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class SampleTask : public Task
    {
        EE_REFLECT_TYPE( SampleTask );

    public:

        SampleTask( AnimationClip const* pAnimation, Percentage time );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Sample"; }
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
        virtual Color GetDebugColor() const override { return Colors::Yellow; }
        virtual float GetDebugProgressOrWeight() const override{ return m_time.ToFloat(); }
        #endif

    private:

        SampleTask() : Task() {}

    private:

        AnimationClip const*    m_pAnimation;
        Percentage              m_time;
    };
}