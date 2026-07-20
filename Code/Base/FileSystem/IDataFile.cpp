#include "IDataFile.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Serialization/TypeSerialization.h"
#include "FileSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS

    constexpr static char const* const g_versionAttrName = "Version";

    //-------------------------------------------------------------------------

    int8_t IDataFile::TryUpgradeOnLoad( TypeSystem::TypeRegistry const& typeRegistry, Log& log, xml_document& document )
    {
        EE_ASSERT( !document.empty() );
        xml_node typeNode = document.first_child();
        EE_ASSERT( !typeNode.empty() );

        // Get data file type
        //-------------------------------------------------------------------------

        xml_attribute typeAttr = typeNode.attribute( Serialization::g_typeIDAttrName );
        if ( typeAttr.empty() )
        {
            return (int8_t) log.LogError( "Trying to upgrade a malformed datafile, missing typeID" );
        }

        TypeSystem::TypeID const dataFileTypeID( typeAttr.as_string() );
        TypeSystem::TypeInfo const* pTypeInfo = typeRegistry.GetTypeInfo( dataFileTypeID );
        if ( pTypeInfo == nullptr )
        {
            return (int8_t) log.LogError( "Trying to upgrade a non-datafile type: %s", dataFileTypeID.c_str() );
        }

        IDataFile const* pDefaultDataFileInstance = TryCast<IDataFile>( pTypeInfo->m_pDefaultInstance );
        if ( pDefaultDataFileInstance == nullptr )
        {
            return (int8_t) log.LogError( "Trying to upgrade a non-datafile type: %s", dataFileTypeID.c_str() );
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
            if ( pDefaultDataFileInstance->UpgradeSourceData( typeRegistry, document, detectedFileVersion ) )
            {
                return 2;
            }
            else
            {
                return (int8_t) false;
            }
        }

        //-------------------------------------------------------------------------

        return (int8_t) true;
    }

    //-------------------------------------------------------------------------

    IDataFile::ReadResult IDataFile::ReadXMLFile( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& filePath, String& xmlStringBuffer, xml_document& doc, Log& log, uint64_t* pOutFileHash )
    {
        EE_ASSERT( filePath.IsFilePath() );

        ReadResult result;

        // Read data
        //-------------------------------------------------------------------------

        if ( !FileSystem::ReadTextFile( filePath, xmlStringBuffer ) )
        {
            result.m_succeeded = log.LogError( "Failed to read data file from disk: %s", filePath.c_str() );
            return result;
        }

        if ( xmlStringBuffer.empty() )
        {
            result.m_succeeded = log.LogError( "Failed to read empty data file: %s", filePath.c_str() );
            return result;
        }

        // Calculate the file hash for the read file
        if ( pOutFileHash != nullptr )
        {
            *pOutFileHash = Hash::GetHash64( xmlStringBuffer );
        }

        if ( !Serialization::ReadXmlFromBufferInPlace( xmlStringBuffer.data(), xmlStringBuffer.size(), doc ) )
        {
            result.m_succeeded = log.LogError( "Failed to read data file: %s", filePath.c_str() );
            return result;
        }

        // Upgrade file in place
        //-------------------------------------------------------------------------

        int8_t const upgradeResult = IDataFile::TryUpgradeOnLoad( typeRegistry, log, doc );
        if ( upgradeResult == 0 )
        {
            result.m_succeeded = log.LogError( "Failed to upgrade data: %s", filePath.c_str() );
            return result;
        }

        result.m_succeeded = true;
        result.m_wasUpgraded = upgradeResult == 2;
        return result;
    }

    IDataFile::ReadResult IDataFile::TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, uint64_t* pOutFileHash )
    {
        EE_ASSERT( filePath.IsFilePath() );

        // Read XML data
        //-------------------------------------------------------------------------

        String xmlStringBuffer;
        xml_document doc;
        ReadResult result = ReadXMLFile( typeRegistry, filePath, xmlStringBuffer, doc, log, pOutFileHash );
        if( !result.m_succeeded )
        {
            return result;
        }

        // Read the type and any custom data
        //-------------------------------------------------------------------------

        IReflectedType* pTypeInstance = Serialization::TryCreateAndReadType( typeRegistry, log, doc );
        if ( pTypeInstance == nullptr )
        {
            result.m_succeeded = log.LogError( "Failed to deserialize data file: %s", filePath.c_str() );
            return result;
        }

        result.m_pDataFile = TryCast<IDataFile>( pTypeInstance );
        if ( result.m_pDataFile == nullptr )
        {
            result.m_succeeded = log.LogError( "Invalid non-datafile type deserialized: %s", pTypeInstance->GetTypeID().c_str() );
            EE::Delete( pTypeInstance );
            return result;
        }

        if ( result.m_pDataFile->SupportsCustomData() )
        {
            xml_node customDataNode = doc.child( g_customDataNodeName );
            if ( !customDataNode.empty() )
            {
                if ( !result.m_pDataFile->ReadCustomData( typeRegistry, log, customDataNode ) )
                {
                    result.m_succeeded = log.LogError( "Failed to read custom data section from data file: %s", filePath.c_str() );
                    EE::Delete( pTypeInstance );
                    return result;
                }
            }
        }

        //-------------------------------------------------------------------------

        result.m_pDataFile->PostLoad( typeRegistry );
        result.m_succeeded = true;
        return result;
    }

    IDataFile::ReadResult IDataFile::TryReadFromFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, IDataFile* pDataFile, uint64_t* pOutFileHash )
    {
        EE_ASSERT( pDataFile != nullptr );
        EE_ASSERT( filePath.IsFilePath() );

        // Read XML data
        //-------------------------------------------------------------------------

        String xmlStringBuffer;
        xml_document doc;
        ReadResult result = ReadXMLFile( typeRegistry, filePath, xmlStringBuffer, doc, log, pOutFileHash );
        if ( !result.m_succeeded )
        {
            return result;
        }

        // Read the type and any custom data
        //-------------------------------------------------------------------------

        if ( !Serialization::ReadType( typeRegistry, log, doc, pDataFile ) )
        {
            result.m_succeeded = log.LogError( "Failed to read type from data file: %s", filePath.c_str() );
            return result;
        }

        if ( pDataFile->SupportsCustomData() )
        {
            xml_node customDataNode = doc.child( g_customDataNodeName );
            if ( !customDataNode.empty() )
            {
                if ( !pDataFile->ReadCustomData( typeRegistry, log, customDataNode ) )
                {
                    result.m_succeeded = log.LogError( "Failed to read custom data section from data file: %s", filePath.c_str() );
                    return result;
                }
            }
        }

        pDataFile->PostLoad( typeRegistry );
        result.m_succeeded = true;
        return result;
    }

    bool IDataFile::TryWriteToFile( TypeSystem::TypeRegistry const& typeRegistry, Log& log, FileSystem::Path const& filePath, IDataFile const* pDataFile, bool onlyWriteFileIfContentsChanged )
    {
        EE_ASSERT( filePath.IsFilePath() );

        //-------------------------------------------------------------------------

        xml_document doc;
        Serialization::WriteType( typeRegistry, pDataFile, doc );

        // Write version number
        //-------------------------------------------------------------------------

        doc.first_child().append_attribute( g_versionAttrName ).set_value( pDataFile->GetFileVersion() );

        // Write custom data
        //-------------------------------------------------------------------------

        if ( pDataFile->SupportsCustomData() )
        {
            xml_node customDataNode = doc.append_child( g_customDataNodeName );
            if ( !pDataFile->WriteCustomData( typeRegistry, log, customDataNode ) )
            {
                return log.LogError( "Failed to write custom data section for data file: %s", filePath.c_str() );
            }
        }

        // Write to file
        //-------------------------------------------------------------------------

        bool const writeSucceeded = Serialization::WriteXmlToFile( doc, filePath, onlyWriteFileIfContentsChanged );
        if ( !writeSucceeded )
        {
            return log.LogError( "Failed to write data file: %s", filePath.c_str() );
        }

        return writeSucceeded;
    }
    #endif
}