#pragma once
#include "BulkImporter.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class AnimationSkeletonBulkImporter final : public BulkImporter
    {
        EE_REFLECT_TYPE( AnimationSkeletonBulkImporter );

    public:

        using BulkImporter::BulkImporter;

    private:

        virtual char const* GetTitle() const override { return "Animation Skeleton"; }
        virtual TInlineVector<FileSystem::Extension, 8> GetSupportedFileExtensions() const override final { return { "fbx", "gltf", "glb" }; }
        virtual void ImportFile( FileImportContext const& ctx ) override;
        virtual ResourceTypeID GetImportedType() const override { return Animation::Skeleton::GetStaticResourceTypeID(); }

    private:

        // Optional value that specifies the name of the skeleton hierarchy to use, if it is unset, we use the first skeleton we find
        EE_REFLECT();
        String                                              m_skeletonRootBoneName;
    };

    //-------------------------------------------------------------------------

    class AnimationClipBulkImporter final : public BulkImporter
    {
        EE_REFLECT_TYPE( AnimationClipBulkImporter );

    public:

        using BulkImporter::BulkImporter;


    private:

        virtual char const* GetTitle() const override { return "Animation Clip"; }
        virtual TInlineVector<FileSystem::Extension, 8> GetSupportedFileExtensions() const override final { return { "fbx", "gltf", "glb" }; }
        virtual void ImportFile( FileImportContext const& ctx ) override;
        virtual ResourceTypeID GetImportedType() const override { return Animation::AnimationClip::GetStaticResourceTypeID(); }
        virtual bool HasRequiredSettingsForImport() const override;
        virtual InlineString GetInfoMessage() const override;

    private:

        EE_REFLECT();
        TResourcePtr<EE::Animation::Skeleton>                   m_skeleton = nullptr;

        EE_REFLECT();
        TVector<TResourcePtr<EE::Animation::Skeleton>>          m_secondarySkeletons;
    };
}