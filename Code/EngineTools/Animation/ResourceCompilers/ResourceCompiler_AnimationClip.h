#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets { class RawAnimation; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClip;
    struct AnimationClipEventData;
    struct AnimationClipResourceDescriptor;

    //-------------------------------------------------------------------------

    class AnimationClipCompiler : public Resource::Compiler
    {
        EE_REGISTER_TYPE( AnimationClipCompiler );
        static const int32_t s_version = 35;

    public:

        AnimationClipCompiler();

    private:

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;

        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;

        void TransferAndCompressAnimationData( RawAssets::RawAnimation const& rawAnimData, AnimationClip& animClip, IntRange const& limitRange ) const;

        bool ReadEventsData( Resource::CompileContext const& ctx, rapidjson::Document const& document, RawAssets::RawAnimation const& rawAnimData, AnimationClipEventData& outEventData ) const;

        bool RegenerateRootMotion( AnimationClipResourceDescriptor const& resourceDescriptor, RawAssets::RawAnimation* pRawAnimation ) const;
    };
}