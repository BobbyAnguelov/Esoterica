#pragma once

#include "ResourceCompilationDatabase.h"
#include "Base/Time/Time.h"
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct IDataFile;
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceDescriptor;
    class CompilerRegistry;
    struct CompileDependency;

    //-------------------------------------------------------------------------

    // Raw file info
    struct CompileDependencyDataInfo final
    {
        inline bool HasError() const { return !m_error.empty(); }
        inline bool IsDataFile() const { return m_isDataFile; }
        inline bool IsRawData() const { return !m_isDataFile; }

    public:

        DataPath                                        m_dataPath;
        FileSystem::Path                                m_sourcePath;
        uint64_t                                        m_sourceFileHash = 0;
        bool                                            m_sourceFileExists = false;
        IDataFile*                                      m_pDataFile = nullptr;
        Blob*                                           m_pRawData = nullptr;
        String                                          m_error;
        bool                                            m_isDataFile = false;
    };

    //-------------------------------------------------------------------------

    // Info about a given resource
    struct CompileDependencyResourceInfo final
    {
        ~CompileDependencyResourceInfo();

        inline bool HasError() const { return !m_error.empty(); }
        void Clear();

        uint64_t CalculateCombinedHash();

        // Get the resource info for a given path
        void GetResourceInfo( DataPath const& path, TInlineVector<CompileDependencyResourceInfo*, 2>& outInfos );

        // Get the data info for a given path
        void GetDataInfo( DataPath const& path, TInlineVector<CompileDependencyDataInfo*, 2>& outInfos );

        // Get all the direct compile dependencies for this resource
        void GetAllCompileDependencyPaths( TVector<DataPath>& outDependencies ) const;

    public:

        ResourceID                                      m_ID;
        CompileDependencyResourceInfo*                  m_pParent = nullptr;
        CompiledResourceRecord                          m_compiledResourceRecord;
        uint64_t                                        m_compiledResourceVersion = UINT64_MAX;

        FileSystem::Path                                m_sourcePath;
        uint64_t                                        m_sourceFileHash = 0;
        uint64_t                                        m_customSourceHash = 0; // Optional context dependent override of the source hash for this descriptor
        uint64_t                                        m_combinedHash = 0; // The total hash of this descriptor and all children
        bool                                            m_sourceFileExists = false;

        FileSystem::Path                                m_outputPath;
        FileSystem::Path                                m_additionalOutputPath;
        bool                                            m_outputFileExists = false;
        bool                                            m_additionalOutputFileExists = false;

        ResourceDescriptor*                             m_pDescriptor = nullptr;
        TVector<CompileDependencyResourceInfo*>         m_resourceDependencies;
        TVector<CompileDependencyDataInfo>              m_dataDependencies;

        String                                          m_error;
    };

    //-------------------------------------------------------------------------

    enum class CompilationResult : int32_t
    {
        Failure = -1,
        SuccessUpToDate = 0,
        Success = 1,
        SuccessWithWarnings = 2,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API CompileContext
    {
        friend class Compiler;

    public:

        CompileContext( ResourceID resourceID, TypeSystem::TypeRegistry const& typeRegistry, CompiledResourceDatabase& compiledResourceDB, CompilerRegistry const& compilerRegistry, FileSystem::Path const& sourceResourceDirectoryPath, FileSystem::Path const& compiledResourceDirectoryPath, Platform::Target platform = Platform::Target::PC, bool isCompilingForPackagedBuild = false )
            : m_resourceID( resourceID )
            , m_typeRegistry( typeRegistry )
            , m_compiledResourceDB( compiledResourceDB )
            , m_compilerRegistry( compilerRegistry )
            , m_sourceResourceDirectoryPath( sourceResourceDirectoryPath )
            , m_compiledResourceDirectoryPath( compiledResourceDirectoryPath )
            , m_platform( platform )
            , m_isCompilingForPackagedBuild( isCompilingForPackagedBuild )
        {}

        ~CompileContext();

        inline bool HasValidArguments() const;
        inline bool IsValid() const { return m_pResourceToCompile != nullptr; }

        EE_FORCE_INLINE bool IsCompilingForDevelopmentBuild() const { return !m_isCompilingForPackagedBuild; }
        EE_FORCE_INLINE bool IsCompilingForPackagedBuild() const { return m_isCompilingForPackagedBuild; }

        inline ResourceID const& GetResourceID() const { EE_ASSERT( IsValid() ); return m_pResourceToCompile->m_ID; }
        inline FileSystem::Path const& GetInputPath() const { EE_ASSERT( IsValid() ); return m_pResourceToCompile->m_sourcePath; }
        inline FileSystem::Path const& GetOutputPath() const { EE_ASSERT( IsValid() ); return m_pResourceToCompile->m_outputPath; }
        inline FileSystem::Path const& GetAdditionalOutputPath() const { EE_ASSERT( IsValid() ); return m_pResourceToCompile->m_additionalOutputPath; }

        //-------------------------------------------------------------------------

        CompilationResult LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;
        void LogMessage( char const* pFormat, ... ) const;

        //-------------------------------------------------------------------------

        // Get the resource info for a given path
        inline void GetResourceInfo( DataPath const& path, TInlineVector<CompileDependencyResourceInfo*, 2>& outInfos ) { EE_ASSERT( IsValid() ); m_pResourceToCompile->GetResourceInfo( path, outInfos ); }

        // Get the data info for a given path
        inline void GetDataInfo( DataPath const& path, TInlineVector<CompileDependencyDataInfo*, 2>& outInfos ) { EE_ASSERT( IsValid() ); m_pResourceToCompile->GetDataInfo( path, outInfos ); }

        // Get the descriptor for the resource to compile
        template<typename T>
        inline T const* GetDescriptor() const
        {
            EE_ASSERT( m_pResourceToCompile != nullptr && m_pResourceToCompile->m_pDescriptor != nullptr );
            return Cast<T>( m_pResourceToCompile->m_pDescriptor );
        }

        // Try to get a loaded descriptor for a given resource
        template<typename T>
        inline T const* GetDescriptor( ResourceID const& ID ) const
        {
            auto iter = m_loadedDescriptorLUT.find( ID );
            if ( iter == m_loadedDescriptorLUT.end() )
            {
                EE_HALT();
                return nullptr;
            }

            return Cast<T>( iter->second );
        }

        // Try to get a loaded descriptor for a given resource
        template<typename T>
        inline T const* GetDataFile( DataPath const& path ) const
        {
            auto iter = m_loadedDataFileLUT.find( path );
            if ( iter == m_loadedDataFileLUT.end() )
            {
                EE_HALT();
                return nullptr;
            }

            return Cast<T>( iter->second );
        }

        // Try to get a loaded descriptor for a given resource
        inline Blob const* GetRawData( DataPath const& path ) const
        {
            auto iter = m_loadedRawDataLUT.find( path );
            if ( iter == m_loadedRawDataLUT.end() )
            {
                EE_HALT();
                return nullptr;
            }

            return iter->second;
        }

        Compiler const* TryGetCompiler() const;

    private:

        ResourceDescriptor* TryLoadResourceDescriptor( ResourceID const& ID, String& errorMessage, uint64_t& outFileHash );
        IDataFile* TryLoadDataFile( DataPath const& dataPath, String& errorMessage, uint64_t& outFileHash );
        Blob* TryLoadRawData( DataPath const& dataPath, String& errorMessage, uint64_t& outFileHash );

        CompileDependencyResourceInfo* CreateResourceInfo( CompileDependencyResourceInfo* pParentInfo, ResourceID const& resourceID );
        CompileDependencyDataInfo const& CreateDataInfo( CompileDependencyResourceInfo* pParentInfo, CompileDependency const& cd );

        void CalculateSourceResourceHash() { m_sourceResourceHash = m_pResourceToCompile->CalculateCombinedHash(); }

        void AppendLogEntries( Log const& log );

    public:

        // Init Args
        //-------------------------------------------------------------------------

        ResourceID const                                m_resourceID;
        TypeSystem::TypeRegistry const&                 m_typeRegistry;
        CompiledResourceDatabase&                       m_compiledResourceDB;
        CompilerRegistry const&                         m_compilerRegistry;
        FileSystem::Path const                          m_sourceResourceDirectoryPath;
        FileSystem::Path const                          m_compiledResourceDirectoryPath;
        Platform::Target const                          m_platform = Platform::Target::PC;
        bool const                                      m_isCompilingForPackagedBuild = false;

        // Options
        //-------------------------------------------------------------------------

        bool                                            m_forceCompilation = false;
        bool                                            m_isStandaloneCompile = false;

        // Working Data
        //-------------------------------------------------------------------------

        String                                          m_compilerName;
        CompileDependencyResourceInfo*                  m_pResourceToCompile = nullptr;
        TVector<ResourceID>                             m_uniqueCompileDependencies;
        THashMap<ResourceID, ResourceDescriptor*>       m_loadedDescriptorLUT;
        THashMap<DataPath, IDataFile*>                  m_loadedDataFileLUT;
        THashMap<DataPath, Blob*>                       m_loadedRawDataLUT;
        uint64_t                                        m_sourceResourceHash = 0;
        bool                                            m_requiresCompilation = false;

        Milliseconds                                    m_createDependencyTreeTime = 0.0f;
        Milliseconds                                    m_upToDateCheckTime = 0;
        Milliseconds                                    m_compilationTime = 0;
        mutable Log                                     m_log;

        CompilationResult                               m_result = CompilationResult::Failure;
    };
}