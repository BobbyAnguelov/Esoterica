#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class BlendTaskBase : public Task
    {
        EE_REFLECT_TYPE( BlendTaskBase );

    public:

        using Task::Task;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override final;
        virtual void Deserialize( TaskSerializer& serializer ) override final;

        #if EE_DEVELOPMENT_TOOLS
        virtual Color GetDebugColor() const override final { return Colors::Lime; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override final;
        #endif

    protected:

        BlendTaskBase() : Task( 0xFF ) {}

    protected:

        BoneMaskTaskList                        m_boneMaskTaskList;
        float                                   m_blendWeight = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        BoneMask                                m_debugBoneMask;
        #endif
    };

    //-------------------------------------------------------------------------

    class BlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( BlendTask );

    public:

        BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        #endif

    protected:

        BlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class OverlayBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( OverlayBlendTask );

    public:

        OverlayBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        #endif

    protected:

        OverlayBlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class AdditiveBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( AdditiveBlendTask );

    public:

        AdditiveBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        #endif

    protected:

        AdditiveBlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class GlobalBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( GlobalBlendTask );

    public:

        GlobalBlendTask( TaskSourceID sourceID, TaskIndex baseTaskIdx, TaskIndex layerTaskIdx, float const blendWeight, BoneMaskTaskList const& boneMaskTaskList );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Global Blend"; }
        #endif

    protected:

        GlobalBlendTask() : BlendTaskBase() {}
    };
}