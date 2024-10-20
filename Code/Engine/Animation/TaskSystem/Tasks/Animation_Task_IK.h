#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationTarget.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IKRig;
}

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class IKRigTask : public Task
    {
        EE_REFLECT_TYPE( IKRigTask );

    public:

        IKRigTask( int8_t sourceTaskIdx, IKRig* pRig, TInlineVector<Target, 6> const& targets );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "IK Rig Solve"; }
        virtual Color GetDebugColor() const override { return Colors::OrangeRed; }
        #endif

    private:

        IKRigTask() : Task() {}

    private:

        IKRig*                          m_pRig = nullptr;
        TInlineVector<Target, 6>        m_effectorTargets;
    };
}