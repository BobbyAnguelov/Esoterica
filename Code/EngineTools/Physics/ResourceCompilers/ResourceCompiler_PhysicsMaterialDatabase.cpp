#include "ResourceCompiler_PhysicsMaterialDatabase.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsMaterialDatabase.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    PhysicsMaterialDatabaseCompiler::PhysicsMaterialDatabaseCompiler()
        : Resource::Compiler( "PhysicsMaterialCompiler" )
    {
        AddOutputType<MaterialDatabase>();
    }

    Resource::CompilationResult PhysicsMaterialDatabaseCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        PhysicsMaterialDatabaseResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Read material libraries
        //-------------------------------------------------------------------------

        MaterialDatabase db;

        for ( DataPath const& libraryPath : resourceDescriptor.m_materialLibraries )
        {
            FileSystem::Path libraryFilePath;
            if ( !ConvertDataPathToFilePath( libraryPath, libraryFilePath ) )
            {
                return Error( "Failed to convert data path to filepath: %s", libraryPath.c_str() );
            }

            //-------------------------------------------------------------------------

            PhysicsMaterialLibrary materialLibrary;
            if ( !IDataFile::TryReadFromFile( *m_pTypeRegistry, libraryFilePath, materialLibrary ) )
            {
                return Error( "ResourceCompiler", "Failed to read physics material library file: %s", libraryFilePath.c_str() );
            }

            for ( auto const& material : materialLibrary.m_settings )
            {
                // Validity check
                if ( !material.IsValid() )
                {
                    return Error( "Invalid physics material encountered in library: %s", libraryFilePath.c_str() );
                }

                // Duplicate check
                auto DuplicateCheck = [] ( MaterialSettings const& materialSettings, StringID const& ID )
                {
                    return materialSettings.m_ID == ID;
                };

                if ( eastl::find( db.m_materials.begin(), db.m_materials.end(), material.m_ID, DuplicateCheck ) != db.m_materials.end() )
                {
                    return Error( "ResourceCompiler", "Duplicate physics material ID '%s' detected", material.m_ID.c_str() );
                }

                // Add valid material
                db.m_materials.emplace_back( material );
            }
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( MaterialDatabase::s_version, MaterialDatabase::GetStaticResourceTypeID(), ctx.m_sourceResourceHash, ctx.m_advancedUpToDateHash );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << db;

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