#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "Base/Memory/Pointers.h"

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
        EE_REFLECT_TYPE( AnimationClipCompiler );
        static const int32_t s_version = 43;

    public:

        AnimationClipCompiler();

    private:

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;

        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;

        Resource::CompilationResult ReadRawAnimation( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, TUniquePtr<RawAssets::RawAnimation>& pOutAnimation ) const;

        Resource::CompilationResult MakeAdditive( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, RawAssets::RawAnimation& rawAnimData ) const;

        Resource::CompilationResult RegenerateRootMotion( AnimationClipResourceDescriptor const& resourceDescriptor, RawAssets::RawAnimation* pRawAnimation ) const;

        Resource::CompilationResult ReadEventsData( Resource::CompileContext const& ctx, rapidjson::Document const& document, RawAssets::RawAnimation const& rawAnimData, AnimationClipEventData& outEventData ) const;

        Resource::CompilationResult TransferAndCompressAnimationData( RawAssets::RawAnimation const& rawAnimData, AnimationClip& animClip, IntRange const& limitRange ) const;
    };
}