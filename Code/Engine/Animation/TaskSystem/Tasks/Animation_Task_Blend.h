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
        virtual float GetDebugProgressOrWeight() const override { return m_blendWeight; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override final;
        #endif

    protected:

        BlendTaskBase() : Task() {}

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

        BlendTask( int8_t sourceTaskIdx, int8_t targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Blend"; }
        virtual InlineString GetDebugTextInfo() const override;
        #endif

    protected:

        BlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class OverlayBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( OverlayBlendTask );

    public:

        OverlayBlendTask( int8_t sourceTaskIdx, int8_t targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Overlay Blend"; }
        virtual InlineString GetDebugTextInfo() const override;
        #endif

    protected:

        OverlayBlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class AdditiveBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( AdditiveBlendTask );

    public:

        AdditiveBlendTask( int8_t sourceTaskIdx, int8_t targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Additive Blend"; }
        virtual InlineString GetDebugTextInfo() const override;
        #endif

    protected:

        AdditiveBlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class GlobalBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( GlobalBlendTask );

    public:

        GlobalBlendTask( int8_t baseTaskIdx, int8_t layerTaskIdx, float const blendWeight, BoneMaskTaskList const& boneMaskTaskList );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Global Blend"; }
        #endif

    protected:

        GlobalBlendTask() : BlendTaskBase() {}
    };
}