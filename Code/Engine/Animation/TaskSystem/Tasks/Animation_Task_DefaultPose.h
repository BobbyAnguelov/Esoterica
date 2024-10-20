#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class ReferencePoseTask : public Task
    {
        EE_REFLECT_TYPE( ReferencePoseTask );

    public:

        ReferencePoseTask() : Task() {}
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override {}
        virtual void Deserialize( TaskSerializer& serializer ) override {}

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightYellow; }
        #endif
    };

    //-------------------------------------------------------------------------

    class ZeroPoseTask : public Task
    {
        EE_REFLECT_TYPE( ZeroPoseTask );

    public:

        ZeroPoseTask() : Task() {}
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override {}
        virtual void Deserialize( TaskSerializer& serializer ) override {}

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightGray; }
        #endif
    };
}