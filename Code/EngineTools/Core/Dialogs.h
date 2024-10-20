#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Resource/ResourceTypeID.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;

    // File Dialogs
    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API FileDialog
    {
        struct Result
        {
            inline operator bool() const { return !m_filePaths.empty(); }

            inline operator FileSystem::Path const&( ) const 
            {
                EE_ASSERT( !m_filePaths.empty() );
                return m_filePaths.front();
            }

        public:

            TInlineVector<FileSystem::Path, 2> m_filePaths;
        };

        struct ExtensionFilter
        {
            ExtensionFilter() = default;
            ExtensionFilter( FileSystem::Extension ext, String displayText = String() );

            inline bool IsValid() const { return !m_extension.empty(); }

            FileSystem::Extension   m_extension;
            WString                 m_filter;
            WString                 m_displayText;
        };

    public:

        [[nodiscard]] static Result SelectFolder( FileSystem::Path const& startingPath = FileSystem::Path() );

        [[nodiscard]] static Result Load( TInlineVector<ExtensionFilter, 2> const& filters = TInlineVector<ExtensionFilter, 2>(), String const& title = String(), bool allowMultiselect = false, FileSystem::Path const& startingPath = FileSystem::Path() );
        [[nodiscard]] static Result Load( ToolsContext const* pToolsContext, ResourceTypeID resourceTypeID, FileSystem::Path const& startingPath = FileSystem::Path() );

        [[nodiscard]] static Result Save( TInlineVector<ExtensionFilter, 2> const& filters = TInlineVector<ExtensionFilter, 2>(), String const& title = String(), FileSystem::Path const& startingPath = FileSystem::Path() );
        [[nodiscard]] static Result Save( ToolsContext const* pToolsContext, ResourceTypeID resourceTypeID, FileSystem::Path const& startingPath = FileSystem::Path() );
    };

    // Message Boxes
    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API MessageDialog
    {
        enum class Type
        {
            Ok = 0,
            OkCancel,
            YesNo,
            YesNoCancel,
            RetryCancel,
            AbortRetryIgnore,
            CancelTryContinue,
        };

        enum class Result
        {
            No,
            Cancel,
            Yes,
            Retry,
            Continue,
        };

    public:

        [[nodiscard]] static Result ShowEx( Severity severity, Type type, String const& title, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            String message;
            message.sprintf_va_list( pMessageFormat, args );
            va_end( args );

            return ShowInternal( severity, type, title, message );
        }

        static void Info( String const& title, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            String message;
            message.sprintf_va_list( pMessageFormat, args );
            va_end( args );

            ShowInternal( Severity::Info, Type::Ok, title, message );
        }

        static void Warning( String const& title, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            String message;
            message.sprintf_va_list( pMessageFormat, args );
            va_end( args );

            ShowInternal( Severity::Info, Type::Ok, title, message );
        }

        static void Error( String const& title, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            String message;
            message.sprintf_va_list( pMessageFormat, args );
            va_end( args );

            ShowInternal( Severity::Error, Type::Ok, title, message );
        }

        static bool Confirmation( Severity severity, String const& title, char const* pMessageFormat, ... )
        {
            va_list args;
            va_start( args, pMessageFormat );
            String message;
            message.sprintf_va_list( pMessageFormat, args );
            va_end( args );

            return ShowInternal( Severity::Info, Type::YesNo, title, message ) == Result::Yes;
        }

    private:

        static Result ShowInternal( Severity severity, Type type, String const& title, String const& message );
    };
}