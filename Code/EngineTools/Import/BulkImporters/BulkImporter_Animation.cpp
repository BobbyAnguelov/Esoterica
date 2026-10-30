#include "BulkImporter_Animation.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    void AnimationSkeletonBulkImporter::ImportFile( FileImportContext const& ctx )
    {
        Animation::SkeletonResourceDescriptor descriptor;
        descriptor.m_skeletonPath = ctx.m_fileToImport.m_path;
        descriptor.m_skeletonRootBoneName = m_skeletonRootBoneName;

        Log log;
        if ( !Resource::ResourceDescriptor::TryWriteToFile( ctx.m_typeRegistry, log, ctx.m_targetFilePath, &descriptor ) )
        {
            ctx.AppendLog( log );
        }
    }

    //-------------------------------------------------------------------------

    void AnimationClipBulkImporter::ImportFile( FileImportContext const& ctx )
    {
        Animation::AnimationClipResourceDescriptor descriptor;
        descriptor.m_animationPath = ctx.m_fileToImport.m_path;
        descriptor.m_skeleton = m_skeleton;
        descriptor.m_secondarySkeletons = m_secondarySkeletons;

        Log log;
        if ( !Resource::ResourceDescriptor::TryWriteToFile( ctx.m_typeRegistry, log, ctx.m_targetFilePath, &descriptor ) )
        {
            ctx.AppendLog( log );
        }
    }

    bool AnimationClipBulkImporter::HasRequiredSettingsForImport() const
    {
        return m_skeleton.IsSetAndIsValidResourceTypeID();
    }

    InlineString AnimationClipBulkImporter::GetInfoMessage() const
    {
        return ( m_skeleton.IsSetAndIsValidResourceTypeID() ) ? InlineString() : "Skeleton field needs to be set!";
    }
}