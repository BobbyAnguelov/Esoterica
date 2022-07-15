#include "ResourceCompiler_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/RawAssets/RawSkeleton.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "System/Animation/AnimationSkeleton.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    SkeletonCompiler::SkeletonCompiler() 
        : Resource::Compiler( "SkeletonCompiler", s_version )
    {
        m_outputTypes.push_back( Skeleton::GetStaticResourceTypeID() );
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
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_skeletonPath, skeletonFilePath ) )
        {
            return Error( "Invalid skeleton data path: %s", resourceDescriptor.m_skeletonPath.c_str() );
        }

        RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        TUniquePtr<RawAssets::RawSkeleton> pRawSkeleton = RawAssets::ReadSkeleton( readerCtx, skeletonFilePath, resourceDescriptor.m_skeletonRootBoneName );
        if ( pRawSkeleton == nullptr )
        {
            return Error( "Failed to read skeleton from source file" );
        }

        // Reflect raw data into runtime format
        //-------------------------------------------------------------------------

        Skeleton skeleton;

        int32_t const numBones = pRawSkeleton->GetNumBones();
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            auto const& boneData = pRawSkeleton->GetBoneData( boneIdx );
            skeleton.m_boneIDs.push_back( boneData.m_name );
            skeleton.m_parentIndices.push_back( boneData.m_parentBoneIdx );
            skeleton.m_localReferencePose.push_back( Transform( boneData.m_localTransform.GetRotation(), boneData.m_localTransform.GetTranslation(), boneData.m_localTransform.GetScale() ) );
        }

        // Serialize skeleton
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( s_version, Skeleton::GetStaticResourceTypeID() ) << skeleton;

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