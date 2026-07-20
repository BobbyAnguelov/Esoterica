#pragma once

#include "TypeReflection_ReflectionDatabase.h"
#include "Applications/Reflector/ReflectedCodeGenerator.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class TypeInfoCodeGenerator : public CodeGenerator
    {
        enum class CompilationMode
        {
            Runtime,
            Tools
        };

    public:

        TypeInfoCodeGenerator( Reflector& reflector, ReflectionDatabase const& database );

        virtual bool GenerateCodeForSolution() override;

    private:

        // Solution/Project Generation Entry Points
        bool GenerateCodeForProject( ReflectedProject const& project );
        bool GenerateSolutionTypeRegistrationFile( CompilationMode mode );
        bool GenerateProjectTypeInfoHeaderFile( ReflectedProject const& project );
        bool GenerateProjectTypeInfoSourceFile( ReflectedProject const& project );
        bool GenerateTypeInfoFileForHeader( ReflectedProject const& project, ReflectedHeader const& header, TVector<ReflectedType> const& typesInHeader, FileSystem::Path const& outputPath );

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

        // Generate an XML version of all the reflected type info
        void GenerateXML();

    private:

        ReflectionDatabase const*           m_pDatabase;
    };
}