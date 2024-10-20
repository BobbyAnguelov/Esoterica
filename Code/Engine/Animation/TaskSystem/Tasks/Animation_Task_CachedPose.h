#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class CachedPoseWriteTask : public Task
    {
        EE_REFLECT_TYPE( CachedPoseWriteTask );

    public:

        CachedPoseWriteTask( int8_t sourceTaskIdx, UUID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Write Cached Pose"; }
        virtual Color GetDebugColor() const override { return Colors::Red; }
        #endif

    private:

        CachedPoseWriteTask() : Task() {}

    private:

        UUID m_cachedPoseID;
    };

    //-------------------------------------------------------------------------

    class CachedPoseReadTask : public Task
    {
        EE_REFLECT_TYPE( CachedPoseReadTask );

    public:

        CachedPoseReadTask( UUID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Read Cached Pose"; }
        virtual Color GetDebugColor() const override { return Colors::Red; }
        #endif

    private:

        CachedPoseReadTask() : Task() {}

    private:

        UUID m_cachedPoseID;
    };
}