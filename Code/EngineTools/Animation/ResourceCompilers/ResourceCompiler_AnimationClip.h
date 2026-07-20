#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "Base/Memory/UniquePtr.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class Animation;
    struct AnimationClip;
}

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

    public:

        AnimationClipCompiler();

    private:

        virtual void GenerateCustomDependencyHashes( Resource::CompileContext const& ctx, Resource::CompileDependencyResourceInfo* pResourceToCompile ) const final;

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;

        Resource::CompilationResult ReadImportedAnimation( Resource::CompileContext const& ctx, DataPath const& skeletonPath, TVector<DataPath> const& secondarySkeletonPaths, DataPath const& animationPath, TUniquePtr<Import::Animation>& outAnimation, String const& animationName = String() ) const;

        Resource::CompilationResult RegenerateRootMotion( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::Animation* pImportedAnimation ) const;

        Resource::CompilationResult ProcessEventsData( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::Animation const& rawAnimData, AnimationClipEventData& outEventData ) const;

        Resource::CompilationResult TransferAndCompressAnimationData( Resource::CompileContext const& ctx, Import::Animation const& importedAnimation, Import::AnimationClip const& importedClip, AnimationClip& outAnimClip, IntRange const& limitRange, TVector<StringID> const& bonesToSampleInModelSpace, bool isPrimaryClip ) const;
    };
}