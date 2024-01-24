#include "CodeGenerator.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"
#include "Base/TypeSystem/TypeID.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Utils/TopologicalSort.h"
#include <eastl/sort.h>
#include <fstream>
#include <string>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static bool SortTypesByDependencies( TVector<ReflectedType>& structureTypes )
    {
        int32_t const numTypes = (int32_t) structureTypes.size();
        if ( numTypes <= 1 )
        {
            return true;
        }

        // Create list to sort
        TVector<TopologicalSorter::Node > list;
        for ( auto i = 0; i < numTypes; i++ )
        {
            list.push_back( TopologicalSorter::Node( i ) );
        }

        for ( auto i = 0; i < numTypes; i++ )
        {
            for ( auto j = 0; j < numTypes; j++ )
            {
                if ( i != j && structureTypes[j].m_ID == structureTypes[i].m_parentID )
                {
                    list[i].m_children.push_back( &list[j] );
                }
            }
        }

        // Try to sort
        if ( !TopologicalSorter::Sort( list ) )
        {
            return false;
        }

        // Update type list
        TVector<ReflectedType> sortedTypes;
        sortedTypes.reserve( numTypes );

        for ( auto& node : list )
        {
            sortedTypes.push_back( structureTypes[node.m_ID] );
        }
        structureTypes.swap( sortedTypes );

        return true;
    }

    //-------------------------------------------------------------------------
    // Generator
    //-------------------------------------------------------------------------

    bool CodeGenerator::SaveStreamToFile( FileSystem::Path const& filePath, std::stringstream& stream )
    {
        bool fileContentsEqual = true;

        // Rewind stream to beginning
        stream.seekg( std::ios::beg );

        // Open existing file and compare contents to the newly generated stream
        std::ifstream existingFile( filePath.c_str(), std::ios::in );
        if ( existingFile.is_open() )
        {
            std::string lineNew, lineExisting;
            while ( getline( existingFile, lineExisting ) && fileContentsEqual )
            {
                if ( !getline( stream, lineNew ) || ( lineExisting != lineNew ) )
                {
                    fileContentsEqual = false;
                }
            }

            // Set different if the stream is longer than the file
            if ( fileContentsEqual && getline( stream, lineNew ) )
            {
                fileContentsEqual = false;
            }

            existingFile.close();
        }
        else
        {
            // std::cout << "Error opening existing file: " << strerror( errno );
            fileContentsEqual = false;
        }

        // If the contents differ overwrite the existing file
        if ( !fileContentsEqual )
        {
            std::fstream outputFile( filePath.c_str(), std::ios::out | std::ios::trunc );
            if ( !outputFile.is_open() )
            {
                return false;
            }

            stream.seekg( std::ios::beg );
            outputFile << stream.rdbuf();
            outputFile.close();
        }

        return true;
    }

    bool CodeGenerator::LogError( char const* pFormat, ... ) const
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        m_errorMessage.assign( buffer );
        return false;
    }

    void CodeGenerator::LogWarning( char const* pFormat, ... ) const
    {
        char buffer[256];

        va_list args;
        va_start( args, pFormat );
        VPrintf( buffer, 256, pFormat, args );
        va_end( args );

        m_warningMessage.assign( buffer );
    }

    //-------------------------------------------------------------------------
    // Project Generation Functions
    //-------------------------------------------------------------------------

    bool CodeGenerator::GenerateCodeForSolution( SolutionInfo const& solutionInfo )
    {
        // Generate code per project
        //-------------------------------------------------------------------------

        for ( ProjectInfo const& prj : solutionInfo.m_projects )
        {
            // Ignore module less projects
            if ( !prj.m_moduleHeaderID.IsValid() )
            {
                continue;
            }

            GenerateCodeForProject( prj );
        }

        // Generate solution type registration files
        //-------------------------------------------------------------------------

        if ( !GenerateSolutionTypeRegistrationFile( solutionInfo, CompilationMode::Runtime ) )
        {
            return false;
        }

        if ( !GenerateSolutionTypeRegistrationFile( solutionInfo, CompilationMode::Tools ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool CodeGenerator::GenerateCodeForProject( ProjectInfo const& projectInfo )
    {
        // Ensure the auto generated directory exists
        //-------------------------------------------------------------------------

        FileSystem::Path const projectAutoGenDirectoryPath = projectInfo.GetAutogeneratedDirectoryPath();
        projectAutoGenDirectoryPath.EnsureDirectoryExists();

        // Delete any unknown files from the auto generated directory
        //-------------------------------------------------------------------------

        TVector<FileSystem::Path> files;
        FileSystem::GetDirectoryContents( projectAutoGenDirectoryPath, files, FileSystem::DirectoryReaderOutput::OnlyFiles );
        for ( auto const& file : files )
        {
            FileSystem::EraseFile( file );
        }

        // Generate code files for the dirty headers
        //-------------------------------------------------------------------------

        TVector<FileSystem::Path> generatedTypeInfoHeaders;
        TVector<FileSystem::Path> generatedCodeGenHeaders;

        for ( HeaderInfo const& headerInfo : projectInfo.m_headerFiles )
        {
            if ( headerInfo.m_ID == projectInfo.m_moduleHeaderID )
            {
                continue;
            }

            TVector<ReflectedType> typesInHeader;
            m_pDatabase->GetAllTypesForHeader( headerInfo.m_ID, typesInHeader );
            if ( !typesInHeader.empty() )
            {
                // Generate typeinfo file for header
                generatedTypeInfoHeaders.emplace_back( headerInfo.GetTypeInfoFilePath( projectAutoGenDirectoryPath ) );
                if ( !GenerateTypeInfoFileForHeader( projectInfo, headerInfo, typesInHeader, generatedTypeInfoHeaders.back() ) )
                {
                    return false;
                }

                //-------------------------------------------------------------------------

                // Generate code gen file for header
                bool hasStructureType = false;
                for ( ReflectedType const& typeInfo : typesInHeader )
                {
                    if ( !typeInfo.IsEnum() )
                    {
                        hasStructureType = true;
                        break;
                    }
                }

                if ( hasStructureType && !projectInfo.IsBaseModule() )
                {
                    // Check if the header declares any entity components
                    bool hasEntityComponentTypeInfo = false;
                    for ( ReflectedType const& typeInfo : typesInHeader )
                    {
                        if ( typeInfo.IsEntityComponent() )
                        {
                            hasEntityComponentTypeInfo = true;
                            break;
                        }
                    }

                    // Generate code gen header
                    if ( hasEntityComponentTypeInfo )
                    {
                        generatedCodeGenHeaders.emplace_back( headerInfo.GetCodeGenFilePath( projectAutoGenDirectoryPath ) );
                        if ( !GenerateCodeGenFileForHeader( projectInfo, headerInfo, typesInHeader, generatedCodeGenHeaders.back() ) )
                        {
                            return false;
                        }
                    }
                }
            }
        }

        // Generate the module files
        //-------------------------------------------------------------------------

        if ( !GenerateProjectTypeInfoFile( projectInfo, generatedTypeInfoHeaders ) )
        {
            return false;
        }

        if ( !projectInfo.IsBaseModule() )
        {
            if ( !GenerateProjectCodeGenFile( projectInfo, generatedCodeGenHeaders ) )
            {
                return false;
            }
        }

        return true;
    }

    bool CodeGenerator::GenerateProjectTypeInfoFile( ProjectInfo const& projectInfo, TVector<FileSystem::Path> const& generatedHeaders )
    {
        // Get all types in project
        TVector<ReflectedType> typesInProject;
        m_pDatabase->GetAllTypesForProject( projectInfo.m_ID, typesInProject );
        if ( !SortTypesByDependencies( typesInProject ) )
        {
            return LogError( "Cyclic header dependency detected in project: %s", projectInfo.m_name.c_str() );
        }

        // Header
        //-------------------------------------------------------------------------

        std::stringstream projectFile;
        projectFile.str( std::string() );
        projectFile.clear();
        projectFile << "//-------------------------------------------------------------------------\n";
        projectFile << "// This is an auto-generated file - DO NOT edit\n";
        projectFile << "//-------------------------------------------------------------------------\n\n";
        projectFile << "#include \"../API.h\"\n";
        projectFile << "#include \"Base/TypeSystem/TypeRegistry.h\"\n";
        projectFile << "#include \"Base/TypeSystem/EnumInfo.h\"\n";
        projectFile << "#include \"Base/Resource/ResourceSystem.h\"\n";
        projectFile << "#include \"" << projectInfo.GetModuleHeaderInfo().m_filePath.c_str() << "\"\n\n";
        projectFile << "//-------------------------------------------------------------------------\n\n";

        // Includes
        //-------------------------------------------------------------------------

        for ( auto& file : generatedHeaders )
        {
            projectFile << "#include \"" << file << "\"\n";
        }

        projectFile << "\n";
        projectFile << "//-------------------------------------------------------------------------\n\n";

        // Registration functions
        //-------------------------------------------------------------------------

        projectFile << "void " << projectInfo.m_moduleClassName.c_str() << "::RegisterTypes( TypeSystem::TypeRegistry& typeRegistry )\n{\n";

        for ( ReflectedType const& type : typesInProject )
        {
            InlineString str;

            if ( type.m_isDevOnly )
            {
                str.sprintf( "    EE_DEVELOPMENT_TOOLS_ONLY( TypeSystem::TTypeInfo<%s%s>::RegisterType( typeRegistry ) );\n", type.m_namespace.c_str(), type.m_name.c_str() );
            }
            else
            {
                str.sprintf( "    TypeSystem::TTypeInfo<%s%s>::RegisterType( typeRegistry );\n", type.m_namespace.c_str(), type.m_name.c_str() );
            }

            projectFile << str.c_str();
        }

        projectFile << "}\n\n";
        projectFile << "void " << projectInfo.m_moduleClassName.c_str() << "::UnregisterTypes( TypeSystem::TypeRegistry& typeRegistry )\n{\n";

        for ( auto iter = typesInProject.rbegin(); iter != typesInProject.rend(); ++iter )
        {
            InlineString str;

            if ( iter->m_isDevOnly )
            {
                str.sprintf( "    EE_DEVELOPMENT_TOOLS_ONLY( TypeSystem::TTypeInfo<%s%s>::UnregisterType( typeRegistry ) );\n", iter->m_namespace.c_str(), iter->m_name.c_str() );
            }
            else
            {
                str.sprintf( "    TypeSystem::TTypeInfo<%s%s>::UnregisterType( typeRegistry );\n", iter->m_namespace.c_str(), iter->m_name.c_str() );
            }

            projectFile << str.c_str();
        }

        projectFile << "}\n\n";

        // Save File
        //-------------------------------------------------------------------------

        FileSystem::Path const& outputPath = projectInfo.GetTypeInfoFilePath();
        if ( !SaveStreamToFile( outputPath, projectFile ) )
        {
            return LogError( "Could not save project type registration file to disk: %s", outputPath.c_str() );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool CodeGenerator::GenerateProjectCodeGenFile( ProjectInfo const& projectInfo, TVector<FileSystem::Path> const& generatedHeaders )
    {
        // Get all types in project
        TVector<ReflectedType> typesInProject;
        m_pDatabase->GetAllTypesForProject( projectInfo.m_ID, typesInProject );
        if ( !SortTypesByDependencies( typesInProject ) )
        {
            return LogError( "Cyclic header dependency detected in project: %s", projectInfo.m_name.c_str() );
        }

        // Header
        //-------------------------------------------------------------------------

        std::stringstream projectFile;
        projectFile.str( std::string() );
        projectFile.clear();
        projectFile << "//-------------------------------------------------------------------------\n";
        projectFile << "// This is an auto-generated file - DO NOT edit\n";
        projectFile << "//-------------------------------------------------------------------------\n\n";

        // Includes
        //-------------------------------------------------------------------------

        for ( auto& file : generatedHeaders )
        {
            projectFile << "#include \"" << file << "\"\n";
        }

        // Save File
        //-------------------------------------------------------------------------

        FileSystem::Path const& outputPath = projectInfo.GetCodeGenFilePath();
        if ( !SaveStreamToFile( outputPath, projectFile ) )
        {
            return LogError( "Could not save project type registration file to disk: %s", outputPath.c_str() );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool CodeGenerator::GenerateSolutionTypeRegistrationFile( SolutionInfo const& solutionInfo, CompilationMode mode )
    {
        //-------------------------------------------------------------------------
        // PREPARE DATA
        //-------------------------------------------------------------------------

        // Get all project and sort them according to dependency order
        //-------------------------------------------------------------------------

        auto sortPredicate = [] ( ProjectInfo const& pA, ProjectInfo const& pB )
        {
            return pA.m_dependencyCount < pB.m_dependencyCount;
        };

        TVector<ProjectInfo> projects = m_pDatabase->GetAllRegisteredProjects();

        for ( auto i = 0; i < projects.size(); i++ )
        {
            // Ignore tools modules for engine headers
            if ( mode == CompilationMode::Runtime )
            {
                if ( projects[i].m_isToolsModule )
                {
                    projects.erase_unsorted( projects.begin() + i );
                    i--;
                    continue;
                }
            }

            // Ignore module-less modules
            if ( !projects[i].m_moduleHeaderID.IsValid() )
            {
                projects.erase_unsorted( projects.begin() + i );
                i--;
                continue;
            }
        }

        eastl::sort( projects.begin(), projects.end(), sortPredicate );

        //-------------------------------------------------------------------------
        // GENERATE
        //-------------------------------------------------------------------------

        // Header
        //-------------------------------------------------------------------------

        std::stringstream fileStream;
        fileStream.str( std::string() );
        fileStream.clear();
        fileStream << "//-------------------------------------------------------------------------\n";
        fileStream << "// This is an auto-generated file - DO NOT edit\n";
        fileStream << "//-------------------------------------------------------------------------\n\n";
        fileStream << "#include \"Base/TypeSystem/TypeRegistry.h\"\n";
        fileStream << "#include \"Base/TypeSystem/EnumInfo.h\"\n\n";

        // Module Includes
        //-------------------------------------------------------------------------

        for ( auto i = 0; i < projects.size(); i++ )
        {
            HeaderInfo const& moduleHeaderInfo = projects[i].GetModuleHeaderInfo();
            fileStream << "#include \"" << moduleHeaderInfo.m_filePath.c_str() << "\"\n";
        }

        fileStream << "\n//-------------------------------------------------------------------------\n\n";

        // Namespace
        //-------------------------------------------------------------------------

        fileStream << "namespace EE::TypeSystem::Reflection\n";
        fileStream << "{\n";

        // Resource Registration
        //-------------------------------------------------------------------------

        GenerateResourceRegistrationMethods( fileStream, solutionInfo );

        fileStream << "\n    //-------------------------------------------------------------------------\n\n";

        // Registration function
        //-------------------------------------------------------------------------

        fileStream << "    inline void RegisterTypes( TypeSystem::TypeRegistry& typeRegistry )\n";
        fileStream << "    {\n";

        for ( ProjectInfo const& projectInfo : projects )
        {
            fileStream << "        " << projectInfo.m_moduleClassName.c_str() << "::RegisterTypes( typeRegistry );\n";
        }

        fileStream << "\n        //-------------------------------------------------------------------------\n\n";
        fileStream << "        RegisterResourceTypes( typeRegistry );\n";

        fileStream << "    }\n\n";

        // Unregistration functions
        //-------------------------------------------------------------------------

        fileStream << "    inline void UnregisterTypes( TypeSystem::TypeRegistry& typeRegistry )\n";
        fileStream << "    {\n";

        for ( auto iter = projects.rbegin(); iter != projects.rend(); ++iter )
        {
            fileStream << "        " << iter->m_moduleClassName.c_str() << "::UnregisterTypes( typeRegistry );\n";
        }

        fileStream << "\n        //-------------------------------------------------------------------------\n\n";
        fileStream << "        UnregisterResourceTypes( typeRegistry );\n";

        fileStream << "    }\n";

        // Namespace
        //-------------------------------------------------------------------------

        fileStream << "}\n";

        // Save file
        //-------------------------------------------------------------------------

        FileSystem::Path engineTypeRegistrationFilePath = solutionInfo.m_path + Reflection::Settings::g_globalAutoGeneratedDirectory;

        if ( mode == CompilationMode::Runtime )
        {
            engineTypeRegistrationFilePath.Append( Reflection::Settings::g_runtimeTypeRegistrationHeaderPath );
        }
        else
        {
            engineTypeRegistrationFilePath.Append( Reflection::Settings::g_toolsTypeRegistrationHeaderPath );
        }

        engineTypeRegistrationFilePath.EnsureDirectoryExists();
        if ( !SaveStreamToFile( engineTypeRegistrationFilePath, fileStream ) )
        {
            return LogError( "Could not save type registration header to disk: %s", engineTypeRegistrationFilePath.c_str() );
        }

        return true;
    }

    bool CodeGenerator::GenerateTypeInfoFileForHeader( ProjectInfo const& projectInfo, HeaderInfo const& headerInfo, TVector<ReflectedType> const& typesInHeader, FileSystem::Path const& outputPath )
    {
        // File Header
        //-------------------------------------------------------------------------

        std::stringstream outputFileStream;
        outputFileStream.str( std::string() );
        outputFileStream.clear();
        outputFileStream << "#pragma once\n\n";
        outputFileStream << "//*************************************************************************\n";
        outputFileStream << "// This is an auto-generated file - DO NOT edit\n";
        outputFileStream << "//*************************************************************************\n\n";
        outputFileStream << "#include \"" << headerInfo.m_filePath.c_str() << "\"\n\n";

        // Get all types for the header
        //-------------------------------------------------------------------------

        for ( ReflectedType const& typeInfo : typesInHeader )
        {
            // Generate enum info
            if ( typeInfo.IsEnum() )
            {
                GenerateEnumTypeInfo( outputFileStream, projectInfo.m_exportMacro, typeInfo );
            }
            else // Generate type info
            {
                // Validation
                if ( !typeInfo.m_parentID.IsValid() )
                {
                    String const fullTypeName = typeInfo.m_namespace + typeInfo.m_name;
                    return LogError( "Invalid parent hierarchy for type (%s), all registered types must derived from a registered type.", fullTypeName.c_str() );
                }

                ReflectedType const* pParentTypeInfo = m_pDatabase->GetType( typeInfo.m_parentID );
                EE_ASSERT( pParentTypeInfo != nullptr );
                GenerateStructureTypeInfo( outputFileStream, projectInfo.m_exportMacro, typeInfo, *pParentTypeInfo );
                outputFileStream << "\n";
            }
        }

        // Save generated file
        //-------------------------------------------------------------------------

        EE_ASSERT( outputPath.IsValid() );
        if ( !SaveStreamToFile( outputPath, outputFileStream ) )
        {
            return LogError( "Could not save type info file to disk: %s", outputPath.c_str() );
        }

        return true;
    }

    bool CodeGenerator::GenerateCodeGenFileForHeader( ProjectInfo const& projectInfo, HeaderInfo const& headerInfo, TVector<ReflectedType> const& typesInHeader, FileSystem::Path const& outputPath )
    {
        // File Header
        //-------------------------------------------------------------------------

        std::stringstream outputFileStream;
        outputFileStream.str( std::string() );
        outputFileStream.clear();
        outputFileStream << "#pragma once\n\n";
        outputFileStream << "//*************************************************************************\n";
        outputFileStream << "// This is an auto-generated file - DO NOT edit\n";
        outputFileStream << "//*************************************************************************\n\n";
        outputFileStream << "#include \"" << headerInfo.m_filePath.c_str() << "\"\n\n";

        // Get all types for the header
        //-------------------------------------------------------------------------

        for ( ReflectedType const& typeInfo : typesInHeader )
        {
            if ( !typeInfo.IsEntityComponent() )
            {
                continue;
            }

            String const fullTypeName( typeInfo.m_namespace + typeInfo.m_name );

            // Header
            //-------------------------------------------------------------------------

            outputFileStream << "//-------------------------------------------------------------------------\n";
            outputFileStream << "// Type: " << fullTypeName.c_str() << "\n";
            outputFileStream << "//-------------------------------------------------------------------------\n\n";

            // Dev Flag
            //-------------------------------------------------------------------------

            if ( typeInfo.m_isDevOnly )
            {
                outputFileStream << "#if EE_DEVELOPMENT_TOOLS\n";
            }

            // Generate entity component methods
            //-------------------------------------------------------------------------

            if ( typeInfo.IsEntityComponent() )
            {
                // Generate Load Method
                //-------------------------------------------------------------------------

                outputFileStream << "void " << fullTypeName.c_str() << "::Load( EntityModel::LoadingContext const& context, Resource::ResourceRequesterID const& requesterID )\n";
                outputFileStream << "{\n";

                if ( typeInfo.HasProperties() )
                {
                    outputFileStream << "    " << fullTypeName.c_str() << "::s_pTypeInfo->LoadResources( context.m_pResourceSystem, requesterID, this );\n";
                    outputFileStream << "    m_status = Status::Loading;\n";
                }
                else
                {
                    outputFileStream << "    m_status = Status::Loaded;\n";
                }

                outputFileStream << "}\n";

                // Generate Unload Method
                //-------------------------------------------------------------------------

                outputFileStream << "\n";
                outputFileStream << "void " << fullTypeName.c_str() << "::Unload( EntityModel::LoadingContext const& context, Resource::ResourceRequesterID const& requesterID )\n";
                outputFileStream << "{\n";

                if ( typeInfo.HasProperties() )
                {
                    outputFileStream << "    " << fullTypeName.c_str() << "::s_pTypeInfo->UnloadResources( context.m_pResourceSystem, requesterID, this );\n";
                }

                outputFileStream << "    m_status = Status::Unloaded;\n";
                outputFileStream << "}\n";

                // Generate Update Status Method
                //-------------------------------------------------------------------------

                outputFileStream << "\n";
                outputFileStream << "void " << fullTypeName.c_str() << "::UpdateLoading()\n";
                outputFileStream << "{\n";
                outputFileStream << "    if( m_status == Status::Loading )\n";
                outputFileStream << "    {\n";

                {
                    if ( typeInfo.HasProperties() )
                    {
                        // Wait for resources to be loaded
                        outputFileStream << "        auto const resourceLoadingStatus = " << fullTypeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( this );\n";
                        outputFileStream << "        if ( resourceLoadingStatus == LoadingStatus::Loading )\n";
                        outputFileStream << "        {\n";
                        outputFileStream << "           return; // Something is still loading so early-out\n";
                        outputFileStream << "        }\n\n";

                        // Set status
                        outputFileStream << "        if ( resourceLoadingStatus == LoadingStatus::Failed )\n";
                        outputFileStream << "        {\n";
                        outputFileStream << "           m_status = EntityComponent::Status::LoadingFailed;\n";
                        outputFileStream << "        }\n";
                        outputFileStream << "        else\n";
                        outputFileStream << "        {\n";
                        outputFileStream << "           m_status = EntityComponent::Status::Loaded;\n";
                        outputFileStream << "        }\n";
                    }
                    else
                    {
                        outputFileStream << "        m_status = EntityComponent::Status::Loaded;\n";
                    }
                }

                outputFileStream << "    }\n";
                outputFileStream << "}\n";
            }

            // Dev Flag
            //-------------------------------------------------------------------------

            if ( typeInfo.m_isDevOnly )
            {
                outputFileStream << "#endif\n";
            }
        }

        // Save generated file
        //-------------------------------------------------------------------------

        EE_ASSERT( outputPath.IsValid() );

        if ( !SaveStreamToFile( outputPath, outputFileStream ) )
        {
            return LogError( "Could not save type info file to disk: %s", outputPath.c_str() );
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Resources
    //-------------------------------------------------------------------------

    bool CodeGenerator::GenerateResourceRegistrationMethods( std::stringstream& outputFileStream, SolutionInfo const& solutionInfo )
    {
        TVector<ReflectedResourceType> const& registeredResourceTypes = m_pDatabase->GetAllRegisteredResourceTypes();

        // Registration function
        //-------------------------------------------------------------------------

        outputFileStream << "    inline void RegisterResourceTypes( TypeSystem::TypeRegistry& typeRegistry )\n";
        outputFileStream << "    {\n";

        if ( !registeredResourceTypes.empty() )
        {
            outputFileStream << "        TypeSystem::ResourceInfo resourceInfo;\n";
        }

        auto GetResourceTypeIDForTypeID = [&registeredResourceTypes] ( TypeID typeID )
        {
            for ( auto const& registeredResourceType : registeredResourceTypes )
            {
                if ( registeredResourceType.m_typeID == typeID )
                {
                    return registeredResourceType.m_resourceTypeID;
                }
            }

            EE_UNREACHABLE_CODE();
            return ResourceTypeID();
        };

        for ( auto& registeredResourceType : registeredResourceTypes )
        {
            outputFileStream << "\n";
            outputFileStream << "        resourceInfo.m_typeID = TypeSystem::TypeID( \"" << registeredResourceType.m_typeID.c_str() << "\");\n";
            outputFileStream << "        resourceInfo.m_resourceTypeID = ResourceTypeID( \"" << registeredResourceType.m_resourceTypeID.ToString().c_str() << "\" );\n";
            outputFileStream << "        resourceInfo.m_parentTypes.clear();\n";

            for ( auto const& parentType : registeredResourceType.m_parents )
            {
                ResourceTypeID const resourceTypeID = GetResourceTypeIDForTypeID( parentType );
                outputFileStream << "        resourceInfo.m_parentTypes.emplace_back( ResourceTypeID( \"" << resourceTypeID.ToString().c_str() << "\" ) );\n";
            }

            outputFileStream << "        EE_DEVELOPMENT_TOOLS_ONLY( resourceInfo.m_friendlyName = \"" << registeredResourceType.m_friendlyName.c_str() << "\" );\n";
            outputFileStream << "        typeRegistry.RegisterResourceTypeID( resourceInfo );\n";
        }

        outputFileStream << "    }\n\n";

        // Unregistration functions
        //-------------------------------------------------------------------------

        outputFileStream << "    inline void UnregisterResourceTypes( TypeSystem::TypeRegistry& typeRegistry )\n";
        outputFileStream << "    {\n";

        for ( auto iter = registeredResourceTypes.rbegin(); iter != registeredResourceTypes.rend(); ++iter )
        {
            outputFileStream << "        typeRegistry.UnregisterResourceTypeID( TypeSystem::TypeID( \"" << iter->m_typeID.c_str() << "\" ) );\n";
        }

        outputFileStream << "    }\n";

        return true;
    }

    //-------------------------------------------------------------------------
    // Type Info
    //-------------------------------------------------------------------------

    void CodeGenerator::GenerateEnumTypeInfo( std::stringstream& outputFileStream, String const& exportMacro, ReflectedType const& typeInfo )
    {
        String const fullTypeName( typeInfo.m_namespace + typeInfo.m_name );

        TInlineVector<int32_t, 50> sortingConstantIndices; // Sorted list of constant indices
        TInlineVector<int32_t, 50> sortedOrder; // Final order for each constant

        for ( auto i = 0u; i < typeInfo.m_enumConstants.size(); i++ )
        {
            sortingConstantIndices.emplace_back( i );
        }

        auto Comparator = [&typeInfo] ( int32_t a, int32_t b )
        {
            auto const& elemA = typeInfo.m_enumConstants[a];
            auto const& elemB = typeInfo.m_enumConstants[b];
            return elemA.m_label < elemB.m_label;
        };

        eastl::sort( sortingConstantIndices.begin(), sortingConstantIndices.end(), Comparator );

        sortedOrder.resize( sortingConstantIndices.size() );

        for ( auto i = 0u; i < typeInfo.m_enumConstants.size(); i++ )
        {
            sortedOrder[sortingConstantIndices[i]] = i;
        }

        //-------------------------------------------------------------------------

        outputFileStream << "\n";
        outputFileStream << "//-------------------------------------------------------------------------\n";
        outputFileStream << "// Enum Info: " << fullTypeName.c_str() << "\n";
        outputFileStream << "//-------------------------------------------------------------------------\n\n";

        if ( typeInfo.m_isDevOnly )
        {
            outputFileStream << "#if EE_DEVELOPMENT_TOOLS\n";
        }

        outputFileStream << "namespace EE::TypeSystem\n";
        outputFileStream << "{\n";
        outputFileStream << "    template<>\n";
        outputFileStream << "    class " /*<< exportMacro.c_str()*/ << " TTypeInfo<" << fullTypeName.c_str() << "> final : public TypeInfo\n";
        outputFileStream << "    {\n";
        outputFileStream << "        static TypeInfo* s_pInstance;\n\n";
        outputFileStream << "    public:\n\n";

        // Static registration Function
        //-------------------------------------------------------------------------

        outputFileStream << "        static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        outputFileStream << "        {\n";
        outputFileStream << "            EE_ASSERT( s_pInstance == nullptr );\n";
        outputFileStream << "            s_pInstance = EE::New<" << " TTypeInfo<" << fullTypeName.c_str() << ">>();\n";
        outputFileStream << "            s_pInstance->m_ID = TypeSystem::TypeID( \"" << fullTypeName.c_str() << "\" );\n";
        outputFileStream << "            s_pInstance->m_size = sizeof( " << fullTypeName.c_str() << " );\n";
        outputFileStream << "            s_pInstance->m_alignment = alignof( " << fullTypeName.c_str() << " );\n";
        outputFileStream << "            typeRegistry.RegisterType( s_pInstance );\n\n";

        outputFileStream << "            TypeSystem::EnumInfo enumInfo;\n";
        outputFileStream << "            enumInfo.m_ID = TypeSystem::TypeID( \"" << fullTypeName.c_str() << "\" );\n";

        switch ( typeInfo.m_underlyingType )
        {
            case TypeSystem::CoreTypeID::Uint8:
            outputFileStream << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Uint8;\n";
            break;

            case TypeSystem::CoreTypeID::Int8:
            outputFileStream << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Int8;\n";
            break;

            case TypeSystem::CoreTypeID::Uint16:
            outputFileStream << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Uint16;\n";
            break;

            case TypeSystem::CoreTypeID::Int16:
            outputFileStream << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Int16;\n";
            break;

            case TypeSystem::CoreTypeID::Uint32:
            outputFileStream << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Uint32;\n";
            break;

            case TypeSystem::CoreTypeID::Int32:
            outputFileStream << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Int32;\n";
            break;

            default:
            EE_HALT();
            break;
        }

        outputFileStream << "\n";
        outputFileStream << "            //-------------------------------------------------------------------------\n\n";

        outputFileStream << "            TypeSystem::EnumInfo::ConstantInfo constantInfo;\n";

        for ( auto i = 0u; i < typeInfo.m_enumConstants.size(); i++ )
        {
            String escapedDescription = typeInfo.m_enumConstants[i].m_description;
            StringUtils::ReplaceAllOccurrencesInPlace( escapedDescription, "\"", "\\\"" );

            outputFileStream << "\n";
            outputFileStream << "            constantInfo.m_ID = StringID( \"" << typeInfo.m_enumConstants[i].m_label.c_str() << "\" );\n";
            outputFileStream << "            constantInfo.m_value = " << typeInfo.m_enumConstants[i].m_value << ";\n";
            outputFileStream << "            constantInfo.m_alphabeticalOrder = " << sortedOrder[i] << ";\n";
            outputFileStream << "            EE_DEVELOPMENT_TOOLS_ONLY( constantInfo.m_description = \"" << escapedDescription.c_str() << "\" );\n";
            outputFileStream << "            enumInfo.m_constants.emplace_back( constantInfo );\n";
        }

        outputFileStream << "\n";
        outputFileStream << "            //-------------------------------------------------------------------------\n\n";
        outputFileStream << "            typeRegistry.RegisterEnum( enumInfo );\n";
        outputFileStream << "        }\n\n";

        // Static unregistration Function
        //-------------------------------------------------------------------------

        outputFileStream << "        static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        outputFileStream << "        {\n";
        outputFileStream << "            EE_ASSERT( s_pInstance != nullptr );\n";
        outputFileStream << "            typeRegistry.UnregisterEnum( s_pInstance->m_ID );\n";
        outputFileStream << "            typeRegistry.UnregisterType( s_pInstance );\n";
        outputFileStream << "            EE::Delete( s_pInstance );\n";
        outputFileStream << "        }\n\n";

        // Constructor
        //-------------------------------------------------------------------------

        outputFileStream << "    public:\n\n";

        outputFileStream << "        TTypeInfo()\n";
        outputFileStream << "        {\n";

        // Create type info
        outputFileStream << "            m_ID = TypeSystem::TypeID( \"" << fullTypeName.c_str() << "\" );\n";
        outputFileStream << "            m_size = sizeof( " << fullTypeName.c_str() << " );\n";
        outputFileStream << "            m_alignment = alignof( " << fullTypeName.c_str() << " );\n\n";

        // Create dev tools info
        outputFileStream << "            EE_DEVELOPMENT_TOOLS_ONLY( m_friendlyName = \"" << typeInfo.GetFriendlyName().c_str() << "\" );\n";
        outputFileStream << "            EE_DEVELOPMENT_TOOLS_ONLY( m_category = \"" << typeInfo.GetCategory().c_str() << "\" );\n";

        outputFileStream << "        }\n\n";

        // Implement required virtual methods
        //-------------------------------------------------------------------------

        outputFileStream << "        virtual IReflectedType* CreateType() const override { EE_HALT(); return nullptr; }\n";
        outputFileStream << "        virtual void CreateTypeInPlace( IReflectedType * pAllocatedMemory ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual void LoadResources( Resource::ResourceSystem * pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType * pType ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual void UnloadResources( Resource::ResourceSystem * pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType * pType ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual LoadingStatus GetResourceLoadingStatus( IReflectedType * pType ) const override { EE_HALT(); return LoadingStatus::Failed; }\n";
        outputFileStream << "        virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType * pType ) const override { EE_HALT(); return LoadingStatus::Failed; }\n";
        outputFileStream << "        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType * pType, uint32_t propertyID ) const override { EE_HALT(); return ResourceTypeID(); }\n";
        outputFileStream << "        virtual void GetReferencedResources( IReflectedType * pType, TVector<ResourceID>&outReferencedResources ) const override {};\n";
        outputFileStream << "        virtual uint8_t* GetArrayElementDataPtr( IReflectedType * pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const override { EE_HALT(); return 0; }\n";
        outputFileStream << "        virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint32_t arrayID ) const override { EE_HALT(); return 0; }\n";
        outputFileStream << "        virtual size_t GetArrayElementSize( uint32_t arrayID ) const override { EE_HALT(); return 0; }\n";
        outputFileStream << "        virtual void ClearArray( IReflectedType * pTypeInstance, uint32_t arrayID ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual void AddArrayElement( IReflectedType * pTypeInstance, uint32_t arrayID ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t insertIdx ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual void RemoveArrayElement( IReflectedType * pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const override { EE_HALT(); }\n";
        outputFileStream << "        virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const override { EE_HALT(); return false; }\n";
        outputFileStream << "        virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const override { EE_HALT(); return false; }\n";
        outputFileStream << "        virtual void ResetToDefault( IReflectedType * pTypeInstance, uint32_t propertyID ) const override { EE_HALT(); }\n";

        //-------------------------------------------------------------------------

        outputFileStream << "    };\n\n";

        outputFileStream << "    TypeInfo* TTypeInfo<" << fullTypeName.c_str() << ">::s_pInstance = nullptr;\n";

        outputFileStream << "}\n";

        if ( typeInfo.m_isDevOnly )
        {
            outputFileStream << "#endif\n";
        }
    }

    void CodeGenerator::GenerateStructureTypeInfo( std::stringstream& outputFileStream, String const& exportMacro, ReflectedType const& typeInfo, ReflectedType const& parentType )
    {
        String const fullTypeName( typeInfo.m_namespace + typeInfo.m_name );

        // Dev Flag
        if ( typeInfo.m_isDevOnly )
        {
            outputFileStream << "#if EE_DEVELOPMENT_TOOLS\n";
        }

        // Type Info
        //-------------------------------------------------------------------------

        outputFileStream << "//-------------------------------------------------------------------------\n";
        outputFileStream << "// TypeInfo: " << fullTypeName.c_str() << "\n";
        outputFileStream << "//-------------------------------------------------------------------------\n\n";
        outputFileStream << "namespace EE\n";
        outputFileStream << "{\n";
        outputFileStream << "    namespace TypeSystem\n";
        outputFileStream << "    {\n";
        outputFileStream << "        template<>\n";
        outputFileStream << "        class " /*<< exportMacro.c_str()*/ << " TTypeInfo<" << fullTypeName.c_str() << "> final : public TypeInfo\n";
        outputFileStream << "        {\n";
        outputFileStream << "           static " << fullTypeName.c_str() << " const* s_pDefaultInstance_" << typeInfo.m_ID.ToUint() << ";\n\n";
        outputFileStream << "        public:\n\n";

        GenerateTypeInfoStaticTypeRegistrationMethod( outputFileStream, typeInfo );
        GenerateTypeInfoStaticTypeUnregistrationMethod( outputFileStream, typeInfo );

        outputFileStream << "        public:\n\n";

        GenerateTypeInfoConstructor( outputFileStream, typeInfo, parentType );
        GenerateTypeInfoCreationMethod( outputFileStream, typeInfo );
        GenerateTypeInfoInPlaceCreationMethod( outputFileStream, typeInfo );
        GenerateTypeInfoLoadResourcesMethod( outputFileStream, typeInfo );
        GenerateTypeInfoUnloadResourcesMethod( outputFileStream, typeInfo );
        GenerateTypeInfoResourceLoadingStatusMethod( outputFileStream, typeInfo );
        GenerateTypeInfoResourceUnloadingStatusMethod( outputFileStream, typeInfo );
        GenerateTypeInfoGetReferencedResourceMethod( outputFileStream, typeInfo );
        GenerateTypeInfoExpectedResourceTypeMethod( outputFileStream, typeInfo );
        GenerateTypeInfoArrayAccessorMethod( outputFileStream, typeInfo );
        GenerateTypeInfoArraySizeMethod( outputFileStream, typeInfo );
        GenerateTypeInfoArrayElementSizeMethod( outputFileStream, typeInfo );
        GenerateTypeInfoArrayClearMethod( outputFileStream, typeInfo );
        GenerateTypeInfoAddArrayElementMethod( outputFileStream, typeInfo );
        GenerateTypeInfoInsertArrayElementMethod( outputFileStream, typeInfo );
        GenerateTypeInfoMoveArrayElementMethod( outputFileStream, typeInfo );
        GenerateTypeInfoRemoveArrayElementMethod( outputFileStream, typeInfo );
        GenerateTypeInfoAreAllPropertiesEqualMethod( outputFileStream, typeInfo );
        GenerateTypeInfoIsPropertyEqualMethod( outputFileStream, typeInfo );
        GenerateTypeInfoSetToDefaultValueMethod( outputFileStream, typeInfo );

        outputFileStream << "        };\n\n";

        outputFileStream << "        " << fullTypeName.c_str() << " const*" << " EE::TypeSystem::TTypeInfo<" << fullTypeName.c_str() << ">::s_pDefaultInstance_" << typeInfo.m_ID.ToUint() << " = nullptr;\n";

        outputFileStream << "    }\n";
        outputFileStream << "}\n";

        // Dev Flag
        //-------------------------------------------------------------------------

        if ( typeInfo.m_isDevOnly )
        {
            outputFileStream << "#endif\n";
        }
    }

    //-------------------------------------------------------------------------
    // Structure Type Info
    //-------------------------------------------------------------------------

    void CodeGenerator::GenerateTypeInfoCreationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual IReflectedType* CreateType() const override final\n";
        file << "            {\n";
        if ( !type.IsAbstract() )
        {
            file << "                auto pMemory = EE::Alloc( sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ), alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ) );\n";
            file << "                return new ( pMemory ) " << type.m_namespace.c_str() << type.m_name.c_str() << "();\n";
        }
        else
        {
            file << "                EE_HALT(); // Error! Trying to instantiate an abstract type!\n";
            file << "                return nullptr;\n";
        }
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoInPlaceCreationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void CreateTypeInPlace( IReflectedType* pAllocatedMemory ) const override final\n";
        file << "            {\n";
        if ( !type.IsAbstract() )
        {
            file << "                EE_ASSERT( pAllocatedMemory != nullptr );\n";
            file << "                new( pAllocatedMemory ) " << type.m_namespace.c_str() << type.m_name.c_str() << "();\n";
        }
        else
        {
            file << "                EE_HALT(); // Error! Trying to instantiate an abstract type!\n";
        }
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoArrayAccessorMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual uint8_t* GetArrayElementDataPtr( IReflectedType* pType, uint32_t arrayID, size_t arrayIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    if ( ( arrayIdx + 1 ) >= pActualType->" << propertyDesc.m_name.c_str() << ".size() )\n";
                    file << "                    {\n";
                    file << "                        pActualType->" << propertyDesc.m_name.c_str() << ".resize( arrayIdx + 1 );\n";
                    file << "                    }\n\n";
                    file << "                    return (uint8_t*) &pActualType->" << propertyDesc.m_name.c_str() << "[arrayIdx];\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( propertyDesc.IsStaticArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    return (uint8_t*) &pActualType->" << propertyDesc.m_name.c_str() << "[arrayIdx];\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return nullptr;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoArraySizeMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint32_t arrayID ) const override final\n";
        file << "            {\n";

        if ( type.HasArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    return pActualType->" << propertyDesc.m_name.c_str() << ".size();\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( propertyDesc.IsStaticArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    return " << propertyDesc.GetArraySize() << ";\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return 0;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoArrayElementSizeMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual size_t GetArrayElementSize( uint32_t arrayID ) const override final\n";
        file << "            {\n";

        for ( auto& propertyDesc : type.m_properties )
        {
            String const templateSpecializationString = propertyDesc.m_templateArgTypeName.empty() ? String() : "<" + propertyDesc.m_templateArgTypeName + ">";

            if ( propertyDesc.IsArrayProperty() )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                file << "                {\n";
                file << "                    return sizeof( " << propertyDesc.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                file << "                }\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }

                file << "\n";
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return 0;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoArrayClearMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void ClearArray( IReflectedType* pTypeInstance, uint32_t arrayID ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".clear();\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoAddArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void AddArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".emplace_back();\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoInsertArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t insertionIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".emplace( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + insertionIdx );\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoMoveArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    auto const originalElement = pActualType->" << propertyDesc.m_name.c_str() << "[originalElementIdx];\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".erase( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + originalElementIdx );\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".insert( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + newElementIdx, originalElement );\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoRemoveArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void RemoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t elementIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".erase( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + elementIdx );\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoAreAllPropertiesEqualMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const override final\n";
        file << "            {\n";

        if ( type.HasProperties() )
        {
            file << "                auto pType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pTypeInstance );\n";
            file << "                auto pOtherType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pOtherTypeInstance );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if( !IsPropertyValueEqual( pType, pOtherType, " << propertyDesc.m_propertyID << " ) )\n";
                file << "                {\n";
                file << "                    return false;\n";
                file << "                }\n\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }
            }
        }

        file << "                return true;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoIsPropertyEqualMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const override final\n";
        file << "            {\n";

        if ( type.HasProperties() )
        {
            file << "                auto pType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pTypeInstance );\n";
            file << "                auto pOtherType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pOtherTypeInstance );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                InlineString propertyTypeName = propertyDesc.m_typeName.c_str();
                if ( !propertyDesc.m_templateArgTypeName.empty() )
                {
                    propertyTypeName += "<";
                    propertyTypeName += propertyDesc.m_templateArgTypeName.c_str();
                    propertyTypeName += ">";
                }

                //-------------------------------------------------------------------------

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                file << "                {\n";

                // Arrays
                if ( propertyDesc.IsArrayProperty() )
                {
                    // Handle individual element comparison
                    //-------------------------------------------------------------------------

                    file << "                    // Compare array elements\n";
                    file << "                    if ( arrayIdx != InvalidIndex )\n";
                    file << "                    {\n";

                    // If it's a dynamic array check the sizes first
                    if ( propertyDesc.IsDynamicArrayProperty() )
                    {
                        file << "                        if ( arrayIdx >= pOtherType->" << propertyDesc.m_name.c_str() << ".size() )\n";
                        file << "                        {\n";
                        file << "                        return false;\n";
                        file << "                        }\n\n";
                    }

                    if ( propertyDesc.IsStructureProperty() )
                    {
                        file << "                        return " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->AreAllPropertyValuesEqual( &pType->" << propertyDesc.m_name.c_str() << "[arrayIdx], &pOtherType->" << propertyDesc.m_name.c_str() << "[arrayIdx] );\n";
                    }
                    else
                    {
                        file << "                        return pType->" << propertyDesc.m_name.c_str() << "[arrayIdx] == pOtherType->" << propertyDesc.m_name.c_str() << "[arrayIdx];\n";
                    }
                    file << "                    }\n";

                    // Handle array comparison
                    //-------------------------------------------------------------------------

                    file << "                    else // Compare entire array contents\n";
                    file << "                    {\n";

                    // If it's a dynamic array check the sizes first
                    if ( propertyDesc.IsDynamicArrayProperty() )
                    {
                        file << "                        if ( pType->" << propertyDesc.m_name.c_str() << ".size() != pOtherType->" << propertyDesc.m_name.c_str() << ".size() )\n";
                        file << "                        {\n";
                        file << "                        return false;\n";
                        file << "                        }\n\n";

                        file << "                        for ( size_t i = 0; i < pType->" << propertyDesc.m_name.c_str() << ".size(); i++ )\n";
                    }
                    else
                    {
                        file << "                        for ( size_t i = 0; i < " << propertyDesc.GetArraySize() << "; i++ )\n";
                    }

                    file << "                        {\n";

                    if ( propertyDesc.IsStructureProperty() )
                    {
                        file << "                        if( !" << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->AreAllPropertyValuesEqual( &pType->" << propertyDesc.m_name.c_str() << "[i], &pOtherType->" << propertyDesc.m_name.c_str() << "[i] ) )\n";
                        file << "                        {\n";
                        file << "                            return false;\n";
                        file << "                        }\n";
                    }
                    else
                    {
                        file << "                        if( pType->" << propertyDesc.m_name.c_str() << "[i] != pOtherType->" << propertyDesc.m_name.c_str() << "[i] )\n";
                        file << "                        {\n";
                        file << "                            return false;\n";
                        file << "                        }\n";
                    }

                    file << "                        }\n\n";

                    file << "                        return true;\n";
                    file << "                    }\n";
                }
                else // Non-Array properties
                {
                    if ( propertyDesc.IsStructureProperty() )
                    {
                        file << "                    return " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->AreAllPropertyValuesEqual( &pType->" << propertyDesc.m_name.c_str() << ", &pOtherType->" << propertyDesc.m_name.c_str() << " );\n";
                    }
                    else
                    {
                        file << "                    return pType->" << propertyDesc.m_name.c_str() << " == pOtherType->" << propertyDesc.m_name.c_str() << ";\n";
                    }
                }

                file << "                }\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }

                file << "\n";
            }
        }
        else
        {
            file << "                EE_UNREACHABLE_CODE();\n";
        }

        file << "                return false;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoSetToDefaultValueMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void ResetToDefault( IReflectedType* pTypeInstance, uint32_t propertyID ) const override final\n";
        file << "            {\n";

        if ( type.HasProperties() )
        {
            file << "                auto pDefaultType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( m_pDefaultInstance );\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr && pDefaultType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                file << "                {\n";

                if ( propertyDesc.IsStaticArrayProperty() )
                {
                    for ( auto i = 0u; i < propertyDesc.GetArraySize(); i++ )
                    {
                        file << "                    pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] = pDefaultType->" << propertyDesc.m_name.c_str() << "[" << i << "];\n";
                    }
                }
                else
                {
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << " = pDefaultType->" << propertyDesc.m_name.c_str() << ";\n";
                }

                file << "                    return;\n";
                file << "                }\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }
            }
        }

        file << "            }\n";
    }

    void CodeGenerator::GenerateTypeInfoExpectedResourceTypeMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType* pType, uint32_t propertyID ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrProperties() )
        {
            for ( auto& propertyDesc : type.m_properties )
            {
                bool const isResourceProp = ( propertyDesc.m_typeID == CoreTypeID::ResourcePtr ) || ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr );
                if ( isResourceProp )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr )
                    {
                        file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                        file << "                {\n";
                        file << "                    return " << propertyDesc.m_templateArgTypeName.c_str() << "::GetStaticResourceTypeID();\n";
                        file << "                }\n";
                    }
                    else if ( propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                    {
                        file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                        file << "                {\n";
                        file << "                        return ResourceTypeID();\n";
                        file << "            }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n\n";
                    }
                    else
                    {
                        file << "\n";
                    }
                }
            }
        }

        file << "                // We should never get here since we are asking for a resource type of an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return ResourceTypeID();\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoLoadResourcesMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                EE_ASSERT( pResourceSystem != nullptr );\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.IsSet() )\n";
                            file << "                    {\n";
                            file << "                        pResourceSystem->LoadResource( resourcePtr, requesterID );\n";
                            file << "                    }\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsSet() )\n";
                                file << "                {\n";
                                file << "                    pResourceSystem->LoadResource( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "], requesterID );\n";
                                file << "                }\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() )\n";
                        file << "                {\n";
                        file << "                    pResourceSystem->LoadResource( pActualType->" << propertyDesc.m_name.c_str() << ", requesterID );\n";
                        file << "                }\n\n";
                    }
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->LoadResources( pResourceSystem, requesterID, &propertyValue );\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->LoadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] );\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->LoadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << " );\n\n";
                    }
                }

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n\n";
                }
            }
        }

        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoUnloadResourcesMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                EE_ASSERT( pResourceSystem != nullptr );\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.IsSet() )\n";
                            file << "                    {\n";
                            file << "                        pResourceSystem->UnloadResource( resourcePtr, requesterID );\n";
                            file << "                    }\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsSet() )\n";
                                file << "                {\n";
                                file << "                    pResourceSystem->UnloadResource( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "], requesterID );\n";
                                file << "                }\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() )\n";
                        file << "                {\n";
                        file << "                    pResourceSystem->UnloadResource( pActualType->" << propertyDesc.m_name.c_str() << ", requesterID );\n";
                        file << "                }\n\n";
                    }
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->UnloadResources( pResourceSystem, requesterID, &propertyValue );\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->UnloadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] );\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->UnloadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << " );\n\n";
                    }
                }

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n\n";
                }
            }
        }

        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoResourceLoadingStatusMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual LoadingStatus GetResourceLoadingStatus( IReflectedType* pType ) const override final\n";
        file << "            {\n";
        file << "                LoadingStatus status = LoadingStatus::Loaded;\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto const& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.HasLoadingFailed() )\n";
                            file << "                    {\n";
                            file << "                        status = LoadingStatus::Failed;\n";
                            file << "                    }\n";
                            file << "                    else if ( resourcePtr.IsSet() && !resourcePtr.IsLoaded() )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Loading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].HasLoadingFailed() )\n";
                                file << "                {\n";
                                file << "                    status = LoadingStatus::Failed;\n";
                                file << "                }\n";
                                file << "                else if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() && !pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsLoaded() )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Loading;\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".HasLoadingFailed() )\n";
                        file << "                {\n";
                        file << "                    status = LoadingStatus::Failed;\n";
                        file << "                }\n";
                        file << "                else if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() && !pActualType->" << propertyDesc.m_name.c_str() << ".IsLoaded() )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Loading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( &propertyValue );\n";
                            file << "                    if ( status == LoadingStatus::Loading )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Loading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] ); \n";
                                file << "                if ( status == LoadingStatus::Loading )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Loading;\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << " );\n";
                        file << "                if ( status == LoadingStatus::Loading )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Loading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                return status;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoResourceUnloadingStatusMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType* pType ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto const& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    EE_ASSERT( !resourcePtr.IsLoading() );\n";
                            file << "                    if ( !resourcePtr.IsUnloaded() )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Unloading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                EE_ASSERT( !pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsLoading() );\n";
                                file << "                if ( !pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsUnloaded() )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Unloading;\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                EE_ASSERT( !pActualType->" << propertyDesc.m_name.c_str() << ".IsLoading() );\n";
                        file << "                if ( !pActualType->" << propertyDesc.m_name.c_str() << ".IsUnloaded() )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Unloading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    LoadingStatus const status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceUnloadingStatus( &propertyValue );\n";
                            file << "                    if ( status != LoadingStatus::Unloaded )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Unloading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceUnloadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] ) != LoadingStatus::Unloaded )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Unloading;\n";
                                file << "                }\n";

                                if ( i == propertyDesc.m_arraySize - 1 )
                                {
                                    file << "\n";
                                }
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceUnloadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << " ) != LoadingStatus::Unloaded )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Unloading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                return LoadingStatus::Unloaded;\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoGetReferencedResourceMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void GetReferencedResources( IReflectedType* pType, TVector<ResourceID>& outReferencedResources ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto const& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.IsSet() )\n";
                            file << "                    {\n";
                            file << "                        outReferencedResources.emplace_back( resourcePtr.GetResourceID() );\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsSet() )\n";
                                file << "                {\n";
                                file << "                    outReferencedResources.emplace_back( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].GetResourceID() );\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() )\n";
                        file << "                {\n";
                        file << "                    outReferencedResources.emplace_back( pActualType->" << propertyDesc.m_name.c_str() << ".GetResourceID() );\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetReferencedResources( &propertyValue, outReferencedResources );\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetReferencedResources( &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "], outReferencedResources ); \n";
                            }
                        }
                    }
                    else
                    {
                        file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetReferencedResources( &pActualType->" << propertyDesc.m_name.c_str() << ", outReferencedResources );\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoStaticTypeRegistrationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        file << "            {\n";

        // Create default type instance
        if ( type.IsAbstract() )
        {
            file << "                IReflectedType* pDefaultTypeInstance = nullptr;\n";
        }
        else
        {
            file << "                auto pMemory = EE::Alloc( sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ), alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ) );\n";
            file << "                IReflectedType* pDefaultTypeInstance = new ( pMemory ) " << type.m_namespace.c_str() << type.m_name.c_str() << "();\n";
        }

        file << "                " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo = EE::New<TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << ">>( pDefaultTypeInstance );\n";
        file << "                typeRegistry.RegisterType( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo );\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoStaticTypeUnregistrationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        file << "            {\n";
        file << "                typeRegistry.UnregisterType( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo );\n";

        // Destroy default type instance
        if ( !type.IsAbstract() )
        {
            file << "                EE::Delete( const_cast<IReflectedType*&>( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo->m_pDefaultInstance ) );\n";
        }

        file << "                EE::Delete( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo );\n";
        file << "            }\n\n";
    }

    void CodeGenerator::GenerateTypeInfoConstructor( std::stringstream& file, ReflectedType const& type, ReflectedType const& parentType )
    {
        // The pass by value here is intentional!
        auto GeneratePropertyRegistrationCode = [this, &file, type] ( ReflectedProperty prop )
        {
            if ( !prop.ParseMetaData() )
            {
                LogWarning( "Invalid metadata detected for property: %s in type: %s%s", prop.m_name.c_str(), type.m_namespace.c_str(), type.m_name.c_str() );
            }

            String const templateSpecializationString = prop.m_templateArgTypeName.empty() ? String() : "<" + prop.m_templateArgTypeName + ">";

            file << "\n";
            file << "                //-------------------------------------------------------------------------\n\n";

            if ( prop.m_isDevOnly )
            {
                file << "                #if EE_DEVELOPMENT_TOOLS\n";
            }

            file << "                propertyInfo.m_ID = StringID( \"" << prop.m_name.c_str() << "\" );\n";
            file << "                propertyInfo.m_typeID = TypeSystem::TypeID( \"" << prop.m_typeName.c_str() << "\" );\n";
            file << "                propertyInfo.m_parentTypeID = " << type.m_ID.ToUint() << ";\n";
            file << "                propertyInfo.m_templateArgumentTypeID = TypeSystem::TypeID( \"" << prop.m_templateArgTypeName.c_str() << "\" );\n\n";

            // Create dev tools info
            file << "                #if EE_DEVELOPMENT_TOOLS\n";
            file << "                propertyInfo.m_friendlyName = \"" << prop.GetFriendlyName().c_str() << "\";\n";
            file << "                propertyInfo.m_category = \"" << prop.GetCategory().c_str() << "\";\n";

            String escapedDescription = prop.m_description;
            StringUtils::ReplaceAllOccurrencesInPlace( escapedDescription, "\"", "\\\"" );
            file << "                propertyInfo.m_description = \"" << escapedDescription.c_str() << "\";\n";

            if ( prop.m_isDevOnly )
            {
                file << "                propertyInfo.m_isForDevelopmentUseOnly = true;\n";
            }
            else
            {
                file << "                propertyInfo.m_isForDevelopmentUseOnly = false;\n";
            }

            if ( prop.m_isToolsReadOnly )
            {
                file << "                propertyInfo.m_isToolsReadOnly = true;\n";
            }
            else
            {
                file << "                propertyInfo.m_isToolsReadOnly = false;\n";
            }

            if ( prop.m_showInRestrictedMode )
            {
                file << "                propertyInfo.m_showInRestrictedMode = true;\n";
            }
            else
            {
                file << "                propertyInfo.m_showInRestrictedMode = false;\n";
            }

            if ( prop.m_customEditorID.IsValid() )
            {
                file << "                propertyInfo.m_customEditorID = StringID( \"" << prop.m_customEditorID.c_str() << "\" );\n";
            }
            else
            {
                file << "                propertyInfo.m_customEditorID = StringID();\n";
            }

            file << "                #endif\n\n";

            // Abstract types cannot have default values since they cannot be instantiated
            if ( type.IsAbstract() )
            {
                file << "                propertyInfo.m_pDefaultValue = nullptr;\n";
            }
            else
            {
                file << "                propertyInfo.m_pDefaultValue = &pActualDefaultInstance->" << prop.m_name.c_str() << ";\n";

                file << "                propertyInfo.m_offset = offsetof( " << type.m_namespace.c_str() << type.m_name.c_str() << ", " << prop.m_name.c_str() << " );\n";

                if ( prop.IsDynamicArrayProperty() )
                {
                    file << "                propertyInfo.m_pDefaultArrayData = pActualDefaultInstance->" << prop.m_name.c_str() << ".data();\n";
                    file << "                propertyInfo.m_arraySize = (int32_t) pActualDefaultInstance->" << prop.m_name.c_str() << ".size();\n";
                    file << "                propertyInfo.m_arrayElementSize = (int32_t) sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                    file << "                propertyInfo.m_size = sizeof( TVector<" << prop.m_typeName.c_str() << templateSpecializationString.c_str() << "> );\n";
                }
                else if ( prop.IsStaticArrayProperty() )
                {
                    file << "                propertyInfo.m_pDefaultArrayData = pActualDefaultInstance->" << prop.m_name.c_str() << ";\n";
                    file << "                propertyInfo.m_arraySize = " << prop.GetArraySize() << ";\n";
                    file << "                propertyInfo.m_arrayElementSize = (int32_t) sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                    file << "                propertyInfo.m_size = sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " ) * " << prop.GetArraySize() << ";\n";
                }
                else
                {
                    file << "                propertyInfo.m_size = sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                }

                file << "                propertyInfo.m_flags.Set( " << prop.m_flags << " );\n";
                file << "                m_properties.emplace_back( propertyInfo );\n";
                file << "                m_propertyMap.insert( TPair<StringID, int32_t>( propertyInfo.m_ID, int32_t( m_properties.size() ) - 1 ) );\n";
            }

            if ( prop.m_isDevOnly )
            {
                file << "                #endif\n";
            }

            return true;
        };

        //-------------------------------------------------------------------------

        file << "            TTypeInfo( IReflectedType const* pDefaultInstance )\n";
        file << "            {\n";

        // Create type info
        file << "                m_ID = TypeSystem::TypeID( \"" << type.m_namespace.c_str() << type.m_name.c_str() << "\" );\n";
        file << "                m_size = sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";
        file << "                m_alignment = alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";

        // Set default instance
        file << "                m_pDefaultInstance = pDefaultInstance;\n";

        // Add type metadata
        if ( type.IsAbstract() )
        {
            file << "                m_isAbstract = true;\n";
        }

        file << "\n";

        // Create dev tools info
        file << "                #if EE_DEVELOPMENT_TOOLS\n";
        file << "                m_friendlyName = \"" << type.GetFriendlyName().c_str() << "\";\n";
        file << "                m_category = \"" << type.GetCategory().c_str() << "\";\n";

        if ( type.m_isDevOnly )
        {
            file << "                m_isForDevelopmentUseOnly = true;\n";
        }

        file << "                #endif\n\n";

        // Add parent info
        file << "                // Parent types\n";
        file << "                //-------------------------------------------------------------------------\n\n";
        file << "                m_pParentTypeInfo = " << parentType.m_namespace.c_str() << parentType.m_name.c_str() << "::s_pTypeInfo;\n";

        // Add properties
        if ( type.HasProperties() )
        {
            file << "\n";
            file << "                // Register properties and type\n";
            file << "                //-------------------------------------------------------------------------\n\n";

            if ( !type.IsAbstract() )
            {
                file << "                auto pActualDefaultInstance = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( m_pDefaultInstance );\n";
            }

            file << "                PropertyInfo propertyInfo;\n";

            for ( auto& prop : type.m_properties )
            {
                GeneratePropertyRegistrationCode( prop );
            }
        }

        file << "            }\n\n";
    }
}