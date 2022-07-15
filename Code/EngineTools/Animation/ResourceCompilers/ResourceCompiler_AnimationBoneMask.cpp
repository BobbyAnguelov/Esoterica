#include "ResourceCompiler_AnimationBoneMask.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationBoneMask.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/RawAssets/RawSkeleton.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "System/Resource/ResourcePtr.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    BoneMaskCompiler::BoneMaskCompiler()
        : Resource::Compiler( "Animation Bone Mask Compiler", s_version )
    {
        m_outputTypes.push_back( BoneMaskDefinition::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult BoneMaskCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        BoneMaskResourceDescriptor resourceDescriptor;

        Serialization::TypeArchiveReader typeReader( *m_pTypeRegistry );
        if ( !typeReader.ReadFromFile( ctx.m_inputFilePath ) )
        {
            return Error( "Failed to read resource descriptor file: %s", ctx.m_inputFilePath.c_str() );
        }

        if ( !typeReader.ReadType( &resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Read Skeleton Data
        //-------------------------------------------------------------------------

        // Convert the skeleton resource path to a physical file path
        if ( !resourceDescriptor.m_pSkeleton.GetResourceID().IsValid() )
        {
            return Error( "Invalid skeleton resource ID" );
        }

        ResourcePath const& skeletonPath = resourceDescriptor.m_pSkeleton.GetResourcePath();
        FileSystem::Path skeletonDescriptorFilePath;
        if ( !ConvertResourcePathToFilePath( skeletonPath, skeletonDescriptorFilePath ) )
        {
            return Error( "Invalid skeleton data path: %s", skeletonPath.c_str() );
        }

        if ( !FileSystem::Exists( skeletonDescriptorFilePath ) )
        {
            return Error( "Invalid skeleton descriptor file path: %s", skeletonDescriptorFilePath.ToString().c_str() );
        }

        SkeletonResourceDescriptor skeletonResourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, skeletonDescriptorFilePath, skeletonResourceDescriptor ) )
        {
            return Error( "Failed to read skeleton resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        FileSystem::Path skeletonFilePath;
        if ( !ConvertResourcePathToFilePath( skeletonResourceDescriptor.m_skeletonPath, skeletonFilePath ) )
        {
            return Error( "Invalid skeleton FBX data path: %s", skeletonResourceDescriptor.m_skeletonPath.GetString().c_str() );
        }

        RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        auto pRawSkeleton = RawAssets::ReadSkeleton( readerCtx, skeletonFilePath, skeletonResourceDescriptor.m_skeletonRootBoneName );
        if ( pRawSkeleton == nullptr || !pRawSkeleton->IsValid() )
        {
            return Error( "Failed to read skeleton file: %s", skeletonFilePath.ToString().c_str() );
        }

        // Read bone mask definition
        //-------------------------------------------------------------------------

        bool hasWarnings = false;

        int32_t const numSkeletonBones = pRawSkeleton->GetNumBones();
        int32_t const numDefinitionBones = (int32_t) resourceDescriptor.m_boneWeights.size();

        if ( numDefinitionBones != numSkeletonBones )
        {
            Warning( "Incorrect number of bone weights - bone mask may be incorrect!" );
            hasWarnings = true;
        }

        BoneMaskDefinition boneMaskDef;
        int32_t const numWeights = Math::Min( numSkeletonBones, numDefinitionBones );
        for ( auto i = 0; i < numWeights; i++ )
        {
            StringID const boneID = pRawSkeleton->GetBoneName( i );

            if ( resourceDescriptor.m_boneWeights[i] == -1.0f || resourceDescriptor.m_boneWeights[i] == 0.0f )
            {
                continue;
            }

            if ( resourceDescriptor.m_boneWeights[i] < 0.0f || resourceDescriptor.m_boneWeights[i] > 1.0f )
            {
                return Error( "Invalid weight detected for bone: %s", boneID.c_str() );
            }

            boneMaskDef.m_weights.emplace_back( boneID, resourceDescriptor.m_boneWeights[i] );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, BoneMaskDefinition::GetStaticResourceTypeID() );
        hdr.AddInstallDependency( resourceDescriptor.m_pSkeleton.GetResourceID() );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << boneMaskDef;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            if ( hasWarnings )
            {
                return CompilationSucceededWithWarnings( ctx );
            }
            else
            {
                return CompilationSucceeded( ctx );
            }
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }
}