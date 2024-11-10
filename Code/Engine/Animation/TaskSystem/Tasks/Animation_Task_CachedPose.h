#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class CachedPoseWriteTask : public Task
    {
        EE_REFLECT_TYPE( CachedPoseWriteTask );

    public:

        CachedPoseWriteTask( int8_t sourceTaskIdx, CachedPoseID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Write Cached Pose"; }
        virtual Color GetDebugColor() const override { return Colors::HotPink; }
        #endif

    private:

        CachedPoseWriteTask() : Task() {}

    private:

        CachedPoseID    m_cachedPoseID;
        bool            m_isDeserializedTask = false;
    };

    //-------------------------------------------------------------------------

    class CachedPoseReadTask : public Task
    {
        EE_REFLECT_TYPE( CachedPoseReadTask );

    public:

        CachedPoseReadTask( CachedPoseID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Read Cached Pose"; }
        virtual Color GetDebugColor() const override { return Colors::HotPink; }
        #endif

    private:

        CachedPoseReadTask() : Task() {}

    private:

        CachedPoseID    m_cachedPoseID;
        bool            m_isDeserializedTask = false;
    };
}