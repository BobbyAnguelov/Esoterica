#pragma once

#pragma warning( disable : 4714 4267 4244 4512 4800 4389 4146 4291 4324 4458 4141 4459 4245 4624 4996 )

#include <clang/AST/Ast.h>
#include <clang/AST/Type.h>
#include <clang-c/Index.h>

#include "Base/FileSystem/FileSystemPath.h"
#include "Base/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace ClangUtils
    {
        //-------------------------------------------------------------------------
        // Cursors
        //-------------------------------------------------------------------------

        inline String GetString( CXString&& string )
        {
            String str = clang_getCString( string );
            clang_disposeString( string );
            return str;
        }

        inline String GetCursorDisplayName( CXCursor const& cr )
        {
            auto displayName = clang_getCursorDisplayName( cr );
            String str = clang_getCString( displayName );
            clang_disposeString( displayName );
            return str;
        }

        inline uint32_t GetLineNumberForCursor( CXCursor const& cr )
        {
            uint32_t line, column, offset;
            CXSourceRange range = clang_getCursorExtent( cr );
            CXSourceLocation start = clang_getRangeStart( range );
            clang_getExpansionLocation( start, nullptr, &line, &column, &offset );
            return line;
        }

        void GetDiagnostics( CXTranslationUnit& TU, TVector<String>& diagnostics );

        FileSystem::Path GetHeaderPathForCursor( CXCursor cr );

        //-------------------------------------------------------------------------
        // QualTypes
        //-------------------------------------------------------------------------

        bool GetQualifiedNameForType( clang::QualType type, String& qualifiedName );

        inline clang::QualType GetQualType( CXType type ) 
        {
            return clang::QualType::getFromOpaquePtr( type.data[0] ); 
        }

        inline bool GetQualifiedNameForType( CXType type, String& qualifiedName )
        {
            return GetQualifiedNameForType( GetQualType( type ), qualifiedName );
        }

        //-------------------------------------------------------------------------
        // Misc
        //-------------------------------------------------------------------------

        inline bool GetAllBaseClasses( TVector<TypeSystem::TypeID>& baseClasses, clang::CXXBaseSpecifier& baseSpecifier )
        {
            String fullyQualifiedName;
            if ( !ClangUtils::GetQualifiedNameForType( baseSpecifier.getType(), fullyQualifiedName ) )
            {
                return false;
            }

            baseClasses.emplace_back( TypeSystem::TypeID( fullyQualifiedName ) );

            clang::CXXRecordDecl* pBaseSpecifierRecordDecl = baseSpecifier.getType()->getAsCXXRecordDecl();
            for ( auto parentBaseSpecifier : pBaseSpecifierRecordDecl->bases() )
            {
                if ( !GetAllBaseClasses( baseClasses, parentBaseSpecifier ) )
                {
                    return false;
                }
            }

            return true;
        }
    }
}