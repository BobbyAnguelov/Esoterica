#include "ResourceCompiler_PhysicsMaterialDatabase.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsMaterialDatabase.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    PhysicsMaterialDatabaseCompiler::PhysicsMaterialDatabaseCompiler()
        : Resource::Compiler( "PhysicsMaterialCompiler", s_version )
    {
        m_outputTypes.push_back( MaterialDatabase::GetStaticResourceTypeID() );
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

        for ( ResourcePath const& libraryPath : resourceDescriptor.m_materialLibraries )
        {
            FileSystem::Path libraryFilePath;
            if ( !ConvertResourcePathToFilePath( libraryPath, libraryFilePath ) )
            {
                return Error( "Failed to convert data path to filepath: %s", libraryPath.c_str() );
            }

            //-------------------------------------------------------------------------

            Serialization::TypeArchiveReader typeReader( *m_pTypeRegistry );
            if ( typeReader.ReadFromFile( libraryFilePath ) )
            {
                int32_t const numSerializedTypes = typeReader.GetNumSerializedTypes();
                if ( numSerializedTypes == 0 )
                {
                    return Error( "Empty physics material library encountered: %s", libraryFilePath.c_str() );
                }

                for ( auto i = 0; i < numSerializedTypes; i++ )
                {
                    MaterialSettings material;
                    typeReader >> material;

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
            else
            {
                return Error( "ResourceCompiler", "Failed to read physics material library file: %s", libraryFilePath.c_str() );
            }
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, MaterialDatabase::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );

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