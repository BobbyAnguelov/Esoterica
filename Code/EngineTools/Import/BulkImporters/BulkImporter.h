#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/DataPath.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Import
{
    struct EE_ENGINETOOLS_API RawFileInfo
    {
        inline bool operator==( DataPath const& dp ) const { return m_path == dp; }

    public:

        DataPath                                            m_path;
        FileSystem::Path                                    m_filePath;
        FileSystem::Extension                               m_extension;
        TVector<DataPath>                                   m_dependentResources;
        bool                                                m_hasRunDependencyCheck = false;
    };

    //-------------------------------------------------------------------------

    class BulkImporter : public IReflectedType
    {
        EE_REFLECT_TYPE( BulkImporter );

    protected:

        struct FileImportContext
        {

        public:

            bool LogError( char const* pFormat, ... ) const;
            void LogWarning( char const* pFormat, ... ) const;
            void LogMessage( char const* pFormat, ... ) const;

            void AppendLog( Log const& log ) const;

        public:

            TypeSystem::TypeRegistry const&                 m_typeRegistry;
            RawFileInfo const&                              m_fileToImport;
            Log&                                            m_log;
            FileSystem::Path const                          m_sourceDataDirectoryPath;
            FileSystem::Path                                m_targetFilePath;
        };

    public:

        BulkImporter() = default;
        BulkImporter( ToolsContext const* pToolsContext );
        virtual ~BulkImporter();

        void Initialize();
        void Shutdown();

        inline bool IsEnabled() const { return m_isEnabled; }
        inline bool IsEnabledAndReadyForImport() const { return m_isEnabled && HasFilesToImport() && HasRequiredSettingsForImport(); }
        virtual char const* GetTitle() const = 0;
        virtual InlineString GetInfoMessage() const { return InlineString(); }
        virtual TInlineVector<FileSystem::Extension, 8> GetSupportedFileExtensions() const = 0;

        void SetRawFiles( TVector<RawFileInfo const*> const& rawFiles );
        inline bool HasFilesToImport() const { return !m_rawFiles.empty(); }
        void Import( Log& importLog, std::atomic<float>& progress, std::atomic<bool> const& cancelSignal );

        void Draw();

    protected:

        virtual ResourceTypeID GetImportedType() const = 0;
        virtual void ImportFile( FileImportContext const& ctx ) = 0;
        virtual bool HasRequiredSettingsForImport() const { return true; }

    protected:

        ToolsContext const*                                 m_pToolsContext = nullptr;
        PropertyGrid*                                       m_pPropertyGrid = nullptr;
        TVector<RawFileInfo const*>                         m_rawFiles;
        bool                                                m_isCollapsed = true;
        bool                                                m_isEnabled = false;
    };
}