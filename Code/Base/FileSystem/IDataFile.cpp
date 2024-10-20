#include "IDataFile.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS

    constexpr static char const* const g_versionAttrName = "Version";

    //-------------------------------------------------------------------------

    bool IDataFile::TryUpgradeOnLoad( TypeSystem::TypeRegistry const& typeRegistry, xml_document& document )
    {
        EE_ASSERT( !document.empty() );
        xml_node typeNode = document.first_child();
        EE_ASSERT( !typeNode.empty() );

        // Get data file type
        //-------------------------------------------------------------------------

        xml_attribute typeAttr = typeNode.attribute( Serialization::g_typeIDAttrName );
        if ( typeAttr.empty() )
        {
            EE_LOG_ERROR( "Data File", "Upgrade", "Trying to upgrade a malformed datafile, missing typeID" );
            return false;
        }

        TypeSystem::TypeID const dataFileTypeID( typeAttr.as_string() );
        TypeSystem::TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( dataFileTypeID );
        if ( pTypeInfo == nullptr )
        {
            EE_LOG_ERROR( "Data File", "Upgrade", "Trying to upgrade a non-datafile type: %s", dataFileTypeID.c_str() );
            return false;
        }

        IDataFile const* pDefaultDataFileInstance = TryCast<IDataFile>( pTypeInfo->m_pDefaultInstance );
        if ( pDefaultDataFileInstance == nullptr )
        {
            EE_LOG_ERROR( "Data File", "Upgrade", "Trying to upgrade a non-datafile type: %s", dataFileTypeID.c_str() );
            return false;
        }

        int32_t const expectedFileVersion = pDefaultDataFileInstance->GetFileVersion();

        // Try get version in file
        //-------------------------------------------------------------------------

        int32_t detectedFileVersion = -1;
        xml_attribute versionAttr = typeNode.attribute( g_versionAttrName );
        if ( !versionAttr.empty() )
        {
            detectedFileVersion = versionAttr.as_int();
        }

        // Upgrade
        //-------------------------------------------------------------------------

        if ( detectedFileVersion != expectedFileVersion )
        {
            return pDefaultDataFileInstance->UpgradeSourceData( typeRegistry, document, detectedFileVersion );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    //-------------------------------------------------------------------------

    IDataFile* IDataFile::TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath )
    {
        EE_ASSERT( dataFilePath.IsFilePath() );

        //-------------------------------------------------------------------------

        xml_document doc;
        if ( !Serialization::ReadXmlFromFile( dataFilePath, doc ) )
        {
            EE_LOG_ERROR( "Data File", "Read", "Failed to read data file: %s", dataFilePath.c_str() );
            return nullptr;
        }

        if ( !TryUpgradeOnLoad( typeRegistry, doc ) )
        {
            EE_LOG_ERROR( "Data File", "Read", "Failed to upgrade data: %s", dataFilePath.c_str() );
            return nullptr;
        }

        IReflectedType* pTypeInstance = Serialization::TryCreateAndReadType( typeRegistry, doc );
        if ( pTypeInstance == nullptr )
        {
            EE_LOG_ERROR( "Data File", "Read", "Failed to deserialize data file: %s", dataFilePath.c_str() );
            return nullptr;
        }

        IDataFile* pDataFile = TryCast<IDataFile>( pTypeInstance );
        if ( pDataFile == nullptr )
        {
            EE_LOG_ERROR( "Data File", "Read", "Invalid non-datafile type deserialized: %s", pTypeInstance->GetTypeID().c_str() );
            EE::Delete( pTypeInstance );
            return nullptr;
        }

        return pDataFile;
    }

    bool IDataFile::TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, IDataFile* pDataFile )
    {
        EE_ASSERT( pDataFile != nullptr );
        EE_ASSERT( dataFilePath.IsFilePath() );

        xml_document doc;
        if ( !Serialization::ReadXmlFromFile( dataFilePath, doc ) )
        {
            EE_LOG_ERROR( "Data File", "Read", "Failed to read data file: %s", dataFilePath.c_str() );
            return false;
        }

        if ( !TryUpgradeOnLoad( typeRegistry, doc ) )
        {
            EE_LOG_ERROR( "Data File", "Read", "Failed to upgrade data: %s", dataFilePath.c_str() );
            return false;
        }

        return Serialization::ReadType( typeRegistry, doc, pDataFile );
    }

    bool IDataFile::TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& dataFilePath, IDataFile const* pDataFile, bool onlyWriteFileIfContentsChanged )
    {
        EE_ASSERT( dataFilePath.IsFilePath() );

        //-------------------------------------------------------------------------

        xml_document doc;
        Serialization::WriteType( typeRegistry, pDataFile, doc );

        // Write version number
        //-------------------------------------------------------------------------

        doc.first_child().append_attribute( g_versionAttrName ).set_value( pDataFile->GetFileVersion() );

        // Write to file
        //-------------------------------------------------------------------------

        bool const writeSucceeded = Serialization::WriteXmlToFile( doc, dataFilePath, onlyWriteFileIfContentsChanged );
        if ( !writeSucceeded )
        {
            EE_LOG_ERROR( "Data File", "Write", "Failed to write data file: %s", dataFilePath.c_str() );
        }

        return writeSucceeded;
    }
    #endif
}