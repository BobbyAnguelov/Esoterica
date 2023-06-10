#include "CodeGenerator_CPP_Enum.h"
#include "System/TypeSystem/TypeID.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::CPP
{
    static void GenerateFile( std::stringstream& file, String const& exportMacro, ReflectedType const& type )
    {
        TInlineVector<int32_t, 50> sortingConstantIndices; // Sorted list of constant indices
        TInlineVector<int32_t, 50> sortedOrder; // Final order for each constant

        for ( auto i = 0u; i < type.m_enumConstants.size(); i++ )
        {
            sortingConstantIndices.emplace_back( i );
        }

        auto Comparator = [&type] ( int32_t a, int32_t b )
        {
            auto const& elemA = type.m_enumConstants[a];
            auto const& elemB = type.m_enumConstants[b];
            return elemA.m_label < elemB.m_label;
        };

        eastl::sort( sortingConstantIndices.begin(), sortingConstantIndices.end(), Comparator );

        sortedOrder.resize( sortingConstantIndices.size() );

        for ( auto i = 0u; i < type.m_enumConstants.size(); i++ )
        {
            sortedOrder[ sortingConstantIndices[i] ] = i;
        }

        //-------------------------------------------------------------------------

        file << "\n";
        file << "//-------------------------------------------------------------------------\n";
        file << "// Enum Helper: " << type.m_namespace.c_str() << type.m_name.c_str() << "\n";
        file << "//-------------------------------------------------------------------------\n\n";

        if ( type.m_isDevOnly )
        {
            file << "#if EE_DEVELOPMENT_TOOLS\n";
        }

        file << "namespace EE::TypeSystem\n";
        file << "{\n";
        file << "    template<>\n";
        file << "    class " << exportMacro.c_str() << " TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << "> final : public TypeInfo\n";
        file << "    {\n";
        file << "        static TypeInfo* s_pInstance;\n\n";
        file << "    public:\n\n";

        // Static registration Function
        //-------------------------------------------------------------------------

        file << "        static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        file << "        {\n";
        file << "            EE_ASSERT( s_pInstance == nullptr );\n";
        file << "            s_pInstance = EE::New<" << " TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << ">>();\n";
        file << "            s_pInstance->m_ID = TypeSystem::TypeID( \"" << type.m_namespace.c_str() << type.m_name.c_str() << "\" );\n";
        file << "            s_pInstance->m_size = sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";
        file << "            s_pInstance->m_alignment = alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";
        file << "            typeRegistry.RegisterType( s_pInstance );\n\n";

        file << "            TypeSystem::EnumInfo enumInfo;\n";
        file << "            enumInfo.m_ID = TypeSystem::TypeID( \"" << type.m_namespace.c_str() << type.m_name.c_str() << "\" );\n";

        switch ( type.m_underlyingType )
        {
            case TypeSystem::CoreTypeID::Uint8:
            file << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Uint8;\n";
            break;

            case TypeSystem::CoreTypeID::Int8:
            file << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Int8;\n";
            break;

            case TypeSystem::CoreTypeID::Uint16:
            file << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Uint16;\n";
            break;

            case TypeSystem::CoreTypeID::Int16:
            file << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Int16;\n";
            break;

            case TypeSystem::CoreTypeID::Uint32:
            file << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Uint32;\n";
            break;

            case TypeSystem::CoreTypeID::Int32:
            file << "            enumInfo.m_underlyingType = TypeSystem::CoreTypeID::Int32;\n";
            break;

            default:
            EE_HALT();
            break;
        }

        file << "\n";
        file << "            //-------------------------------------------------------------------------\n\n";

        file << "            TypeSystem::EnumInfo::ConstantInfo constantInfo;\n";

        for ( auto i = 0u; i < type.m_enumConstants.size(); i++ )
        {
            String escapedDescription = type.m_enumConstants[i].m_description;
            StringUtils::ReplaceAllOccurrencesInPlace( escapedDescription, "\"", "\\\"" );

            file << "\n";
            file << "            constantInfo.m_ID = StringID( \"" << type.m_enumConstants[i].m_label.c_str() << "\" );\n";
            file << "            constantInfo.m_value = " << type.m_enumConstants[i].m_value << ";\n";
            file << "            constantInfo.m_alphabeticalOrder = " << sortedOrder[i] << ";\n";
            file << "            EE_DEVELOPMENT_TOOLS_ONLY( constantInfo.m_description = \"" << escapedDescription.c_str() << "\" );\n";
            file << "            enumInfo.m_constants.emplace_back( constantInfo );\n";
        }

        file << "\n";
        file << "            //-------------------------------------------------------------------------\n\n";
        file << "            typeRegistry.RegisterEnum( enumInfo );\n";
        file << "        }\n\n";

        // Static unregistration Function
        //-------------------------------------------------------------------------

        file << "        static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        file << "        {\n";
        file << "            EE_ASSERT( s_pInstance != nullptr );\n";
        file << "            typeRegistry.UnregisterEnum( s_pInstance->m_ID );\n";
        file << "            typeRegistry.UnregisterType( s_pInstance );\n";
        file << "            EE::Delete( s_pInstance );\n";
        file << "        }\n\n";

        // Constructor
        //-------------------------------------------------------------------------

        file << "    public:\n\n";

        file << "        TTypeInfo()\n";
        file << "        {\n";

        // Create type info
        file << "            m_ID = TypeSystem::TypeID( \"" << type.m_namespace.c_str() << type.m_name.c_str() << "\" );\n";
        file << "            m_size = sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";
        file << "            m_alignment = alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n\n";

        // Create dev tools info
        file << "            #if EE_DEVELOPMENT_TOOLS\n";
        file << "            m_friendlyName = \"" << type.GetFriendlyName().c_str() << "\";\n";
        file << "            m_category = \"" << type.GetCategory().c_str() << "\";\n";
        file << "            #endif\n";

        file << "        }\n\n";

        // Implement required virtual methods
        //-------------------------------------------------------------------------

        file << "        virtual IReflectedType* CreateType() const override { EE_HALT(); return nullptr; }\n";
        file << "        virtual void CreateTypeInPlace( IReflectedType * pAllocatedMemory ) const override { EE_HALT(); }\n";
        file << "        virtual void LoadResources( Resource::ResourceSystem * pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType * pType ) const override { EE_HALT(); }\n";
        file << "        virtual void UnloadResources( Resource::ResourceSystem * pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType * pType ) const override { EE_HALT(); }\n";
        file << "        virtual LoadingStatus GetResourceLoadingStatus( IReflectedType * pType ) const override { EE_HALT(); return LoadingStatus::Failed; }\n";
        file << "        virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType * pType ) const override { EE_HALT(); return LoadingStatus::Failed; }\n";
        file << "        virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType * pType, uint32_t propertyID ) const override { EE_HALT(); return ResourceTypeID(); }\n";
        file << "        virtual void GetReferencedResources( IReflectedType * pType, TVector<ResourceID>&outReferencedResources ) const override {};\n";
        file << "        virtual uint8_t* GetArrayElementDataPtr( IReflectedType * pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const override { EE_HALT(); return 0; }\n";
        file << "        virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint32_t arrayID ) const override { EE_HALT(); return 0; }\n";
        file << "        virtual size_t GetArrayElementSize( uint32_t arrayID ) const override { EE_HALT(); return 0; }\n";
        file << "        virtual void ClearArray( IReflectedType * pTypeInstance, uint32_t arrayID ) const override { EE_HALT(); }\n";
        file << "        virtual void AddArrayElement( IReflectedType * pTypeInstance, uint32_t arrayID ) const override { EE_HALT(); }\n";
        file << "        virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t insertIdx ) const override { EE_HALT(); }\n";
        file << "        virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const override { EE_HALT(); }\n";
        file << "        virtual void RemoveArrayElement( IReflectedType * pTypeInstance, uint32_t arrayID, size_t arrayIdx ) const override { EE_HALT(); }\n";
        file << "        virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const override { EE_HALT(); return false; }\n";
        file << "        virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const override { EE_HALT(); return false; }\n";
        file << "        virtual void ResetToDefault( IReflectedType * pTypeInstance, uint32_t propertyID ) const override { EE_HALT(); }\n";

        //-------------------------------------------------------------------------

        file << "    };\n\n";

        file << "    TypeInfo* TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << ">::s_pInstance = nullptr;\n";

        file << "}\n";

        if ( type.m_isDevOnly )
        {
            file << "#endif\n";
        }
    }

    //-------------------------------------------------------------------------

    namespace EnumGenerator
    {
        void Generate( std::stringstream& codeFile, String const& exportMacro, ReflectedType const& type )
        {
            EE_ASSERT( type.IsEnum() );
            GenerateFile( codeFile, exportMacro, type );
        }
    }
}