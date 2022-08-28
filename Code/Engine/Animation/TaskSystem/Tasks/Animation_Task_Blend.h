#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class BlendTask : public Task
    {

    public:

        BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, TBitFlags<PoseBlendOptions> const blendOptions = TBitFlags<PoseBlendOptions>(), BoneMask const* pBoneMask = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return String( String::CtorSprintf(), "Blend Task: %.2f", m_blendWeight ); }
        virtual Color GetDebugColor() const { return Colors::Yellow; }
        #endif

    private:

        BoneMask const*                         m_pBoneMask = nullptr;
        float                                   m_blendWeight = 0.0f;
        TBitFlags<PoseBlendOptions>             m_blendOptions;
    };
}