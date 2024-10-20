#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "Base/Memory/UniquePtr.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Import { class ImportedAnimation; }

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

        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const final;

        virtual bool GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const override;

        Resource::CompilationResult ReadImportedAnimation( DataPath const& skeletonPath, DataPath const& animationPath, TUniquePtr<Import::ImportedAnimation>& outAnimation, String const& animationName = String() ) const;

        Resource::CompilationResult MakeAdditive( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::ImportedAnimation& rawAnimData, bool isSecondaryAnimation ) const;

        Resource::CompilationResult RegenerateRootMotion( AnimationClipResourceDescriptor const& resourceDescriptor, Import::ImportedAnimation* pImportedAnimation ) const;

        Resource::CompilationResult ProcessEventsData( Resource::CompileContext const& ctx, AnimationClipResourceDescriptor const& resourceDescriptor, Import::ImportedAnimation const& rawAnimData, AnimationClipEventData& outEventData ) const;

        Resource::CompilationResult TransferAndCompressAnimationData( Import::ImportedAnimation const& rawAnimData, AnimationClip& animClip, IntRange const& limitRange, bool isSecondaryAnimation ) const;
    };
}