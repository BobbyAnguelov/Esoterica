#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class ReferencePoseTask : public Task
    {
        EE_REFLECT_TYPE( ReferencePoseTask );

    public:

        ReferencePoseTask( TaskSourceID sourceID ) : Task( sourceID ) {}
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return true; }

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightYellow; }
        #endif

    private:

        ReferencePoseTask() : Task( 0xFF ) {}
    };

    //-------------------------------------------------------------------------

    class ZeroPoseTask : public Task
    {
        EE_REFLECT_TYPE( ZeroPoseTask );

    public:

        ZeroPoseTask( TaskSourceID sourceID ) : Task( sourceID ) {}
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return true; }

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightGray; }
        #endif

    private:

        ZeroPoseTask() : Task( 0xFF ) {}
    };
}