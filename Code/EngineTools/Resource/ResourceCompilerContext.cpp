#include "ResourceCompilerContext.h"
#include "ResourceDescriptor.h"
#include "ResourceCompilerRegistry.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    CompileDependencyResourceInfo::~CompileDependencyResourceInfo()
    {
        EE_ASSERT( m_resourceDependencies.empty() );
    }

    void CompileDependencyResourceInfo::Clear()
    {
        m_ID.Clear();
        m_pParent = nullptr;
        m_compiledResourceRecord = CompiledResourceRecord();
        m_sourcePath.Clear();
        m_sourceFileHash = 0;
        m_sourceFileExists = false;

        m_outputPath.Clear();
        m_additionalOutputPath.Clear();
        m_outputFileExists = false;
        m_additionalOutputFileExists = false;

        m_pDescriptor = nullptr;

        //-------------------------------------------------------------------------

        for ( CompileDependencyResourceInfo* pResInfo : m_resourceDependencies )
        {
            pResInfo->Clear();
            EE::Delete( pResInfo );
        }
        m_resourceDependencies.clear();

        //-------------------------------------------------------------------------

        m_dataDependencies.clear();
    }

    uint64_t CompileDependencyResourceInfo::CalculateCombinedHash()
    {
        m_combinedHash = ( m_customSourceHash != 0 ) ? m_customSourceHash : m_sourceFileHash;

        for ( CompileDependencyResourceInfo* pResInfo : m_resourceDependencies )
        {
            uint64_t const childCombinedHash = pResInfo->CalculateCombinedHash();
            m_combinedHash += childCombinedHash;
        }

        for ( CompileDependencyDataInfo const& rawDataInfo : m_dataDependencies )
        {
            m_combinedHash += rawDataInfo.m_sourceFileHash;
        }

        return m_combinedHash;
    }

    void CompileDependencyResourceInfo::GetResourceInfo( DataPath const& path, TInlineVector<CompileDependencyResourceInfo*, 2>& outInfos )
    {
        if ( m_ID == path )
        {
            outInfos.emplace_back( this );

            // We do not allow circular dependencies so we cannot have reference to ourselves in our dependencies
            return;
        }

        //-------------------------------------------------------------------------

        for ( CompileDependencyResourceInfo* pDependency : m_resourceDependencies )
        {
            pDependency->GetResourceInfo( path, outInfos );
        }
    }

    void CompileDependencyResourceInfo::GetDataInfo( DataPath const& path, TInlineVector<CompileDependencyDataInfo*, 2>& outInfos )
    {
        for ( CompileDependencyDataInfo& dataDependency : m_dataDependencies )
        {
            outInfos.emplace_back( &dataDependency );
        }

        for ( CompileDependencyResourceInfo* pResourceDependency : m_resourceDependencies )
        {
            pResourceDependency->GetDataInfo( path, outInfos );
        }
    }

    void CompileDependencyResourceInfo::GetAllCompileDependencyPaths( TVector<DataPath>& outDependencies ) const
    {
        for ( CompileDependencyDataInfo const& dataDependency : m_dataDependencies )
        {
            VectorEmplaceBackUnique( outDependencies, dataDependency.m_dataPath );
        }

        for ( CompileDependencyResourceInfo* pResourceDependency : m_resourceDependencies )
        {
            VectorEmplaceBackUnique( outDependencies, pResourceDependency->m_ID.GetDataPath() );
        }
    }

    //-------------------------------------------------------------------------

    CompileContext::~CompileContext()
    {
        if ( m_pResourceToCompile != nullptr )
        {
            m_pResourceToCompile->Clear();
            EE::Delete( m_pResourceToCompile );
        }

        for ( auto& iter : m_loadedDescriptorLUT )
        {
            EE::Delete( iter.second );
        }
        m_loadedDescriptorLUT.clear();

        for ( auto& iter : m_loadedDataFileLUT )
        {
            EE::Delete( iter.second );
        }
        m_loadedDataFileLUT.clear();

        for ( auto& iter : m_loadedRawDataLUT )
        {
            EE::Delete( iter.second );
        }
        m_loadedRawDataLUT.clear();

        m_uniqueCompileDependencies.clear();
        m_loadedDescriptorLUT.clear();
    }

    bool CompileContext::HasValidArguments() const
    {
        if ( !m_resourceID.IsValid() )
        {
            return false;
        }

        if ( !m_sourceResourceDirectoryPath.IsValid() || !m_compiledResourceDirectoryPath.IsValid() )
        {
            return false;
        }

        return true;
    }

    CompilationResult CompileContext::LogError( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        if ( m_isStandaloneCompile )
        {
            SystemLog::AddEntryVarArgs( Severity::Error, "Resource", m_compilerName.c_str(), __FILE__, __LINE__, pFormat, args );
        }
        m_log.LogError( pFormat, args );
        va_end( args );

        return CompilationResult::Failure;
    }

    void CompileContext::LogWarning( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        if ( m_isStandaloneCompile )
        {
            SystemLog::AddEntryVarArgs( Severity::Warning, "Resource", m_compilerName.c_str(), __FILE__, __LINE__, pFormat, args );
        }
        m_log.LogWarning( pFormat, args );
        va_end( args );
    }

    void CompileContext::LogMessage( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        if ( m_isStandaloneCompile )
        {
            SystemLog::AddEntryVarArgs( Severity::Info, "Resource", m_compilerName.c_str(), __FILE__, __LINE__, pFormat, args );
        }
        m_log.LogInfo( pFormat, args );
        va_end( args );
    }

    void CompileContext::AppendLogEntries( Log const& log )
    {
        for ( auto const& entry : log.GetLogEntries() )
        {
            switch ( entry.m_severity )
            {
                case Severity::Error:
                case Severity::FatalError:
                {
                    LogError( entry.m_message.c_str() );
                }
                break;

                case Severity::Warning:
                {
                    LogWarning( entry.m_message.c_str() );
                }
                break;

                case Severity::Info:
                {
                    LogMessage( entry.m_message.c_str() );
                }
                break;
            }
        }
    }

    ResourceDescriptor* CompileContext::TryLoadResourceDescriptor( ResourceID const& ID, String& errorMessage, uint64_t& outFileHash )
    {
        EE_ASSERT( ID.IsValid() );

        auto iter = m_loadedDescriptorLUT.find( ID );
        if ( iter != m_loadedDescriptorLUT.end() )
        {
            return iter->second;
        }

        //-------------------------------------------------------------------------

        FileSystem::Path const descriptorFilePath = ID.GetFileSystemPath( m_sourceResourceDirectoryPath );
        if ( !descriptorFilePath.Exists() )
        {
            errorMessage = String( String::CtorSprintf(), "Descriptor file doesnt exist: %s", descriptorFilePath.c_str() );
            return nullptr;
        }

        Log log;
        ResourceDescriptor* pDescriptor = ResourceDescriptor::TryReadFromFile( m_typeRegistry, log, descriptorFilePath, &outFileHash );
        if ( pDescriptor == nullptr )
        {
            errorMessage = String( String::CtorSprintf(), "Failed to read descriptor: %s", descriptorFilePath.c_str() );
        }

        AppendLogEntries( log );

        m_loadedDescriptorLUT.insert_or_assign( ID, pDescriptor );
        return pDescriptor;
    }

    IDataFile* CompileContext::TryLoadDataFile( DataPath const& dataPath, String& errorMessage, uint64_t& outFileHash )
    {
        EE_ASSERT( dataPath.IsValid() );

        auto iter = m_loadedDataFileLUT.find( dataPath );
        if ( iter != m_loadedDataFileLUT.end() )
        {
            return iter->second;
        }

        //-------------------------------------------------------------------------

        FileSystem::Path const dataFilePath = dataPath.GetFileSystemPath( m_sourceResourceDirectoryPath );
        if ( !dataFilePath.Exists() )
        {
            errorMessage = String( String::CtorSprintf(), "Data file doesnt exist: %s", dataFilePath.c_str() );
            return nullptr;
        }

        Log log;
        IDataFile::ReadResult result = IDataFile::TryReadFromFile( m_typeRegistry, log, dataFilePath, &outFileHash );
        if ( result.m_pDataFile == nullptr )
        {
            errorMessage = String( String::CtorSprintf(), "Failed to read data file: %s", dataPath.c_str() );
            return nullptr;
        }

        AppendLogEntries( log );

        m_loadedDataFileLUT.insert_or_assign( dataPath, result.m_pDataFile );
        return result.m_pDataFile;
    }

    Blob* CompileContext::TryLoadRawData( DataPath const& dataPath, String& errorMessage, uint64_t& outFileHash )
    {
        EE_ASSERT( dataPath.IsValid() );

        auto iter = m_loadedRawDataLUT.find( dataPath );
        if ( iter != m_loadedRawDataLUT.end() )
        {
            return iter->second;
        }

        //-------------------------------------------------------------------------

        FileSystem::Path const dataFilePath = dataPath.GetFileSystemPath( m_sourceResourceDirectoryPath );
        if ( !dataFilePath.Exists() )
        {
            errorMessage = String( String::CtorSprintf(), "Raw data file doesnt exist: %s", dataFilePath.c_str() );
            return nullptr;
        }

        Blob* pRawData = EE::New<Blob>();
        if ( !FileSystem::ReadBinaryFile( dataFilePath, *pRawData ) )
        {
            errorMessage = String( String::CtorSprintf(), "Failed to read raw data: %s", dataPath.c_str() );
            EE::Delete( pRawData );
            return nullptr;
        }

        outFileHash = Hash::GetHash64( *pRawData );

        m_loadedRawDataLUT.insert_or_assign( dataPath, pRawData );
        return pRawData;
    }

    //-------------------------------------------------------------------------

    CompileDependencyResourceInfo* CompileContext::CreateResourceInfo( CompileDependencyResourceInfo* pParentInfo, ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        ResourceTypeID const resourceTypeID = resourceID.GetResourceTypeID();

        // Basic resource info
        //-------------------------------------------------------------------------

        CompileDependencyResourceInfo* pCreatedInfo = EE::New<CompileDependencyResourceInfo>();
        pCreatedInfo->m_ID = resourceID;
        pCreatedInfo->m_pParent = pParentInfo;

        // Compilable Resources
        //-------------------------------------------------------------------------

        Compiler const* pCompiler = m_compilerRegistry.GetCompilerForResourceType( resourceTypeID );
        bool const isCompilableResource = pCompiler != nullptr;
        if ( isCompilableResource )
        {
            pCreatedInfo->m_compiledResourceVersion = pCompiler->GetVersionForResourceType( resourceTypeID );

            pCreatedInfo->m_sourcePath = resourceID.GetFileSystemPath( m_sourceResourceDirectoryPath );
            pCreatedInfo->m_sourceFileExists = FileSystem::Exists( pCreatedInfo->m_sourcePath );
            pCreatedInfo->m_sourceFileHash = pCreatedInfo->m_sourceFileExists ? FileSystem::GetFileModifiedTime( pCreatedInfo->m_sourcePath ) : 0;

            pCreatedInfo->m_outputPath = resourceID.GetCompiledFileSystemPath( m_compiledResourceDirectoryPath );
            pCreatedInfo->m_outputFileExists = FileSystem::Exists( pCreatedInfo->m_outputPath );

            if ( pCompiler->WillGenerateAdditionalDataFile( resourceTypeID ) )
            {
                pCreatedInfo->m_additionalOutputPath = IResource::GetAdditionalDataFilePath( pCreatedInfo->m_outputPath );
                pCreatedInfo->m_additionalOutputFileExists = FileSystem::Exists( pCreatedInfo->m_additionalOutputPath );
            }
        }
        else
        {
            pCreatedInfo->m_sourcePath = resourceID.GetFileSystemPath( m_sourceResourceDirectoryPath );
            pCreatedInfo->m_sourceFileExists = FileSystem::Exists( pCreatedInfo->m_sourcePath );
            pCreatedInfo->m_sourceFileHash = pCreatedInfo->m_sourceFileExists ? FileSystem::GetFileModifiedTime( pCreatedInfo->m_sourcePath ) : 0;
        }

        // Compiled resource record
        //-------------------------------------------------------------------------

        m_compiledResourceDB.GetRecord( pCreatedInfo->m_ID, pCreatedInfo->m_compiledResourceRecord );

        // Descriptor
        //-------------------------------------------------------------------------

        String descriptorLoadError;
        pCreatedInfo->m_pDescriptor = TryLoadResourceDescriptor( resourceID.GetDataPath(), descriptorLoadError, pCreatedInfo->m_sourceFileHash );
        if ( pCreatedInfo->m_pDescriptor == nullptr )
        {
            pCreatedInfo->m_error = String( String::CtorSprintf(), "Failed to load descriptor: %s, Reason: %s", resourceID.c_str(), descriptorLoadError.c_str() );
            return pCreatedInfo;
        }

        // Compile Dependencies
        //-------------------------------------------------------------------------

        String subResourceName;
        if ( resourceID.IsSubResourceID() )
        {
            subResourceName = resourceID.GetSubResourceName();
        }

        // Get the compile dependencies for this descriptor
        TVector<Resource::CompileDependency> dependencies;
        pCreatedInfo->m_pDescriptor->GetCompileDependencies( m_typeRegistry, m_sourceResourceDirectoryPath, subResourceName, dependencies );

        // Get any additional dependencies
        pCompiler->GetAdditionalCompileDependencies( m_typeRegistry, m_sourceResourceDirectoryPath, resourceID, pCreatedInfo->m_pDescriptor, dependencies );

        for ( Resource::CompileDependency const& compileDependency : dependencies )
        {
            // Skip invalid dependencies
            if ( !compileDependency.IsValid() )
            {
                pCreatedInfo->m_error = String( String::CtorSprintf(), "Invalid compile dependency encountered for descriptor: ", resourceID.c_str() );
                return pCreatedInfo;
            }

            if ( compileDependency.m_isResource )
            {
                ResourceID const compileDependencyResourceID( compileDependency );
                bool const isRegisteredResource = m_typeRegistry.IsRegisteredResourceType( compileDependencyResourceID.GetResourceTypeID() );
                bool const hasCompilerForResourceType = m_compilerRegistry.HasCompilerForResourceType( compileDependencyResourceID.GetResourceTypeID() );

                if ( isRegisteredResource && hasCompilerForResourceType )
                {
                    // Skip resources already in the tree!
                    if ( VectorContains( m_uniqueCompileDependencies, compileDependencyResourceID ) )
                    {
                        continue;
                    }

                    // Check for circular references
                    //-------------------------------------------------------------------------

                    auto pNodeToCheck = pCreatedInfo;
                    while ( pNodeToCheck != nullptr )
                    {
                        if ( pNodeToCheck->m_ID == compileDependencyResourceID )
                        {
                            pCreatedInfo->m_error = String( String::CtorSprintf(), "Circular dependency detected for resource: %s", compileDependencyResourceID.c_str() );
                            return pCreatedInfo;
                        }

                        pNodeToCheck = pNodeToCheck->m_pParent;
                    }

                    // Create dependency
                    //-------------------------------------------------------------------------

                    auto pChildDependencyNode = CreateResourceInfo( pCreatedInfo, compileDependencyResourceID );
                    pCreatedInfo->m_resourceDependencies.emplace_back( pChildDependencyNode );
                    m_uniqueCompileDependencies.emplace_back( compileDependency );

                    if ( pChildDependencyNode->HasError() )
                    {
                        pCreatedInfo->m_error = String( String::CtorSprintf(), "Failed to create info for compile dependency: %s, Reason: %s", compileDependencyResourceID.c_str(), pChildDependencyNode->m_error.c_str() );
                        return pCreatedInfo;
                    }
                }
            }
            else // Data files
            {
                CompileDependencyDataInfo const& createdRawDataInfo = CreateDataInfo( pCreatedInfo, compileDependency );
                if ( createdRawDataInfo.HasError() )
                {
                    pCreatedInfo->m_error = String( String::CtorSprintf(), "Failed to create info for compile dependency: %s, Reason: %s", compileDependency.m_path.c_str(), createdRawDataInfo.m_error.c_str() );
                    return pCreatedInfo;
                }
            }
        }

        return pCreatedInfo;
    }

    CompileDependencyDataInfo const& CompileContext::CreateDataInfo( CompileDependencyResourceInfo* pParentInfo, CompileDependency const& cd )
    {
        EE_ASSERT( !cd.m_isResource );

        CompileDependencyDataInfo& rawDataInfo = pParentInfo->m_dataDependencies.emplace_back();
        rawDataInfo.m_dataPath = cd.m_path;
        rawDataInfo.m_sourcePath = cd.GetFileSystemPath( m_sourceResourceDirectoryPath );
        rawDataInfo.m_sourceFileExists = FileSystem::Exists( rawDataInfo.m_sourcePath );
        rawDataInfo.m_sourceFileHash = rawDataInfo.m_sourceFileExists ? FileSystem::GetFileModifiedTime( rawDataInfo.m_sourcePath ) : 0;

        if ( rawDataInfo.m_sourceFileExists )
        {
            // If this is a known data file, try to load it
            DataFileExtension const fileExt( rawDataInfo.m_sourcePath.GetExtension() );
            if ( m_typeRegistry.IsRegisteredDataFileType( fileExt ) )
            {
                String dataFileLoadError;
                rawDataInfo.m_isDataFile = true;
                rawDataInfo.m_pDataFile = TryLoadDataFile( rawDataInfo.m_dataPath, dataFileLoadError, rawDataInfo.m_sourceFileHash );
                if ( rawDataInfo.m_pDataFile == nullptr )
                {
                    rawDataInfo.m_error = String( String::CtorSprintf(), "Failed to load data file: %s, Reason: %s", rawDataInfo.m_dataPath.c_str(), dataFileLoadError.c_str() );
                }
            }
            else // Load raw data
            {
                String dataFileLoadError;
                rawDataInfo.m_isDataFile = false;
                rawDataInfo.m_pRawData = TryLoadRawData( rawDataInfo.m_dataPath, dataFileLoadError, rawDataInfo.m_sourceFileHash );
                if ( rawDataInfo.m_pRawData == nullptr )
                {
                    rawDataInfo.m_error = String( String::CtorSprintf(), "Failed to load raw data: %s, Reason: %s", rawDataInfo.m_dataPath.c_str(), dataFileLoadError.c_str() );
                }
            }
        }
        else
        {
            rawDataInfo.m_error = "Source file doesnt exist!";
        }

        return rawDataInfo;
    }

    Compiler const* CompileContext::TryGetCompiler() const
    {
        EE_ASSERT( HasValidArguments() );
        EE_ASSERT( m_resourceID.IsValid() );
        return m_compilerRegistry.GetCompilerForResourceType( m_resourceID.GetResourceTypeID() );
    }
}