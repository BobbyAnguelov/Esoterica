#include "ClangUtils.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace ClangUtils
    {
        void GetDiagnostics( CXTranslationUnit& TU, TVector<String>& diagnostics )
        {
            auto const numDiagnostics = clang_getNumDiagnostics( TU );
            for ( auto i = 0u; i < numDiagnostics; i++ )
            {
                CXDiagnostic diagnostic = clang_getDiagnostic( TU, i );
                diagnostics.push_back( GetString( clang_formatDiagnostic( diagnostic, clang_defaultDiagnosticDisplayOptions() ) ) );
            }
        }

        FileSystem::Path GetHeaderPathForCursor( CXCursor cr )
        {
            CXFile pFile;
            CXSourceRange const cursorRange = clang_getCursorExtent( cr );
            clang_getExpansionLocation( clang_getRangeStart( cursorRange ), &pFile, nullptr, nullptr, nullptr );

            FileSystem::Path HeaderFilePath;
            if ( pFile != nullptr )
            {
                CXString clangFilePath = clang_File_tryGetRealPathName( pFile );
                HeaderFilePath = FileSystem::Path( clang_getCString( clangFilePath ) );
                clang_disposeString( clangFilePath );
            }

            return HeaderFilePath;
        }

        bool GetQualifiedNameForType( clang::QualType type, String& qualifiedName )
        {
            clang::Type const* pType = type.getTypePtr();

            if ( pType->isArrayType() )
            {
                auto elementType = pType->castAsArrayTypeUnsafe()->getElementType();
                if ( !GetQualifiedNameForType( elementType, qualifiedName ) )
                {
                    return false;
                }
            }
            else if ( pType->isBooleanType() )
            {
                qualifiedName = "bool";
            }
            else if ( pType->isBuiltinType() )
            {
                auto const* pBT = pType->getAs<clang::BuiltinType>();
                switch ( pBT->getKind() )
                {
                    case clang::BuiltinType::Char_S:
                    qualifiedName = "int8_t";
                    break;

                    case clang::BuiltinType::Char_U:
                    qualifiedName = "uint8_t";
                    break;

                    case clang::BuiltinType::UChar:
                    qualifiedName = "uint8_t";
                    break;

                    case clang::BuiltinType::SChar:
                    qualifiedName = "int8_t";
                    break;

                    case clang::BuiltinType::Char16:
                    qualifiedName = "uint16_t";
                    break;

                    case clang::BuiltinType::Char32:
                    qualifiedName = "uint32_t";
                    break;

                    case clang::BuiltinType::UShort:
                    qualifiedName = "uint16_t";
                    break;

                    case clang::BuiltinType::Short:
                    qualifiedName = "int16_t";
                    break;

                    case clang::BuiltinType::UInt:
                    qualifiedName = "uint32_t";
                    break;

                    case clang::BuiltinType::Int:
                    qualifiedName = "int32_t";
                    break;

                    case clang::BuiltinType::ULongLong:
                    qualifiedName = "uint64_t";
                    break;

                    case clang::BuiltinType::LongLong:
                    qualifiedName = "int64_t";
                    break;

                    case clang::BuiltinType::Float:
                    qualifiedName = "float";
                    break;

                    case clang::BuiltinType::Double:
                    qualifiedName = "double";
                    break;

                    default:
                    {
                        return false;
                    }
                };
            }
            else if ( pType->isPointerType() || pType->isReferenceType() )
            {
                // Do Nothing
            }
            else if ( pType->isRecordType() )
            {
                clang::RecordDecl const* pRecordDecl = pType->getAs<clang::RecordType>()->getDecl();
                EE_ASSERT( pRecordDecl != nullptr );
                qualifiedName = pRecordDecl->getQualifiedNameAsString().c_str();
            }
            else if ( pType->isEnumeralType() )
            {
                clang::NamedDecl const* pNamedDecl = pType->getAs<clang::EnumType>()->getDecl();
                EE_ASSERT( pNamedDecl != nullptr );
                qualifiedName = pNamedDecl->getQualifiedNameAsString().c_str();
            }
            else if ( pType->getTypeClass() == clang::Type::Typedef )
            {
                clang::NamedDecl const* pNamedDecl = pType->getAs<clang::TypedefType>()->getDecl();
                EE_ASSERT( pNamedDecl != nullptr );
                qualifiedName = pNamedDecl->getQualifiedNameAsString().c_str();
            }
            else
            {
                return false;
            }

            // Convert 3rd party types to EE types
            //-------------------------------------------------------------------------

            if ( qualifiedName == "eastl::basic_string" )
            {
                qualifiedName = "EE::String";
            }
            else  if ( qualifiedName == "eastl::string" )
            {
                qualifiedName = "EE::String";
            }
            else if ( qualifiedName == "eastl::vector" )
            {
                qualifiedName = "EE::TVector";
            }
            else if ( qualifiedName == "eastl::fixed_vector" )
            {
                qualifiedName = "EE::TInlineVector";
            }
            return true;
        }
    }
}
