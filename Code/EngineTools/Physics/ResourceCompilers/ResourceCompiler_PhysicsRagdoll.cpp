#include "ResourceCompiler_PhysicsRagdoll.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Import/Importer.h"
#include "EngineTools/Import/ImportedSkeleton.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Definition.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    RagdollCompiler::RagdollCompiler()
        : Resource::Compiler( "PhysicsRagdollCompiler" )
    {
        RegisterOutput<RagdollDefinition>();
    }

    Resource::CompilationResult RagdollCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<RagdollResourceDescriptor>();
        if ( !pResourceDescriptor->m_skeleton.IsSet() || pResourceDescriptor->m_bodies.empty() )
        {
            return ctx.LogError( "Invalid ragdoll definition!" );
        }

        if ( !pResourceDescriptor->ValidateSettings() )
        {
            return ctx.LogError( "Invalid ragdoll body settings detected, please open this definition in the editor and fix any errors!" );
        }

        // Read skeleton
        //-------------------------------------------------------------------------

        auto pSkeletonResourceDescriptor = ctx.GetDescriptor<Animation::SkeletonResourceDescriptor>( pResourceDescriptor->m_skeleton.GetDataPath() );

        FileSystem::Path const skeletonFilePath = pSkeletonResourceDescriptor->m_skeletonPath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );
        Import::Source const skeletonFileSource( skeletonFilePath, ctx.GetRawData( pSkeletonResourceDescriptor->m_skeletonPath ) );

        Import::ReaderContext readerCtx = { [&ctx]( char const* pString ) { ctx.LogWarning( pString ); }, [&ctx] ( char const* pString ) { ctx.LogError( pString ); } };
        TUniquePtr<Import::Skeleton> skeleton = Import::Importer::ReadSkeleton( readerCtx, skeletonFileSource, pSkeletonResourceDescriptor->m_skeletonRootBoneName, pSkeletonResourceDescriptor->m_highLODBones );
        if ( skeleton == nullptr || !skeleton->IsValid() )
        {
            return ctx.LogError( "Failed to read skeleton file: %s", skeletonFilePath.ToString().c_str() );
        }

        // Validate body bone IDs
        //-------------------------------------------------------------------------

        for ( auto const& body : pResourceDescriptor->m_bodies )
        {
            if ( skeleton->GetBoneIndex( body.m_boneID ) == InvalidIndex )
            {
                return ctx.LogError( "Body refers to invalid/missing bone: %s", body.m_boneID.c_str() );
            }
        }

        // Transfer Data
        //-------------------------------------------------------------------------

        RagdollDefinition definition;
        definition.m_skeleton = pResourceDescriptor->m_skeleton;
        definition.m_bodies = pResourceDescriptor->m_bodies;
        definition.m_disableCollisionRules = pResourceDescriptor->m_disableCollisionRules;

        Log log;
        if ( !definition.PerformValidation( &log ) )
        {
            for ( auto const& entry : log.GetLogEntries() )
            {
                switch ( entry.m_severity )
                {
                    case Severity::FatalError:
                    case Severity::Error:
                    {
                        ctx.LogError( entry.m_message.c_str() );
                    }
                    break;

                    case Severity::Warning:
                    {
                        ctx.LogWarning( entry.m_message.c_str() );
                    }
                    break;

                    case Severity::Info:
                    ctx.LogMessage( entry.m_message.c_str() );
                    break;
                }
            }

            return Resource::CompilationResult::Failure;
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( RagdollDefinition::s_version, RagdollDefinition::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        hdr.AddInstallDependency( definition.m_skeleton.GetResourceID() );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << definition;

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