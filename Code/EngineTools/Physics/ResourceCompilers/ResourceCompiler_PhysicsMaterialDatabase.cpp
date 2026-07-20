#include "ResourceCompiler_PhysicsMaterialDatabase.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsMaterialDatabase.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    PhysicsMaterialDatabaseCompiler::PhysicsMaterialDatabaseCompiler()
        : Resource::Compiler( "PhysicsMaterialCompiler" )
    {
        RegisterOutput<MaterialDatabase>();
    }

    Resource::CompilationResult PhysicsMaterialDatabaseCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<PhysicsMaterialDatabaseResourceDescriptor>();

        // Read material libraries
        //-------------------------------------------------------------------------

        MaterialDatabase db;

        for ( DataPath const& libraryPath : pResourceDescriptor->m_materialLibraries )
        {
            auto pMaterialLibrary = ctx.GetDataFile<PhysicsMaterialLibrary>( libraryPath );
            for ( auto const& material : pMaterialLibrary->m_materials )
            {
                // Validity check
                if ( !material.IsValid() )
                {
                    return ctx.LogError( "Invalid physics material encountered in library: %s (%s)", libraryPath.c_str() ,libraryPath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath ).c_str() );
                }

                // Duplicate check
                auto DuplicateCheck = [] ( Material const& materialSettings, StringID const& ID )
                {
                    return materialSettings.m_ID == ID;
                };

                if ( eastl::find( db.m_materials.begin(), db.m_materials.end(), material.m_ID, DuplicateCheck ) != db.m_materials.end() )
                {
                    return ctx.LogError( "ResourceCompiler", "Duplicate physics material ID '%s' detected", material.m_ID.c_str() );
                }

                // Add valid material
                db.m_materials.emplace_back( material );
            }
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( MaterialDatabase::s_version, MaterialDatabase::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << db;

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