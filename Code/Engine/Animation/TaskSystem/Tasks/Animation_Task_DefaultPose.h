#pragma once

#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ReferencePoseTask : public PoseTask
    {
        EE_REFLECT_TYPE( ReferencePoseTask );

    public:

        ReferencePoseTask() : PoseTask() {}
        virtual void Execute( TaskContext const& context ) override;
        virtual void Serialize( TaskSerializer& serializer ) const override {}
        virtual void Deserialize( TaskSerializer& serializer ) override {}

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightYellow; }
        #endif
    };

    //-------------------------------------------------------------------------

    class ZeroPoseTask : public PoseTask
    {
        EE_REFLECT_TYPE( ZeroPoseTask );

    public:

        ZeroPoseTask() : PoseTask() {}
        virtual void Execute( TaskContext const& context ) override;
        virtual void Serialize( TaskSerializer& serializer ) const override {}
        virtual void Deserialize( TaskSerializer& serializer ) override {}

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Default Pose Task"; }
        virtual Color GetDebugColor() const override { return Colors::LightGray; }
        #endif
    };
}