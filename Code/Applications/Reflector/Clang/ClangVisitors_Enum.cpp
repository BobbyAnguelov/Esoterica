#include "ClangVisitors_Enum.h"
#include "Applications/Reflector/Database/ReflectionDataTypes.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static CXChildVisitResult VisitEnumContents( CXCursor cr, CXCursor parent, CXClientData pClientData )
    {
        auto pContext = reinterpret_cast<ClangParserContext*>( pClientData );

        CXCursorKind kind = clang_getCursorKind( cr );
        if ( kind == CXCursor_EnumConstantDecl )
        {
            auto pEnum = reinterpret_cast<ReflectedType*>( pContext->m_pParentReflectedType );
            clang::EnumConstantDecl* pEnumConstantDecl = (clang::EnumConstantDecl*) cr.data[0];

            ReflectedEnumConstant constant;

            // Set Label
            constant.m_label = ClangUtils::GetCursorDisplayName( cr );
            constant.m_ID = StringID( constant.m_label );

            // Set Value
            auto const& initVal = pEnumConstantDecl->getInitVal();
            constant.m_value = (int32_t) initVal.getExtValue();

            // Set property description
            CXString const commentString = clang_Cursor_getBriefCommentText( cr );
            if ( commentString.data != nullptr )
            {
                constant.m_description = clang_getCString( commentString );
            }
            clang_disposeString( commentString );

            pEnum->AddEnumConstant( constant );
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr, StringID const headerID )
    {
        auto cursorName = ClangUtils::GetCursorDisplayName( cr );

        String fullyQualifiedCursorName;
        if ( !ClangUtils::GetQualifiedNameForType( clang_getCursorType( cr ), fullyQualifiedCursorName ) )
        {
            pContext->LogError( "Failed to get qualified type for cursor: %s", cursorName.c_str() );
            return CXChildVisit_Break;
        }

        //-------------------------------------------------------------------------

        clang::EnumDecl* pEnumDecl = (clang::EnumDecl*) cr.data[0];
        clang::QualType integerType = pEnumDecl->getIntegerType();

        if ( integerType.isNull() )
        {
            pContext->LogError( "Failed to get underlying type for enum: %s. You must specify the underlying integer type for exposed enums!", fullyQualifiedCursorName.c_str() );
            return CXChildVisit_Break;
        }

        CoreTypeID underlyingCoreType;

        auto const* pBT = integerType.getTypePtr()->getAs<clang::BuiltinType>();
        switch ( pBT->getKind() )
        {
            case clang::BuiltinType::UChar:
            underlyingCoreType = CoreTypeID::Uint8;
            break;

            case clang::BuiltinType::SChar:
            underlyingCoreType = CoreTypeID::Int8;
            break;

            case clang::BuiltinType::UShort:
            underlyingCoreType = CoreTypeID::Uint16;
            break;

            case clang::BuiltinType::Short:
            underlyingCoreType = CoreTypeID::Int16;
            break;

            case clang::BuiltinType::UInt:
            underlyingCoreType = CoreTypeID::Uint32;
            break;

            case clang::BuiltinType::Int:
            underlyingCoreType = CoreTypeID::Int32;
            break;

            case clang::BuiltinType::ULongLong:
            case clang::BuiltinType::LongLong:
            {
                pContext->LogError( "64bit enum detected: %s. This is not supported!", fullyQualifiedCursorName.c_str() );
                return CXChildVisit_Break;
            }
            break;

            default:
            {
                pContext->LogError( "Unknown underlying type for enum: %s", fullyQualifiedCursorName.c_str() );
                return CXChildVisit_Break;
            }
            break;
        }

        //-------------------------------------------------------------------------

        auto enumTypeID = pContext->GenerateTypeID( fullyQualifiedCursorName );
        if ( !pContext->IsInEngineNamespace() )
        {
            return CXChildVisit_Continue;
        }

        ReflectionMacro macro;
        if ( pContext->GetReflectionMacroForType( headerID, cr, macro ) )
        {
            ReflectedType enumDescriptor( enumTypeID, cursorName );
            enumDescriptor.m_headerID = headerID;
            enumDescriptor.m_namespace = pContext->GetCurrentNamespace();
            enumDescriptor.m_flags.SetFlag( ReflectedType::Flags::IsEnum );
            enumDescriptor.m_underlyingType = underlyingCoreType;

            // Record current parent type, and update it to the new type
            void* pPreviousParentReflectedType = pContext->m_pParentReflectedType;
            pContext->m_pParentReflectedType = &enumDescriptor;
            {
                clang_visitChildren( cr, VisitEnumContents, pContext );
            }
            // Reset parent type back to original parent
            pContext->m_pParentReflectedType = pPreviousParentReflectedType;

            //-------------------------------------------------------------------------

            if ( pContext->m_detectDevOnlyTypesAndProperties )
            {
                if ( pContext->m_pDatabase->IsTypeRegistered( enumDescriptor.m_ID ) )
                {
                    pContext->m_pDatabase->UpdateRegisteredType( &enumDescriptor );
                }
                else
                {
                    pContext->LogError( "Trying to flag unknown type as a runtime type: %s in file: %s", enumDescriptor.m_ID.c_str(), headerID.c_str() );
                    return CXChildVisit_Break;
                }
            }
            else // Register type
            {
                if ( !pContext->m_pDatabase->IsTypeRegistered( enumDescriptor.m_ID ) )
                {
                    pContext->m_pDatabase->RegisterType( &enumDescriptor );
                }
                else // We do not allow multiple resources registered with the same ID
                {
                    pContext->LogError( "Duplicate enum type ID encountered: %s in file: %s", enumDescriptor.m_ID.c_str(), headerID.c_str() );
                    return CXChildVisit_Break;
                }
            }
        }

        return CXChildVisit_Continue;
    }
}