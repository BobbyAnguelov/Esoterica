#pragma once

#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/Animation/TaskSystem/Animation_BoneMaskTask.h"

//-------------------------------------------------------------------------

namespace EE { class DebugDrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BlendTaskBase : public PoseTask
    {
        EE_REFLECT_TYPE( BlendTaskBase );

    public:

        using PoseTask::PoseTask;

        virtual int32_t GetNumDependencies() const override { return 2; }
        virtual void Serialize( TaskSerializer& serializer ) const override final;
        virtual void Deserialize( TaskSerializer& serializer ) override final;

        #if EE_DEVELOPMENT_TOOLS
        virtual Color GetDebugColor() const override final { return Colors::Lime; }
        virtual float GetDebugProgressOrWeight() const override { return m_blendWeight; }
        virtual void DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override final;
        #endif

    protected:

        BlendTaskBase() : PoseTask() {}

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
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
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
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
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
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
        #endif

    protected:

        AdditiveBlendTask() : BlendTaskBase() {}
    };

    //-------------------------------------------------------------------------

    class ModelSpaceBlendTask final : public BlendTaskBase
    {
        EE_REFLECT_TYPE( ModelSpaceBlendTask );

    public:

        ModelSpaceBlendTask( int8_t baseTaskIdx, int8_t layerTaskIdx, float const blendWeight, BoneMaskTaskList const& boneMaskTaskList );
        virtual void Execute( TaskContext const& context ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Model Space Blend"; }
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
        #endif

    protected:

        ModelSpaceBlendTask() : BlendTaskBase() {}
    };
}