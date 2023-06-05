#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class BlendTask : public Task
    {
        EE_REFLECT_TYPE( BlendTask );

    public:

        BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, PoseBlendMode blendMode = PoseBlendMode::Interpolative, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        virtual Color GetDebugColor() const override { return Colors::Yellow; }
        #endif

    private:

        BlendTask() : Task( 0xFF ) {}

    private:

        BoneMaskTaskList                        m_boneMaskTaskList;
        float                                   m_blendWeight = 0.0f;
        PoseBlendMode                           m_blendMode;
    };
}