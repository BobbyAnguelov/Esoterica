#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class DefaultPoseTask : public Task
    {
        EE_REFLECT_TYPE( DefaultPoseTask );

    public:

        DefaultPoseTask( TaskSourceID sourceID, Pose::Type type );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightGray; }
        #endif

    private:

        DefaultPoseTask() : Task( 0xFF ) {}

    private:

        Pose::Type m_type = Pose::Type::None;
    };
}