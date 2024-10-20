#include "ResourceCompiler_AnimationSkeleton.h"
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
        AddOutputType<Skeleton>();
    }

    Resource::CompilationResult SkeletonCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        SkeletonResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        FileSystem::Path skeletonFilePath;
        if ( !ConvertDataPathToFilePath( resourceDescriptor.m_skeletonPath, skeletonFilePath ) )
        {
            return Error( "Invalid skeleton data path: %s", resourceDescriptor.m_skeletonPath.c_str() );
        }

        Import::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        TUniquePtr<Import::ImportedSkeleton> pImportedSkeleton = Import::ReadSkeleton( readerCtx, skeletonFilePath, resourceDescriptor.m_skeletonRootBoneName, resourceDescriptor.m_highLODBones );
        if ( pImportedSkeleton == nullptr )
        {
            return Error( "Failed to read skeleton from source file" );
        }

        // Reflect raw data into runtime format
        //-------------------------------------------------------------------------

        Skeleton skeleton;
        int32_t const numBones = pImportedSkeleton->GetNumBones();
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            auto const& boneData = pImportedSkeleton->GetBoneData( boneIdx );
            skeleton.m_boneIDs.push_back( boneData.m_name );
            skeleton.m_parentIndices.push_back( boneData.m_parentBoneIdx );
            skeleton.m_parentSpaceReferencePose.push_back( Transform( boneData.m_parentSpaceTransform.GetRotation(), boneData.m_parentSpaceTransform.GetTranslation(), boneData.m_parentSpaceTransform.GetScale() ) );
            skeleton.m_numBonesToSampleAtLowLOD = pImportedSkeleton->GetNumBonesToSampleAtLowLOD();
        }

        // Generate runtime bone-masks
        //-------------------------------------------------------------------------

        TVector<StringID> missingBones;
        TVector<BoneMask::SerializedData> serializedBoneMasks;

        for ( BoneMaskDefinition const& definition : resourceDescriptor.m_boneMaskDefinitions )
        {
            if ( !definition.IsValid() )
            {
                Warning( "Invalid bone mask definition encountered (%s)", definition.m_ID.IsValid() ? definition.m_ID.c_str() : "No ID set!" );
                continue;
            }

            BoneMask::SerializedData& serializedMask = serializedBoneMasks.emplace_back();
            definition.GenerateSerializedBoneMask( &skeleton, serializedMask, &missingBones );

            for ( StringID boneID : missingBones )
            {
                Warning( "Couldn't find bone (%s) while serializing bone mask (%s)", boneID.c_str(), definition.m_ID.c_str() );
            }
        }

        // Serialize skeleton
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( Skeleton::s_version, Skeleton::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );
        archive << skeleton;
        archive << serializedBoneMasks;

        // Write preview data
        if ( ctx.IsCompilingForDevelopmentBuild() )
        {
            archive << resourceDescriptor.m_previewMesh.GetResourceID();
            archive << resourceDescriptor.m_previewAttachmentSocketID;
        }

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }
}