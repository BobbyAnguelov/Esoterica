#pragma once

#include "Base/_Module/API.h"
#include "FileSystemExtension.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace pugi { class xml_document; class xml_node;}

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

        constexpr static char const* const g_customDataNodeName = "CustomData";

    public:

        #if EE_DEVELOPMENT_TOOLS
        struct ReadResult
        {
            bool        m_succeeded = false;
            bool        m_wasUpgraded = false;
            IDataFile*  m_pDataFile = nullptr; // Only set if you didn't pass in a type for the read!
        };

        // Try to read a resource data file from disk without knowing the type
        // Optionally returns the hash of the file from disk
        static ReadResult TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, uint64_t* pOutFileHash = nullptr );

        // Try to read a resource data file into an existing file of the expected type
        // Optionally returns the hash of the file from disk
        static ReadResult TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, IDataFile* pDataFile, uint64_t* pOutFileHash = nullptr );

        // Try to read a specific data file from disk
        // Optionally returns the hash of the file from disk
        template<typename T>
        static ReadResult TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, T& outData, uint64_t* pOutFileHash = nullptr )
        {
            static_assert( std::is_base_of<IDataFile, T>::value, "T must be a child of IDataFile" );
            return TryReadFromFile( typeRegistry, log, filePath, (IDataFile*) &outData );
        }

        // Write a data file to disk
        // There are two modes for writing to file, overwriting the file without checking if the write is necessary (default) or checking contents and only updating the file if necessary
        static bool TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, IDataFile const* pDataFile, bool onlyWriteFileIfContentsChanged = false );
        #endif

    private:

        #if EE_DEVELOPMENT_TOOLS
        static ReadResult ReadXMLFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, String& xmlStringBuffer, pugi::xml_document& document, Log& log, uint64_t* pOutFileHash );
        static int8_t TryUpgradeOnLoad( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_document& document );
        #endif

    public:

        // Get the version for this data file
        virtual int32_t GetFileVersion() const = 0;

        // Get the extension for this data file type ( without the leading '.' )
        virtual FileSystem::Extension GetExtension() const = 0;

        // Get the friendly name for this data file type
        virtual char const* GetFriendlyName() const = 0;

    protected:

        // Override this to enable custom data support
        virtual bool SupportsCustomData() const { return false; }

        // Override this to write custom xml data into a data file
        virtual bool WriteCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode ) const { return false; };

        // Override this to read any custom xml data found in a data file
        virtual bool ReadCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode ) { return true; };

        #if EE_DEVELOPMENT_TOOLS
        // Called whenever we detect a version mismatch at load but before deserialization. Allows you to upgrade the xml data in-situ before progressing with the load
        // Returns true if the upgrade succeeded or had nothing to do, false if it failed!
        virtual bool UpgradeSourceData( TypeSystem::TypeRegistry const& typeRegistry, pugi::xml_document& document, int32_t versionDetected ) const { return true; }

        // Called straight after loading a file allowing us to perform any basic cleanup/sanitization
        virtual void PostLoad( TypeSystem::TypeRegistry const& typeRegistry ) {}
        #endif
    };
}

//-------------------------------------------------------------------------

// Use this macro if you want to define a registered and standalone data file
// Not all data files need to be registered in this way!
// Note: The expected fourCC can only contain lowercase letters and digits
#define EE_DATA_FILE( typeName, extension, friendlyName, fileVersion ) \
    EE_REFLECT_TYPE( typeName );\
    public: \
        static_assert( StringUtils::IsLowercaseAlphaNumeric_ConstEval( extension ), "Only lowercase alphanumeric characters allowed in data file extensions" );\
        static_assert( StringUtils::GetStringLiteralLength_ConstEval( extension ) > 0 && StringUtils::GetStringLiteralLength_ConstEval( extension ) <= 9, "Data file extension must be 8 or less characters" );\
        constexpr static int32_t const s_fileVersion = fileVersion;\
        constexpr static char const* const s_friendlyName = #friendlyName;\
        virtual int32_t GetFileVersion() const override { return fileVersion; };\
        static inline FileSystem::Extension GetStaticExtension() { return extension; };\
        virtual FileSystem::Extension GetExtension() const override { return extension; };\
        virtual char const* GetFriendlyName() const override { return friendlyName; }\
    private: