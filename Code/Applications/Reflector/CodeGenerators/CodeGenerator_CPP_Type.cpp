#include "CodeGenerator_CPP_Type.h"
#include "Base/TypeSystem/TypeID.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::CPP
{
    //-------------------------------------------------------------------------
    // Factory/Serialization Methods
    //-------------------------------------------------------------------------

    static void GenerateCreationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual IReflectedType* CreateType() const override final\n";
        file << "            {\n";
        if ( !type.IsAbstract() )
        {
            file << "                auto pMemory = EE::Alloc( sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ), alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ) );\n";
            file << "                return new ( pMemory ) " << type.m_namespace.c_str() << type.m_name.c_str() << "();\n";
        }
        else
        {
            file << "                EE_HALT(); // Error! Trying to instantiate an abstract type!\n";
            file << "                return nullptr;\n";
        }
        file << "            }\n\n";
    }

    static void GenerateInPlaceCreationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void CreateTypeInPlace( IReflectedType* pAllocatedMemory ) const override final\n";
        file << "            {\n";
        if ( !type.IsAbstract() )
        {
            file << "                EE_ASSERT( pAllocatedMemory != nullptr );\n";
            file << "                new( pAllocatedMemory ) " << type.m_namespace.c_str() << type.m_name.c_str() << "();\n";
        }
        else
        {
            file << "                EE_HALT(); // Error! Trying to instantiate an abstract type!\n";
        }
        file << "            }\n\n";
    }

    //-------------------------------------------------------------------------
    // Array Methods
    //-------------------------------------------------------------------------

    static void GenerateArrayAccessorMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual uint8_t* GetArrayElementDataPtr( IReflectedType* pType, uint32_t arrayID, size_t arrayIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    if ( ( arrayIdx + 1 ) >= pActualType->" << propertyDesc.m_name.c_str() << ".size() )\n";
                    file << "                    {\n";
                    file << "                        pActualType->" << propertyDesc.m_name.c_str() << ".resize( arrayIdx + 1 );\n";
                    file << "                    }\n\n";
                    file << "                    return (uint8_t*) &pActualType->" << propertyDesc.m_name.c_str() << "[arrayIdx];\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( propertyDesc.IsStaticArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    return (uint8_t*) &pActualType->" << propertyDesc.m_name.c_str() << "[arrayIdx];\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return nullptr;\n";
        file << "            }\n\n";
    }

    static void GenerateArraySizeMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual size_t GetArraySize( IReflectedType const* pTypeInstance, uint32_t arrayID ) const override final\n";
        file << "            {\n";

        if ( type.HasArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    return pActualType->" << propertyDesc.m_name.c_str() << ".size();\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( propertyDesc.IsStaticArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    return " << propertyDesc.GetArraySize() << ";\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return 0;\n";
        file << "            }\n\n";
    }

    static void GenerateArrayElementSizeMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual size_t GetArrayElementSize( uint32_t arrayID ) const override final\n";
        file << "            {\n";

        for ( auto& propertyDesc : type.m_properties )
        {
            String const templateSpecializationString = propertyDesc.m_templateArgTypeName.empty() ? String() : "<" + propertyDesc.m_templateArgTypeName + ">";

            if ( propertyDesc.IsArrayProperty() )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                file << "                {\n";
                file << "                    return sizeof( " << propertyDesc.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                file << "                }\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }

                file << "\n";
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return 0;\n";
        file << "            }\n\n";
    }

    static void GenerateArrayClearMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void ClearArray( IReflectedType* pTypeInstance, uint32_t arrayID ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".clear();\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    static void GenerateAddArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void AddArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".emplace_back();\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    static void GenerateInsertArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void InsertArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t insertionIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".emplace( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + insertionIdx );\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    static void GenerateMoveArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void MoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t originalElementIdx, size_t newElementIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    auto const originalElement = pActualType->" << propertyDesc.m_name.c_str() << "[originalElementIdx];\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".erase( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + originalElementIdx );\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".insert( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + newElementIdx, originalElement );\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    static void GenerateRemoveArrayElementMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void RemoveArrayElement( IReflectedType* pTypeInstance, uint32_t arrayID, size_t elementIdx ) const override final\n";
        file << "            {\n";

        if ( type.HasDynamicArrayProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.IsDynamicArrayProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    file << "                if ( arrayID == " << propertyDesc.m_propertyID << " )\n";
                    file << "                {\n";
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << ".erase( pActualType->" << propertyDesc.m_name.c_str() << ".begin() + elementIdx );\n";
                    file << "                    return;\n";
                    file << "                }\n";

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                // We should never get here since we are asking for a ptr to an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "            }\n\n";
    }

    //-------------------------------------------------------------------------
    // Default Value Methods
    //-------------------------------------------------------------------------

    static void GenerateAreAllPropertiesEqualMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual bool AreAllPropertyValuesEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance ) const override final\n";
        file << "            {\n";

        if ( type.HasProperties() )
        {
            file << "                auto pType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pTypeInstance );\n";
            file << "                auto pOtherType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pOtherTypeInstance );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if( !IsPropertyValueEqual( pType, pOtherType, " << propertyDesc.m_propertyID << " ) )\n";
                file << "                {\n";
                file << "                    return false;\n";
                file << "                }\n\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }
            }
        }

        file << "                return true;\n";
        file << "            }\n\n";
    }

    static void GenerateIsPropertyEqualMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual bool IsPropertyValueEqual( IReflectedType const* pTypeInstance, IReflectedType const* pOtherTypeInstance, uint32_t propertyID, int32_t arrayIdx = InvalidIndex ) const override final\n";
        file << "            {\n";

        if ( type.HasProperties() )
        {
            file << "                auto pType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pTypeInstance );\n";
            file << "                auto pOtherType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( pOtherTypeInstance );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                InlineString propertyTypeName = propertyDesc.m_typeName.c_str();
                if ( !propertyDesc.m_templateArgTypeName.empty() )
                {
                    propertyTypeName += "<";
                    propertyTypeName += propertyDesc.m_templateArgTypeName.c_str();
                    propertyTypeName += ">";
                }

                //-------------------------------------------------------------------------

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                file << "                {\n";

                // Arrays
                if ( propertyDesc.IsArrayProperty() )
                {
                    // Handle individual element comparison
                    //-------------------------------------------------------------------------

                    file << "                    // Compare array elements\n";
                    file << "                    if ( arrayIdx != InvalidIndex )\n";
                    file << "                    {\n";

                    // If it's a dynamic array check the sizes first
                    if ( propertyDesc.IsDynamicArrayProperty() )
                    {
                        file << "                        if ( arrayIdx >= pOtherType->" << propertyDesc.m_name.c_str() << ".size() )\n";
                        file << "                        {\n";
                        file << "                        return false;\n";
                        file << "                        }\n\n";
                    }

                    if ( propertyDesc.IsStructureProperty() )
                    {
                        file << "                        return " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->AreAllPropertyValuesEqual( &pType->" << propertyDesc.m_name.c_str() << "[arrayIdx], &pOtherType->" << propertyDesc.m_name.c_str() << "[arrayIdx] );\n";
                    }
                    else
                    {
                        file << "                        return pType->" << propertyDesc.m_name.c_str() << "[arrayIdx] == pOtherType->" << propertyDesc.m_name.c_str() << "[arrayIdx];\n";
                    }
                    file << "                    }\n";

                    // Handle array comparison
                    //-------------------------------------------------------------------------

                    file << "                    else // Compare entire array contents\n";
                    file << "                    {\n";

                    // If it's a dynamic array check the sizes first
                    if ( propertyDesc.IsDynamicArrayProperty() )
                    {
                        file << "                        if ( pType->" << propertyDesc.m_name.c_str() << ".size() != pOtherType->" << propertyDesc.m_name.c_str() << ".size() )\n";
                        file << "                        {\n";
                        file << "                        return false;\n";
                        file << "                        }\n\n";

                        file << "                        for ( size_t i = 0; i < pType->" << propertyDesc.m_name.c_str() << ".size(); i++ )\n";
                    }
                    else
                    {
                        file << "                        for ( size_t i = 0; i < " << propertyDesc.GetArraySize() << "; i++ )\n";
                    }

                    file << "                        {\n";

                    if ( propertyDesc.IsStructureProperty() )
                    {
                        file << "                        if( !" << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->AreAllPropertyValuesEqual( &pType->" << propertyDesc.m_name.c_str() << "[i], &pOtherType->" << propertyDesc.m_name.c_str() << "[i] ) )\n";
                        file << "                        {\n";
                        file << "                            return false;\n";
                        file << "                        }\n";
                    }
                    else
                    {
                        file << "                        if( pType->" << propertyDesc.m_name.c_str() << "[i] != pOtherType->" << propertyDesc.m_name.c_str() << "[i] )\n";
                        file << "                        {\n";
                        file << "                            return false;\n";
                        file << "                        }\n";
                    }

                    file << "                        }\n\n";

                    file << "                        return true;\n";
                    file << "                    }\n";
                }
                else // Non-Array properties
                {
                    if ( propertyDesc.IsStructureProperty() )
                    {
                        file << "                    return " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->AreAllPropertyValuesEqual( &pType->" << propertyDesc.m_name.c_str() << ", &pOtherType->" << propertyDesc.m_name.c_str() << " );\n";
                    }
                    else
                    {
                        file << "                    return pType->" << propertyDesc.m_name.c_str() << " == pOtherType->" << propertyDesc.m_name.c_str() << ";\n";
                    }
                }

                file << "                }\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }

                file << "\n";
            }
        }
        else
        {
            file << "                EE_UNREACHABLE_CODE();\n";
        }

        file << "                return false;\n";
        file << "            }\n\n";
    }

    static void GenerateSetToDefaultValueMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void ResetToDefault( IReflectedType* pTypeInstance, uint32_t propertyID ) const override final\n";
        file << "            {\n";

        if ( type.HasProperties() )
        {
            file << "                auto pDefaultType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( m_pDefaultInstance );\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pTypeInstance );\n";
            file << "                EE_ASSERT( pActualType != nullptr && pDefaultType != nullptr );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                file << "                {\n";

                if ( propertyDesc.IsStaticArrayProperty() )
                {
                    for ( auto i = 0u; i < propertyDesc.GetArraySize(); i++ )
                    {
                        file << "                    pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] = pDefaultType->" << propertyDesc.m_name.c_str() << "[" << i << "];\n";
                    }
                }
                else
                {
                    file << "                    pActualType->" << propertyDesc.m_name.c_str() << " = pDefaultType->" << propertyDesc.m_name.c_str() << ";\n";
                }

                file << "                    return;\n";
                file << "                }\n";

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n";
                }
            }
        }

        file << "            }\n";
    }

    //-------------------------------------------------------------------------
    // Resource Methods
    //-------------------------------------------------------------------------

    static void GenerateExpectedResourceTypeMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual ResourceTypeID GetExpectedResourceTypeForProperty( IReflectedType* pType, uint32_t propertyID ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrProperties() )
        {
            for ( auto& propertyDesc : type.m_properties )
            {
                bool const isResourceProp = ( propertyDesc.m_typeID == CoreTypeID::ResourcePtr ) || ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr );
                if ( isResourceProp )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr )
                    {
                        file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                        file << "                {\n";
                        file << "                    return " << propertyDesc.m_templateArgTypeName.c_str() << "::GetStaticResourceTypeID();\n";
                        file << "                }\n";
                    }
                    else if ( propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                    {
                        file << "                if ( propertyID == " << propertyDesc.m_propertyID << " )\n";
                        file << "                {\n";
                        file << "                        return ResourceTypeID();\n";
                        file << "            }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n\n";
                    }
                    else
                    {
                        file << "\n";
                    }
                }
            }
        }

        file << "                // We should never get here since we are asking for a resource type of an invalid property\n";
        file << "                EE_UNREACHABLE_CODE();\n";
        file << "                return ResourceTypeID();\n";
        file << "            }\n\n";
    }

    static void GenerateLoadResourcesMethod( ReflectionDatabase const& database, std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void LoadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                EE_ASSERT( pResourceSystem != nullptr );\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.IsSet() )\n";
                            file << "                    {\n";
                            file << "                        pResourceSystem->LoadResource( resourcePtr, requesterID );\n";
                            file << "                    }\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsSet() )\n";
                                file << "                {\n";
                                file << "                    pResourceSystem->LoadResource( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "], requesterID );\n";
                                file << "                }\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() )\n";
                        file << "                {\n";
                        file << "                    pResourceSystem->LoadResource( pActualType->" << propertyDesc.m_name.c_str() << ", requesterID );\n";
                        file << "                }\n\n";
                    }
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->LoadResources( pResourceSystem, requesterID, &propertyValue );\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->LoadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] );\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->LoadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << " );\n\n";
                    }
                }

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n\n";
                }
            }
        }

        file << "            }\n\n";
    }

    static void GenerateUnloadResourcesMethod( ReflectionDatabase const& database, std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void UnloadResources( Resource::ResourceSystem* pResourceSystem, Resource::ResourceRequesterID const& requesterID, IReflectedType* pType ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                EE_ASSERT( pResourceSystem != nullptr );\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #if EE_DEVELOPMENT_TOOLS\n";
                }

                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.IsSet() )\n";
                            file << "                    {\n";
                            file << "                        pResourceSystem->UnloadResource( resourcePtr, requesterID );\n";
                            file << "                    }\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsSet() )\n";
                                file << "                {\n";
                                file << "                    pResourceSystem->UnloadResource( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "], requesterID );\n";
                                file << "                }\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() )\n";
                        file << "                {\n";
                        file << "                    pResourceSystem->UnloadResource( pActualType->" << propertyDesc.m_name.c_str() << ", requesterID );\n";
                        file << "                }\n\n";
                    }
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->UnloadResources( pResourceSystem, requesterID, &propertyValue );\n";
                            file << "                }\n\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->UnloadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] );\n\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->UnloadResources( pResourceSystem, requesterID, &pActualType->" << propertyDesc.m_name.c_str() << " );\n\n";
                    }
                }

                if ( propertyDesc.m_isDevOnly )
                {
                    file << "                #endif\n\n";
                }
            }
        }

        file << "            }\n\n";
    }

    static void GenerateResourceLoadingStatusMethod( ReflectionDatabase const& database, std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual LoadingStatus GetResourceLoadingStatus( IReflectedType* pType ) const override final\n";
        file << "            {\n";
        file << "                LoadingStatus status = LoadingStatus::Loaded;\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "\n";
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto const& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.HasLoadingFailed() )\n";
                            file << "                    {\n";
                            file << "                        status = LoadingStatus::Failed;\n";
                            file << "                    }\n";
                            file << "                    else if ( resourcePtr.IsSet() && !resourcePtr.IsLoaded() )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Loading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].HasLoadingFailed() )\n";
                                file << "                {\n";
                                file << "                    status = LoadingStatus::Failed;\n";
                                file << "                }\n";
                                file << "                else if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() && !pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsLoaded() )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Loading;\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".HasLoadingFailed() )\n";
                        file << "                {\n";
                        file << "                    status = LoadingStatus::Failed;\n";
                        file << "                }\n";
                        file << "                else if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() && !pActualType->" << propertyDesc.m_name.c_str() << ".IsLoaded() )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Loading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( &propertyValue );\n";
                            file << "                    if ( status == LoadingStatus::Loading )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Loading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] ); \n";
                                file << "                if ( status == LoadingStatus::Loading )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Loading;\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << " );\n";
                        file << "                if ( status == LoadingStatus::Loading )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Loading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                return status;\n";
        file << "            }\n\n";
    }

    static void GenerateResourceUnloadingStatusMethod( ReflectionDatabase const& database, std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual LoadingStatus GetResourceUnloadingStatus( IReflectedType* pType ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto const& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    EE_ASSERT( !resourcePtr.IsLoading() );\n";
                            file << "                    if ( !resourcePtr.IsUnloaded() )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Unloading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                EE_ASSERT( !pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsLoading() );\n";
                                file << "                if ( !pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsUnloaded() )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Unloading;\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                EE_ASSERT( !pActualType->" << propertyDesc.m_name.c_str() << ".IsLoading() );\n";
                        file << "                if ( !pActualType->" << propertyDesc.m_name.c_str() << ".IsUnloaded() )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Unloading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    LoadingStatus const status = " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceUnloadingStatus( &propertyValue );\n";
                            file << "                    if ( status != LoadingStatus::Unloaded )\n";
                            file << "                    {\n";
                            file << "                        return LoadingStatus::Unloading;\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceUnloadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "] ) != LoadingStatus::Unloaded )\n";
                                file << "                {\n";
                                file << "                    return LoadingStatus::Unloading;\n";
                                file << "                }\n";

                                if ( i == propertyDesc.m_arraySize - 1 )
                                {
                                    file << "\n";
                                }
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetResourceUnloadingStatus( &pActualType->" << propertyDesc.m_name.c_str() << " ) != LoadingStatus::Unloaded )\n";
                        file << "                {\n";
                        file << "                    return LoadingStatus::Unloading;\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "                return LoadingStatus::Unloaded;\n";
        file << "            }\n\n";
    }

    static void GenerateGetReferencedResourceMethod( ReflectionDatabase const& database, std::stringstream& file, ReflectedType const& type )
    {
        file << "            virtual void GetReferencedResources( IReflectedType* pType, TVector<ResourceID>& outReferencedResources ) const override final\n";
        file << "            {\n";

        if ( type.HasResourcePtrOrStructProperties() )
        {
            file << "                auto pActualType = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << "*>( pType );\n";

            for ( auto& propertyDesc : type.m_properties )
            {
                if ( propertyDesc.m_typeID == CoreTypeID::TResourcePtr || propertyDesc.m_typeID == CoreTypeID::ResourcePtr )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto const& resourcePtr : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    if ( resourcePtr.IsSet() )\n";
                            file << "                    {\n";
                            file << "                        outReferencedResources.emplace_back( resourcePtr.GetResourceID() );\n";
                            file << "                    }\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].IsSet() )\n";
                                file << "                {\n";
                                file << "                    outReferencedResources.emplace_back( pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "].GetResourceID() );\n";
                                file << "                }\n";
                            }
                        }
                    }
                    else
                    {
                        file << "                if ( pActualType->" << propertyDesc.m_name.c_str() << ".IsSet() )\n";
                        file << "                {\n";
                        file << "                    outReferencedResources.emplace_back( pActualType->" << propertyDesc.m_name.c_str() << ".GetResourceID() );\n";
                        file << "                }\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
                else if ( !IsCoreType( propertyDesc.m_typeID ) && !propertyDesc.IsEnumProperty() && !propertyDesc.IsBitFlagsProperty() )
                {
                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #if EE_DEVELOPMENT_TOOLS\n";
                    }

                    if ( propertyDesc.IsArrayProperty() )
                    {
                        if ( propertyDesc.IsDynamicArrayProperty() )
                        {
                            file << "                for ( auto& propertyValue : pActualType->" << propertyDesc.m_name.c_str() << " )\n";
                            file << "                {\n";
                            file << "                    " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetReferencedResources( &propertyValue, outReferencedResources );\n";
                            file << "                }\n";
                        }
                        else // Static array
                        {
                            for ( auto i = 0; i < propertyDesc.m_arraySize; i++ )
                            {
                                file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetReferencedResources( &pActualType->" << propertyDesc.m_name.c_str() << "[" << i << "], outReferencedResources ); \n";
                            }
                        }
                    }
                    else
                    {
                        file << "                " << propertyDesc.m_typeName.c_str() << "::s_pTypeInfo->GetReferencedResources( &pActualType->" << propertyDesc.m_name.c_str() << ", outReferencedResources );\n";
                    }

                    if ( propertyDesc.m_isDevOnly )
                    {
                        file << "                #endif\n";
                    }

                    file << "\n";
                }
            }
        }

        file << "            }\n\n";
    }

    //-------------------------------------------------------------------------
    // Entity Methods
    //-------------------------------------------------------------------------

    static void GenerateLoadMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "\n";
        file << "    void " << type.m_namespace.c_str() << type.m_name.c_str() << "::Load( EntityModel::LoadingContext const& context, Resource::ResourceRequesterID const& requesterID )\n";
        file << "    {\n";

        if ( type.HasProperties() )
        {
            file << "            " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo->LoadResources( context.m_pResourceSystem, requesterID, this );\n";
            file << "            m_status = Status::Loading;\n";
        }
        else
        {
            file << "            m_status = Status::Loaded;\n";
        }

        file << "    }\n";
    }

    static void GenerateUnloadMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "\n";
        file << "    void " << type.m_namespace.c_str() << type.m_name.c_str() << "::Unload( EntityModel::LoadingContext const& context, Resource::ResourceRequesterID const& requesterID )\n";
        file << "    {\n";

        if ( type.HasProperties() )
        {
            file << "        " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo->UnloadResources( context.m_pResourceSystem, requesterID, this );\n";
        }

        file << "        m_status = Status::Unloaded;\n";
        file << "    }\n";
    }

    static void GenerateUpdateLoadingMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "\n";
        file << "    void " << type.m_namespace.c_str() << type.m_name.c_str() << "::UpdateLoading()\n";
        file << "    {\n";
        file << "        if( m_status == Status::Loading )\n";
        file << "        {\n";

        {
            if ( type.HasProperties() )
            {
            // Wait for resources to be loaded
            file << "            auto const resourceLoadingStatus = " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo->GetResourceLoadingStatus( this );\n";
            file << "            if ( resourceLoadingStatus == LoadingStatus::Loading )\n";
            file << "            {\n";
            file << "            return; // Something is still loading so early-out\n";
            file << "            }\n\n";

            // Set status
            file << "            if ( resourceLoadingStatus == LoadingStatus::Failed )\n";
            file << "            {\n";
            file << "            m_status = EntityComponent::Status::LoadingFailed;\n";
            file << "            }\n";
            file << "            else\n";
            file << "            {\n";
            file << "            m_status = EntityComponent::Status::Loaded;\n";
            file << "            }\n";
            }
            else
            {
            file << "            m_status = EntityComponent::Status::Loaded;\n";
            }
        }

        file << "        }\n";
        file << "    }\n";
    }

    //-------------------------------------------------------------------------
    // Type Registration Methods
    //-------------------------------------------------------------------------

    static void GenerateStaticTypeRegistrationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            static void RegisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        file << "            {\n";

        // Create default type instance
        if ( type.IsAbstract() )
        {
            file << "                IReflectedType* pDefaultTypeInstance = nullptr;\n";
        }
        else
        {
            file << "                auto pMemory = EE::Alloc( sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ), alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " ) );\n";
            file << "                IReflectedType* pDefaultTypeInstance = new ( pMemory ) " << type.m_namespace.c_str() << type.m_name.c_str() << "();\n";
        }

        file << "                " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo = EE::New<TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << ">>( pDefaultTypeInstance );\n";
        file << "                typeRegistry.RegisterType( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo );\n";
        file << "            }\n\n";
    }

    static void GenerateStaticTypeUnregistrationMethod( std::stringstream& file, ReflectedType const& type )
    {
        file << "            static void UnregisterType( TypeSystem::TypeRegistry& typeRegistry )\n";
        file << "            {\n";
        file << "                typeRegistry.UnregisterType( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo );\n";

        // Destroy default type instance
        if ( !type.IsAbstract() )
        {
            file << "                EE::Delete( const_cast<IReflectedType*&>( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo->m_pDefaultInstance ) );\n";
        }

        file << "                EE::Delete( " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo );\n";
        file << "            }\n\n";
    }

    static void GenerateTypeInfoConstructor( std::stringstream& file, ReflectedType const& type, ReflectedType const& parentType )
    {
        // The pass by value here is intentional!
        auto GeneratePropertyRegistrationCode = [&file, type] ( ReflectedProperty prop )
        {
            prop.ParseMetaData();

            String const templateSpecializationString = prop.m_templateArgTypeName.empty() ? String() : "<" + prop.m_templateArgTypeName + ">";

            file << "\n";
            file << "                //-------------------------------------------------------------------------\n\n";

            if ( prop.m_isDevOnly )
            {
                file << "                #if EE_DEVELOPMENT_TOOLS\n";
            }

            file << "                propertyInfo.m_ID = StringID( \"" << prop.m_name.c_str() << "\" );\n";
            file << "                propertyInfo.m_typeID = TypeSystem::TypeID( \"" << prop.m_typeName.c_str() << "\" );\n";
            file << "                propertyInfo.m_parentTypeID = " << type.m_ID.ToUint() << ";\n";
            file << "                propertyInfo.m_templateArgumentTypeID = TypeSystem::TypeID( \"" << prop.m_templateArgTypeName.c_str() << "\" );\n\n";

            // Create dev tools info
            file << "                #if EE_DEVELOPMENT_TOOLS\n";
            file << "                propertyInfo.m_friendlyName = \"" << prop.GetFriendlyName().c_str() << "\";\n";
            file << "                propertyInfo.m_category = \"" << prop.GetCategory().c_str() << "\";\n";

            String escapedDescription = prop.m_description;
            StringUtils::ReplaceAllOccurrencesInPlace( escapedDescription, "\"", "\\\"" );
            file << "                propertyInfo.m_description = \"" << escapedDescription.c_str() << "\";\n";

            if ( prop.m_isDevOnly )
            {
                file << "                propertyInfo.m_isForDevelopmentUseOnly = true;\n";
            }
            else
            {
                file << "                propertyInfo.m_isForDevelopmentUseOnly = false;\n";
            }

            if ( prop.m_isToolsReadOnly )
            {
                file << "                propertyInfo.m_isToolsReadOnly = true;\n";
            }
            else
            {
                file << "                propertyInfo.m_isToolsReadOnly = false;\n";
            }

            if ( prop.m_customEditorID.IsValid() )
            {
                file << "                propertyInfo.m_customEditorID = StringID( \"" << prop.m_customEditorID.c_str() << "\" );\n";
            }
            else
            {
                file << "                propertyInfo.m_customEditorID = StringID();\n";
            }

            file << "                #endif\n\n";

            // Abstract types cannot have default values since they cannot be instantiated
            if ( type.IsAbstract() )
            {
                file << "                propertyInfo.m_pDefaultValue = nullptr;\n";
            }
            else
            {
                file << "                propertyInfo.m_pDefaultValue = &pActualDefaultInstance->" << prop.m_name.c_str() << ";\n";

                file << "                propertyInfo.m_offset = offsetof( " << type.m_namespace.c_str() << type.m_name.c_str() << ", " << prop.m_name.c_str() << " );\n";

                if ( prop.IsDynamicArrayProperty() )
                {
                    file << "                propertyInfo.m_pDefaultArrayData = pActualDefaultInstance->" << prop.m_name.c_str() << ".data();\n";
                    file << "                propertyInfo.m_arraySize = (int32_t) pActualDefaultInstance->" << prop.m_name.c_str() << ".size();\n";
                    file << "                propertyInfo.m_arrayElementSize = (int32_t) sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                    file << "                propertyInfo.m_size = sizeof( TVector<" << prop.m_typeName.c_str() << templateSpecializationString.c_str() << "> );\n";
                }
                else if ( prop.IsStaticArrayProperty() )
                {
                    file << "                propertyInfo.m_pDefaultArrayData = pActualDefaultInstance->" << prop.m_name.c_str() << ";\n";
                    file << "                propertyInfo.m_arraySize = " << prop.GetArraySize() << ";\n";
                    file << "                propertyInfo.m_arrayElementSize = (int32_t) sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                    file << "                propertyInfo.m_size = sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " ) * " << prop.GetArraySize() << ";\n";
                }
                else
                {
                    file << "                propertyInfo.m_size = sizeof( " << prop.m_typeName.c_str() << templateSpecializationString.c_str() << " );\n";
                }

                file << "                propertyInfo.m_flags.Set( " << prop.m_flags << " );\n";
                file << "                m_properties.emplace_back( propertyInfo );\n";
                file << "                m_propertyMap.insert( TPair<StringID, int32_t>( propertyInfo.m_ID, int32_t( m_properties.size() ) - 1 ) );\n";
            }

            if ( prop.m_isDevOnly )
            {
                file << "            #endif\n";
            }
        };

        //-------------------------------------------------------------------------

        file << "            TTypeInfo( IReflectedType const* pDefaultInstance )\n";
        file << "            {\n";

        // Create type info
        file << "                m_ID = TypeSystem::TypeID( \"" << type.m_namespace.c_str() << type.m_name.c_str() << "\" );\n";
        file << "                m_size = sizeof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";
        file << "                m_alignment = alignof( " << type.m_namespace.c_str() << type.m_name.c_str() << " );\n";

        // Set default instance
        file << "                m_pDefaultInstance = pDefaultInstance;\n";

        // Add type metadata
        if ( type.IsAbstract() )
        {
            file << "                m_isAbstract = true;\n";
        }

        file << "\n";

        // Create dev tools info
        file << "                #if EE_DEVELOPMENT_TOOLS\n";
        file << "                m_friendlyName = \"" << type.GetFriendlyName().c_str() << "\";\n";
        file << "                m_category = \"" << type.GetCategory().c_str() << "\";\n";

        if ( type.m_isDevOnly )
        {
            file << "                m_isForDevelopmentUseOnly = true;\n";
        }

        file << "                #endif\n\n";

        // Add parent info
        file << "                // Parent types\n";
        file << "                //-------------------------------------------------------------------------\n\n";
        file << "                m_pParentTypeInfo = " << parentType.m_namespace.c_str() << parentType.m_name.c_str() << "::s_pTypeInfo;\n";

        // Add properties
        if ( type.HasProperties() )
        {
            file << "\n";
            file << "                // Register properties and type\n";
            file << "                //-------------------------------------------------------------------------\n\n";

            if ( !type.IsAbstract() )
            {
                file << "                auto pActualDefaultInstance = reinterpret_cast<" << type.m_namespace.c_str() << type.m_name.c_str() << " const*>( m_pDefaultInstance );\n";
            }
            
            file << "                PropertyInfo propertyInfo;\n";

            for ( auto& prop : type.m_properties )
            {
                GeneratePropertyRegistrationCode( prop );
            }
        }

        file << "            }\n\n";
    }

    //-------------------------------------------------------------------------
    // File generation
    //-------------------------------------------------------------------------

    static void GenerateTypeInfoFile( std::stringstream& file, ReflectionDatabase const& database, String const& exportMacro, ReflectedType const& type, ReflectedType const& parentType )
    {
        // Dev Flag
        if ( type.m_isDevOnly )
        {
            file << "#if EE_DEVELOPMENT_TOOLS\n";
        }

        // Type Info
        //-------------------------------------------------------------------------

        file << "\n";
        file << "//-------------------------------------------------------------------------\n";
        file << "// TypeInfo: " << type.m_namespace.c_str() << type.m_name.c_str() << "\n";
        file << "//-------------------------------------------------------------------------\n\n";

        file << "namespace EE\n";
        file << "{\n";

        // Define static type info for type
        file << "    TypeSystem::TypeInfo const* " << type.m_namespace.c_str() << type.m_name.c_str() << "::s_pTypeInfo = nullptr;\n\n";

        file << "    namespace TypeSystem\n";
        file << "    {\n";
        file << "        template<>\n";
        file << "        class " << exportMacro.c_str() << " TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << "> final : public TypeInfo\n";
        file << "        {\n";

        file << "           static " << type.m_namespace.c_str() << type.m_name.c_str() <<" const* s_pDefaultInstance_" << type.m_ID.ToUint() << ";\n\n";

        file << "        public:\n\n";

        GenerateStaticTypeRegistrationMethod( file, type );
        GenerateStaticTypeUnregistrationMethod( file, type );

        file << "        public:\n\n";

        GenerateTypeInfoConstructor( file, type, parentType );
        GenerateCreationMethod( file, type );
        GenerateInPlaceCreationMethod( file, type );
        GenerateLoadResourcesMethod( database, file, type );
        GenerateUnloadResourcesMethod( database, file, type );
        GenerateResourceLoadingStatusMethod( database, file, type );
        GenerateResourceUnloadingStatusMethod( database, file, type );
        GenerateGetReferencedResourceMethod( database, file, type );
        GenerateExpectedResourceTypeMethod( file, type );
        GenerateArrayAccessorMethod( file, type );
        GenerateArraySizeMethod( file, type );
        GenerateArrayElementSizeMethod( file, type );
        GenerateArrayClearMethod( file, type );
        GenerateAddArrayElementMethod( file, type );
        GenerateInsertArrayElementMethod( file, type );
        GenerateMoveArrayElementMethod( file, type );
        GenerateRemoveArrayElementMethod( file, type );
        GenerateAreAllPropertiesEqualMethod( file, type );
        GenerateIsPropertyEqualMethod( file, type );
        GenerateSetToDefaultValueMethod( file, type );

        file << "        };\n\n";

        file << "        " << type.m_namespace.c_str() << type.m_name.c_str() << " const*" << " TTypeInfo<" << type.m_namespace.c_str() << type.m_name.c_str() << ">::s_pDefaultInstance_" << type.m_ID.ToUint() << " = nullptr; \n";

        file << "    }\n";

        // Generate entity component methods
        //-------------------------------------------------------------------------

        if ( type.IsEntityComponent() )
        {
            file << "    //-------------------------------------------------------------------------\n";
            file << "    // Entity Component: " << type.m_namespace.c_str() << type.m_name.c_str() << "\n";
            file << "    //-------------------------------------------------------------------------\n\n";

            GenerateLoadMethod( file, type );
            GenerateUnloadMethod( file, type );
            GenerateUpdateLoadingMethod( file, type );
        }

        file << "}\n";

        // Dev Flag
        //-------------------------------------------------------------------------

        if ( type.m_isDevOnly )
        {
            file << "#endif\n";
        }
    }

    //-------------------------------------------------------------------------

    namespace TypeGenerator
    {
        void Generate( ReflectionDatabase const& database, std::stringstream& codeFile, String const& exportMacro, ReflectedType const& type, ReflectedType const& parentType )
        {
            GenerateTypeInfoFile( codeFile, database, exportMacro, type, parentType );
        }
    }
}