#pragma once

#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class CachedPoseWriteTask : public PoseTask
    {
        EE_REFLECT_TYPE( CachedPoseWriteTask );

    public:

        CachedPoseWriteTask( int8_t sourceTaskIdx, CachedPoseID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual int32_t GetNumDependencies() const override { return 1; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Write Cached Pose"; }
        virtual Color GetDebugColor() const override { return Colors::HotPink; }
        #endif

    private:

        CachedPoseWriteTask() : PoseTask() {}

    private:

        CachedPoseID    m_cachedPoseID;
        bool            m_isDeserializedTask = false;
    };

    //-------------------------------------------------------------------------

    class CachedPoseReadTask : public PoseTask
    {
        EE_REFLECT_TYPE( CachedPoseReadTask );

    public:

        CachedPoseReadTask( CachedPoseID cachedPoseID );
        virtual void Execute( TaskContext const& context ) override;
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Read Cached Pose"; }
        virtual Color GetDebugColor() const override { return Colors::HotPink; }
        #endif

    private:

        CachedPoseReadTask() : PoseTask() {}

    private:

        CachedPoseID    m_cachedPoseID;
        bool            m_isDeserializedTask = false;
    };
}