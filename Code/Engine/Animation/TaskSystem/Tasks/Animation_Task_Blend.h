#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/AnimationBlender.h"

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class BlendTask final : public Task
    {
        EE_REFLECT_TYPE( BlendTask );

    public:

        BlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        virtual Color GetDebugColor() const override { return Colors::Lime; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        BlendTask() : Task( 0xFF ) {}

    private:

        BoneMaskTaskList                        m_boneMaskTaskList;
        float                                   m_blendWeight = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        BoneMask                                m_debugBoneMask;
        #endif
    };

    //-------------------------------------------------------------------------

    class AdditiveBlendTask final : public Task
    {
        EE_REFLECT_TYPE( AdditiveBlendTask );

    public:

        AdditiveBlendTask( TaskSourceID sourceID, TaskIndex sourceTaskIdx, TaskIndex targetTaskIdx, float const blendWeight, BoneMaskTaskList const* pBoneMaskTaskList = nullptr );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override;
        virtual Color GetDebugColor() const override { return Colors::Lime; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        AdditiveBlendTask() : Task( 0xFF ) {}

    private:

        BoneMaskTaskList                        m_boneMaskTaskList;
        float                                   m_blendWeight = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        BoneMask                                m_debugBoneMask;
        #endif
    };

    //-------------------------------------------------------------------------

    class GlobalBlendTask final : public Task
    {
        EE_REFLECT_TYPE( GlobalBlendTask );

    public:

        GlobalBlendTask( TaskSourceID sourceID, TaskIndex baseTaskIdx, TaskIndex layerTaskIdx, float const layerWeight, BoneMaskTaskList const& boneMaskTaskList );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual String GetDebugText() const override { return "Global Blend"; }
        virtual Color GetDebugColor() const override { return Colors::Lime; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Pose const* pRecordedPose, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        GlobalBlendTask() : Task( 0xFF ) {}

    private:

        BoneMaskTaskList                        m_boneMaskTaskList;
        float                                   m_layerWeight = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        BoneMask                                m_debugBoneMask;
        #endif
    };
}