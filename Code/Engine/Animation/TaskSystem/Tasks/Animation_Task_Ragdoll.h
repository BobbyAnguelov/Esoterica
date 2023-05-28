#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class Ragdoll;
}

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class RagdollSetPoseTask : public Task
    {
        EE_ANIMATION_TASK( RagdollSetPoseTask );

    public:

        enum InitOption
        {
            DoNothing,
            InitializeBodies
        };

    public:

        RagdollSetPoseTask( Physics::Ragdoll* pRagdoll, TaskSourceID sourceID, TaskIndex sourceTaskIdx, InitOption initOption = InitOption::DoNothing );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Set Ragdoll Pose"; }
        virtual Color GetDebugColor() const { return Colors::Orange; }
        #endif

    private:

        Physics::Ragdoll*                       m_pRagdoll = nullptr;
        InitOption                              m_initOption = InitOption::DoNothing;
    };

    //-------------------------------------------------------------------------

    class RagdollGetPoseTask : public Task
    {

    public:

        RagdollGetPoseTask( Physics::Ragdoll* pRagdoll, TaskSourceID sourceID, TaskIndex sourceTaskIdx, float const physicsBlendWeight = 1.0f );
        RagdollGetPoseTask( Physics::Ragdoll* pRagdoll, TaskSourceID sourceID );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        virtual Color GetDebugColor() const { return Colors::Yellow; }
        #endif

    private:

        Physics::Ragdoll*                       m_pRagdoll = nullptr;
        float                                   m_physicsBlendWeight = 1.0f;
    };
}