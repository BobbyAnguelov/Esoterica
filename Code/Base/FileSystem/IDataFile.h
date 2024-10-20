#pragma once

#include "Base/_Module/API.h"
#include "FileSystemExtension.h"
#include "Base/Encoding/FourCC.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace pugi { class xml_document; }

//-------------------------------------------------------------------------
// Data File
//-------------------------------------------------------------------------
// This is a standalone tools only data file that contains various settings you might want to share or compile into a different form
// These files are intended for tools and development use only!

namespace EE
{
    struct EE_BASE_API IDataFile : public IReflectedType
    {
        EE_REFLECT_TYPE( IDataFile );

    public:

        #if EE_DEVELOPMENT_TOOLS
        // Try to read a resource data file from disk without knowing the type
        static IDataFile* TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath );

        // Try to read a resource data file into an existing file of the expected type
        static bool TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, IDataFile* pDataFile );

        // Try to read a specific data file from disk
        template<typename T>
        static bool TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, T& outData )
        {
            static_assert( std::is_base_of<IDataFile, T>::value, "T must be a child of IDataFile" );
            return TryReadFromFile( typeRegistry, dataFilePath, (IDataFile*) &outData );
        }

        // Write a data file to disk
        // There are two modes for writing to file, overwriting the file without checking if the write is necessary (default) or checking contents and only updating the file if necessary
        static bool TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, IDataFile const* pDataFile, bool onlyWriteFileIfContentsChanged = false );
        #endif

    private:

        #if EE_DEVELOPMENT_TOOLS
        static bool TryUpgradeOnLoad( TypeSystem::TypeRegistry const& typeRegistry, pugi::xml_document& document );
        #endif

    public:

        // Get the version for this data file
        virtual int32_t GetFileVersion() const = 0;

        // Get the extension for this data file type ( without the leading '.' )
        virtual FileSystem::Extension GetExtension() const = 0;

        // Get the friendly name for this data file type
        virtual char const* GetFriendlyName() const = 0;

    protected:

        #if EE_DEVELOPMENT_TOOLS
        // Called whenever we detect a version mismatch at load but before deserialization. Allows you to upgrade the json data in-situ before progressing with the load
        // Returns true if the upgrade succeeded or had nothing to do, false if it failed!
        virtual bool UpgradeSourceData( TypeSystem::TypeRegistry const& typeRegistry, pugi::xml_document& document, int32_t versionDetected ) const { return true; }
        #endif
    };
}

//-------------------------------------------------------------------------

// Use this macro if you want to define a registered and standalone data file
// Not all data files need to be registered in this way!
// Note: The expected fourCC can only contain lowercase letters and digits
#define EE_DATA_FILE( typeName, extensionFourCC, friendlyName, fileVersion ) \
    EE_REFLECT_TYPE( typeName );\
    public: \
        constexpr static int32_t const s_fileVersion = fileVersion;\
        constexpr static uint32_t const s_extensionFourCC = extensionFourCC;\
        constexpr static char const* const s_friendlyName = #friendlyName;\
        virtual int32_t GetFileVersion() const override { return fileVersion; };\
        static inline FileSystem::Extension GetStaticExtension() { return FourCC::ToString( s_extensionFourCC ); };\
        virtual FileSystem::Extension GetExtension() const override { return FourCC::ToString( s_extensionFourCC ); };\
        virtual char const* GetFriendlyName() const override { return friendlyName; }\
    private: