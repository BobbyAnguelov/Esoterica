#pragma once

#include "Applications/Reflector/Database/ReflectionDatabase.h"
#include <sstream>

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

        struct GeneratedFile
        {
            FileSystem::Path                m_path;
            String                          m_contents;
        };

    public:

        CodeGenerator( FileSystem::Path const& solutionPath, ReflectionDatabase const& database );

        inline bool HasErrors() const { return !m_errorMessage.empty(); }
        char const* GetErrorMessage() const { return m_errorMessage.c_str(); }

        inline bool HasWarnings() const { return !m_warningMessage.empty(); }
        char const* GetWarningMessage() const { return m_warningMessage.c_str(); }

        bool GenerateCodeForSolution();

        inline TVector<GeneratedFile> const& GetGeneratedFiles() const { return m_generatedFiles; }

    private:

        // Solution/Project Generation Entry Points
        bool GenerateCodeForProject( ProjectInfo const& projectInfo );
        bool GenerateSolutionTypeRegistrationFile( CompilationMode mode );
        bool GenerateProjectTypeInfoHeaderFile( ProjectInfo const& projectInfo );
        bool GenerateProjectTypeInfoSourceFile( ProjectInfo const& projectInfo );
        bool GenerateTypeInfoFileForHeader( ProjectInfo const& projectInfo, HeaderInfo const& headerInfo, TVector<ReflectedType> const& typesInHeader, FileSystem::Path const& outputPath );

        // Resources And Data files
        bool GenerateResourceRegistrationMethods( std::stringstream& outputFileStream, CompilationMode mode );
        bool GenerateDataFileRegistrationMethods( std::stringstream& outputFileStream );

        // Type Info
        void GenerateEnumTypeInfo( std::stringstream& outputFileStream, String const& exportMacro, ReflectedType const& typeInfo );
        void GenerateStructureTypeInfo( std::stringstream& file, String const& exportMacro, ReflectedType const& type, ReflectedType const& parentType );
        void GenerateComponentCodegen( std::stringstream& outputFileStream, ReflectedType const& typeInfo );

        // Structure Type Info
        void GenerateTypeInfoConstructor( std::stringstream& file, ReflectedType const& type, ReflectedType const& parentType );
        void GenerateTypeInfoCreationMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoInPlaceCreationMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoResetTypeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoLoadResourcesMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoUnloadResourcesMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoResourceLoadingStatusMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoResourceUnloadingStatusMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArrayAccessorMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArraySizeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArrayElementSizeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArraySetSizeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoArrayClearMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoAddArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoInsertArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoMoveArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoRemoveArrayElementMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoCopyProperties( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoAreAllPropertiesEqualMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoIsPropertyEqualMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoSetToDefaultValueMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoExpectedResourceTypeMethod( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoStaticTypeRegistrationMethods( std::stringstream& file, ReflectedType const& type );
        void GenerateTypeInfoGetReferencedResourceMethod( std::stringstream& file, ReflectedType const& type );

        bool LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;

    private:

        FileSystem::Path                    m_solutionPath;
        ReflectionDatabase const*           m_pDatabase;
        mutable String                      m_warningMessage;
        mutable String                      m_errorMessage;

        TVector<GeneratedFile>              m_generatedFiles;
    };
}