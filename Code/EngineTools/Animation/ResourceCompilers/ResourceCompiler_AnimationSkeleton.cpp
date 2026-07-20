#include "ResourceCompiler_AnimationSkeleton.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Import/ImportedSkeleton.h"
#include "EngineTools/Import/Importer.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    SkeletonCompiler::SkeletonCompiler() 
        : Resource::Compiler( "SkeletonCompiler" )
    {
        RegisterOutput<Skeleton>();
    }

    Resource::CompilationResult SkeletonCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        SkeletonResourceDescriptor resourceDescriptor = *ctx.GetDescriptor<SkeletonResourceDescriptor>();

        // Basic validation
        //-------------------------------------------------------------------------

        if ( !resourceDescriptor.ValidateData( &ctx.m_log ) )
        {
            return Resource::CompilationResult::Failure;
        }

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        FileSystem::Path const skeletonFilePath = resourceDescriptor.m_skeletonPath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );

        Import::ReaderContext readerCtx = { [&ctx]( char const* pString ) { ctx.LogWarning( pString ); }, [&ctx] ( char const* pString ) { ctx.LogError( pString ); } };
        Import::Source const fileSource( skeletonFilePath, ctx.GetRawData( resourceDescriptor.m_skeletonPath ) );
        TUniquePtr<Import::Skeleton> pImportedSkeleton = Import::Importer::ReadSkeleton( readerCtx, fileSource, resourceDescriptor.m_skeletonRootBoneName, resourceDescriptor.m_highLODBones );
        if ( pImportedSkeleton == nullptr )
        {
            return ctx.LogError( "Failed to read skeleton from source file" );
        }

        // Reflect raw data into runtime format
        //-------------------------------------------------------------------------

        Skeleton skeleton;
        int32_t const numBones = (int32_t) pImportedSkeleton->GetNumBones();
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            auto const& boneData = pImportedSkeleton->GetBoneData( boneIdx );
            skeleton.m_boneIDs.push_back( boneData.m_name );
            skeleton.m_parentIndices.push_back( boneData.m_parentBoneIdx );
            skeleton.m_parentSpaceReferencePose.push_back( Transform( boneData.m_parentSpaceTransform.GetRotation(), boneData.m_parentSpaceTransform.GetTranslation(), boneData.m_parentSpaceTransform.GetScale() ) );
            skeleton.m_numBonesToSampleAtLowLOD = (int32_t) pImportedSkeleton->GetNumBonesToSampleAtLowLOD();
        }

        // Setup secondary skeletons
        //-------------------------------------------------------------------------

        for ( SkeletonResourceDescriptor::SecondarySkeleton const& secondarySkeletonDesc : resourceDescriptor.m_secondarySkeletons )
        {
            if ( !secondarySkeletonDesc.m_skeleton.IsSet() )
            {
                continue;
            }

            Skeleton::SecondarySkeleton& secondarySkeleton = skeleton.m_secondarySkeletons.emplace_back();
            secondarySkeleton.m_attachmentBoneID = secondarySkeletonDesc.m_attachmentBoneID;
            secondarySkeleton.m_skeleton = secondarySkeletonDesc.m_skeleton;
        }

        // Generate bone index LUT
        //-------------------------------------------------------------------------

        skeleton.m_boneIndexLUT.clear();
        for ( int32_t i = 0; i < skeleton.GetNumBones(); i++ )
        {
            skeleton.m_boneIndexLUT.try_emplace( skeleton.m_boneIDs[i], i );
        }

        // Generate runtime bone-masks
        //-------------------------------------------------------------------------

        TVector<BoneMaskSetDefinition> validBoneMaskSets;

        for ( BoneMaskSetDefinition const& boneMaskSet : resourceDescriptor.m_boneMaskSetDefinitions )
        {
            if ( !boneMaskSet.IsValid() )
            {
                ctx.LogWarning( "Ignoring invalid bone mask set encountered (%s)", boneMaskSet.m_ID.IsValid() ? boneMaskSet.m_ID.c_str() : "No ID set!" );
                continue;
            }

            validBoneMaskSets.emplace_back( boneMaskSet );
        }

        // Transfer float channel sets
        //-------------------------------------------------------------------------

        for ( FloatChannelSet const &setDef : resourceDescriptor.m_floatChannelSets )
        {
            if ( !setDef.IsValid() )
            {
                ctx.LogWarning( "Ignoring invalid float channel set encountered (%s)", setDef.m_ID.IsValid() ? setDef.m_ID.c_str() : "No ID set!" );
                continue;
            }

            //-------------------------------------------------------------------------

            FloatChannelSet& reflectedFloatChannelSet = skeleton.m_floatChannelSets.emplace_back();
            reflectedFloatChannelSet.m_ID = setDef.m_ID;

            for ( size_t i = 0; i < setDef.m_channelIDs.size(); i++ )
            {
                if ( !setDef.m_channelIDs[i].IsValid() )
                {
                    ctx.LogWarning( "Ignoring invalid float channel ID in set (%s)", setDef.m_ID.c_str() );
                    continue;
                }

                if ( VectorFindIndex( reflectedFloatChannelSet.m_channelIDs, setDef.m_channelIDs[i] ) != InvalidIndex )
                {
                    ctx.LogWarning( "Ignoring duplicate float channel ID (%s) in set (%s)", setDef.m_channelIDs[i].c_str(), setDef.m_ID.c_str() );
                    continue;
                }

                if ( skeleton.GetBoneIndex( setDef.m_channelIDs[i] ) != InvalidIndex )
                {
                    ctx.LogError( "Float channel ID (%s) in set (%s) has the same name as a bone in the skeleton, this is not allowed!", setDef.m_channelIDs[i].c_str(), setDef.m_ID.c_str() );
                    return Resource::CompilationResult::Failure;
                }

                reflectedFloatChannelSet.m_channelIDs.emplace_back( setDef.m_channelIDs[i] );
            }
        }

        // Serialize skeleton
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( Skeleton::s_version, Skeleton::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );

        for ( auto const& secondarySkeletonDesc : resourceDescriptor.m_secondarySkeletons )
        {
            hdr.AddInstallDependency( secondarySkeletonDesc.m_skeleton.GetResourceID() );
        }

        Serialization::BinaryOutputArchive archive;
        archive << hdr;
        archive << skeleton;
        archive << validBoneMaskSets;

        // Write preview data
        if ( ctx.IsCompilingForDevelopmentBuild() )
        {
            archive << resourceDescriptor.m_previewMesh.GetResourceID();
        }

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            return Resource::CompilationResult::Success;
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }
}