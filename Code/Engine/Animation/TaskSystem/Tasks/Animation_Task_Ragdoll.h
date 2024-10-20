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
        EE_REFLECT_TYPE( RagdollSetPoseTask );

    public:

        enum InitOption
        {
            DoNothing,
            InitializeBodies
        };

    public:

        RagdollSetPoseTask( Physics::Ragdoll* pRagdoll, int8_t sourceTaskIdx, InitOption initOption = InitOption::DoNothing );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Set Ragdoll Pose"; }
        virtual Color GetDebugColor() const override { return Colors::Orange; }
        #endif

    private:

        RagdollSetPoseTask() : Task() {}

    private:

        Physics::Ragdoll*                       m_pRagdoll = nullptr;
        InitOption                              m_initOption = InitOption::DoNothing;
    };

    //-------------------------------------------------------------------------

    class RagdollGetPoseTask : public Task
    {
        EE_REFLECT_TYPE( RagdollGetPoseTask );

    public:

        RagdollGetPoseTask( Physics::Ragdoll* pRagdoll, int8_t sourceTaskIdx, float const physicsBlendWeight = 1.0f );
        RagdollGetPoseTask( Physics::Ragdoll* pRagdoll );
        virtual void Execute( TaskContext const& context ) override;
        virtual bool AllowsSerialization() const override { return false; }

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Get Ragdoll Pose"; }
        virtual float GetDebugProgressOrWeight() const override { return m_physicsBlendWeight; }
        virtual Color GetDebugColor() const override { return Colors::Yellow; }
        #endif

    private:

        RagdollGetPoseTask() : Task() {}

    private:

        Physics::Ragdoll*                       m_pRagdoll = nullptr;
        float                                   m_physicsBlendWeight = 1.0f;
    };
}