#pragma once

#include "Applications/Reflector/Database/ReflectionDatabase.h"
#include <sstream>

//-------------------------------------------------------------------------

using namespace EE::TypeSystem::Reflection;
using namespace EE::TypeSystem;

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class CodeGenerator
    {
        enum class CompilationMode
        {
            Runtime,
            Tools
        };

    public:

        CodeGenerator( ReflectionDatabase const& database ) : m_pDatabase( &database ) {}
        ~CodeGenerator() {}

        inline bool HasErrors() const { return !m_errorMessage.empty(); }
        char const* GetErrorMessage() const { return m_errorMessage.c_str(); }

        inline bool HasWarnings() const { return !m_warningMessage.empty(); }
        char const* GetWarningMessage() const { return m_warningMessage.c_str(); }

        bool GenerateCodeForSolution( SolutionInfo const& solutionInfo );

    private:

        // Solution/Project Generation Entry Points
        bool GenerateCodeForProject( ProjectInfo const& projectInfo );
        bool GenerateSolutionTypeRegistrationFile( SolutionInfo const& solutionInfo, CompilationMode mode );
        bool GenerateProjectTypeInfoFile( ProjectInfo const& projectInfo, TVector<FileSystem::Path> const& generatedHeaders );
        bool GenerateProjectCodeGenFile( ProjectInfo const& projectInfo, TVector<FileSystem::Path> const& generatedHeaders );
        bool GenerateTypeInfoFileForHeader( ProjectInfo const& projectInfo, HeaderInfo const& headerInfo, TVector<ReflectedType> const& typesInHeader, FileSystem::Path const& outputPath );
        bool GenerateCodeGenFileForHeader( ProjectInfo const& projectInfo, HeaderInfo const& headerInfo, TVector<ReflectedType> const& typesInHeader, FileSystem::Path const& outputPath );

        // Resources
        bool GenerateResourceRegistrationMethods( std::stringstream& outputFileStream, SolutionInfo const& solutionInfo );

        // Type Info
        void GenerateEnumTypeInfo( std::stringstream& outputFileStream, String const& exportMacro, ReflectedType const& typeInfo );
        void GenerateStructureTypeInfo( std::stringstream& file, String const& exportMacro, ReflectedType const& type, ReflectedType const& parentType );

        // Structure Type Info
        void GenerateTypeInfoConstructor( std::stringstream& file, ReflectedType const& type, ReflectedType const& parentType );
        void GenerateTypeInfoCreationMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoInPlaceCreationMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoLoadResourcesMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoUnloadResourcesMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoResourceLoadingStatusMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoResourceUnloadingStatusMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArrayAccessorMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArraySizeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArrayElementSizeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArrayClearMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoAddArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoInsertArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoMoveArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoRemoveArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoAreAllPropertiesEqualMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoIsPropertyEqualMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoSetToDefaultValueMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoExpectedResourceTypeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoStaticTypeRegistrationMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoStaticTypeUnregistrationMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoGetReferencedResourceMethod( std::stringstream& file, ReflectedType const& type );

        // Utils
        static bool SaveStreamToFile( FileSystem::Path const& filePath, std::stringstream& stream );

        bool LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;

    private:

        ReflectionDatabase const*           m_pDatabase;
        mutable String                      m_warningMessage;
        mutable String                      m_errorMessage;
    };
}