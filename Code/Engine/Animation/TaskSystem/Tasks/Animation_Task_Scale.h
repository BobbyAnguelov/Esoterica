#pragma once

#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"
#include "Engine/Animation/TaskSystem/Animation_BoneMaskTask.h"
#include "Engine/Animation/AnimationTarget.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ScaleTask : public PoseTask
    {
        EE_REFLECT_TYPE( ScaleTask );

    public:

        ScaleTask( int8_t sourceTaskIdx, BoneMaskTaskList const& boneMaskTaskList );
        virtual void Execute( TaskContext const& context ) override;

        virtual int32_t GetNumDependencies() const override { return 1; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Masked Scale"; }
        virtual Color GetDebugColor() const override { return Colors::GhostWhite; }
        #endif

    private:

        ScaleTask() : PoseTask() {}

    private:

        BoneMaskTaskList                        m_boneMaskTaskList;
    };
}