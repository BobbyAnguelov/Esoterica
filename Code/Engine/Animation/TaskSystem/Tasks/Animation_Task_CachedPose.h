#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class CachedPoseWriteTask : public Task
    {
        EE_REFLECT_TYPE( CachedPoseWriteTask );

    public:

        CachedPoseWriteTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, UUID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return String( "Write Cached Pose" ); }
        virtual Color GetDebugColor() const override { return Colors::Red; }
        #endif

    private:

        CachedPoseWriteTask() : Task(0xFF) {}

    private:

        UUID m_cachedPoseID;
    };

    //-------------------------------------------------------------------------

    class CachedPoseReadTask : public Task
    {
        EE_REFLECT_TYPE( CachedPoseReadTask );

    public:

        CachedPoseReadTask( TaskSourceID sourceID, UUID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return String( "Read Cached Pose" ); }
        virtual Color GetDebugColor() const override { return Colors::Red; }
        #endif

    private:

        CachedPoseReadTask() : Task( 0xFF ) {}

    private:

        UUID m_cachedPoseID;
    };
}