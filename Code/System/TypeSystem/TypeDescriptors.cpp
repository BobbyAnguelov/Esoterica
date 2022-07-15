#include "TypeDescriptors.h"
#include "TypeRegistry.h"
#include "System/Math/Math.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    namespace
    {
        struct ResolvedPathElement
        {
            StringID                m_propertyID;
            int32_t                 m_arrayElementIdx;
            PropertyInfo const*     m_pPropertyInfo;
            uint8_t*                m_pAddress;
        };

        struct ResolvedPath
        {
            inline bool IsValid() const { return !m_pathElements.empty(); }
            inline void Reset() { m_pathElements.clear(); }

            TInlineVector<ResolvedPathElement, 6> m_pathElements;
        };

        // Resolves a given property path with a given type instance to calculate the final address of the property this path refers to
        static ResolvedPath ResolvePropertyPath( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, uint8_t* const pTypeInstanceAddress, PropertyPath const& path )
        {
            ResolvedPath resolvedPath;

            uint8_t* pResolvedTypeInstance = pTypeInstanceAddress;
            TypeInfo const* pResolvedTypeInfo = pTypeInfo;
            PropertyInfo const* pFoundPropertyInfo = nullptr;

            // Resolve property path
            size_t const numPathElements = path.GetNumElements();
            size_t const lastElementIdx = numPathElements - 1;
            for ( size_t i = 0; i < numPathElements; i++ )
            {
                EE_ASSERT( pResolvedTypeInfo != nullptr );

                // Get the property info for the path element
                pFoundPropertyInfo = pResolvedTypeInfo->GetPropertyInfo( path[i].m_propertyID );
                if ( pFoundPropertyInfo == nullptr )
                {
                    resolvedPath.Reset();
                    break;
                }

                ResolvedPathElement& resolvedPathElement = resolvedPath.m_pathElements.emplace_back();
                resolvedPathElement.m_propertyID = path[i].m_propertyID;
                resolvedPathElement.m_arrayElementIdx = path[i].m_arrayElementIdx;
                resolvedPathElement.m_pPropertyInfo = pFoundPropertyInfo;

                // Calculate the address of the resolved property
                if ( pFoundPropertyInfo->IsArrayProperty() )
                {
                    resolvedPathElement.m_pAddress = pResolvedTypeInfo->GetArrayElementDataPtr( reinterpret_cast<IRegisteredType*>( pResolvedTypeInstance ), path[i].m_propertyID, path[i].m_arrayElementIdx );
                }
                else // Structure/Type
                {
                    resolvedPathElement.m_pAddress = pResolvedTypeInstance + pFoundPropertyInfo->m_offset;
                }

                // Update the resolved type instance and type info
                pResolvedTypeInstance = resolvedPathElement.m_pAddress;
                if ( !IsCoreType( resolvedPathElement.m_pPropertyInfo->m_typeID ) )
                {
                    pResolvedTypeInfo = typeRegistry.GetTypeInfo( resolvedPathElement.m_pPropertyInfo->m_typeID );
                }
                else
                {
                    pResolvedTypeInfo = nullptr;
                }
            }

            return resolvedPath;
        }
    }

    //-------------------------------------------------------------------------

    namespace
    {
        struct TypeDescriber
        {
            static void DescribeType( TypeRegistry const& typeRegistry, TypeDescriptor& typeDesc, TypeID typeID, IRegisteredType const* pTypeInstance, PropertyPath& path, bool shouldSetPropertyStringValues )
            {
                EE_ASSERT( !IsCoreType( typeID ) );
                auto const pTypeInfo = typeRegistry.GetTypeInfo( typeID );
                EE_ASSERT( pTypeInfo != nullptr );

                //-------------------------------------------------------------------------

                for ( auto const& propInfo : pTypeInfo->m_properties )
                {
                    if ( propInfo.IsArrayProperty() )
                    {
                        size_t const elementByteSize = propInfo.m_arrayElementSize;
                        size_t const numArrayElements = propInfo.IsStaticArrayProperty() ? propInfo.m_arraySize : pTypeInfo->GetArraySize( pTypeInstance, propInfo.m_ID );
                        if ( numArrayElements > 0 )
                        {
                            uint8_t const* pArrayElementAddress = pTypeInfo->GetArrayElementDataPtr( const_cast<IRegisteredType*>( pTypeInstance ), propInfo.m_ID, 0 );

                            // Write array elements
                            for ( auto i = 0; i < numArrayElements; i++ )
                            {
                                path.Append( propInfo.m_ID, i );
                                DescribeProperty( typeRegistry, typeDesc, pTypeInfo, pTypeInstance, propInfo, shouldSetPropertyStringValues, pArrayElementAddress, path, i );
                                pArrayElementAddress += elementByteSize;
                                path.RemoveLastElement();
                            }
                        }
                    }
                    else
                    {
                        path.Append( propInfo.m_ID );
                        auto pPropertyInstance = propInfo.GetPropertyAddress( pTypeInstance );
                        DescribeProperty( typeRegistry, typeDesc, pTypeInfo, pTypeInstance, propInfo, shouldSetPropertyStringValues, pPropertyInstance, path );
                        path.RemoveLastElement();
                    }
                }
            }

            static void DescribeProperty( TypeRegistry const& typeRegistry, TypeDescriptor& typeDesc, TypeInfo const* pParentTypeInfo, IRegisteredType const* pParentInstance, PropertyInfo const& propertyInfo, bool shouldSetPropertyStringValues, void const* pPropertyInstance, PropertyPath& path, int32_t arrayElementIdx = InvalidIndex )
            {
                if ( IsCoreType( propertyInfo.m_typeID ) || propertyInfo.IsEnumProperty() )
                {
                    // Only describe non-default properties
                    if ( !pParentTypeInfo->IsPropertyValueSetToDefault( pParentInstance, propertyInfo.m_ID, arrayElementIdx ) )
                    {
                        PropertyDescriptor& propertyDesc = typeDesc.m_properties.emplace_back( PropertyDescriptor() );
                        propertyDesc.m_path = path;
                        Conversion::ConvertNativeTypeToBinary( typeRegistry, propertyInfo, pPropertyInstance, propertyDesc.m_byteValue );

                        #if EE_DEVELOPMENT_TOOLS
                        if ( shouldSetPropertyStringValues )
                        {
                            Conversion::ConvertNativeTypeToString( typeRegistry, propertyInfo, pPropertyInstance, propertyDesc.m_stringValue );
                        }
                        #endif
                    }
                }
                else
                {
                    DescribeType( typeRegistry, typeDesc, propertyInfo.m_typeID, (IRegisteredType*) pPropertyInstance, path, shouldSetPropertyStringValues );
                }
            }
        };
    }

    //-------------------------------------------------------------------------

    TypeDescriptor::TypeDescriptor( TypeRegistry const& typeRegistry, IRegisteredType* pTypeInstance, bool shouldSetPropertyStringValues )
    {
        EE_ASSERT( pTypeInstance != nullptr );
        DescribeTypeInstance( typeRegistry, pTypeInstance, shouldSetPropertyStringValues );
    }

    void TypeDescriptor::DescribeTypeInstance( TypeRegistry const& typeRegistry, IRegisteredType* pTypeInstance, bool shouldSetPropertyStringValues )
    {
        // Reset descriptor
        m_typeID = pTypeInstance->GetTypeID();
        m_properties.clear();

        // Fill property values
        PropertyPath path;
        TypeDescriber::DescribeType( typeRegistry, *this, m_typeID, pTypeInstance, path, shouldSetPropertyStringValues );
    }

    PropertyDescriptor* TypeDescriptor::GetProperty( PropertyPath const& path )
    {
        for ( auto& prop : m_properties )
        {
            if ( prop.m_path == path )
            {
                return &prop;
            }
        }

        return nullptr;
    }

    void TypeDescriptor::RemovePropertyValue( TypeSystem::PropertyPath const& path )
    {
        for ( int32_t i = (int32_t) m_properties.size() - 1; i >= 0; i-- )
        {
            if ( m_properties[i].m_path == path )
            {
                m_properties.erase_unsorted( m_properties.begin() + i );
            }
        }
    }

    void* TypeDescriptor::SetPropertyValues( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, void* pTypeInstance ) const
    {
        EE_ASSERT( pTypeInfo != nullptr );
        EE_ASSERT( IsValid() && pTypeInfo->m_ID == m_typeID );

        for ( auto const& propertyValue : m_properties )
        {
            EE_ASSERT( propertyValue.IsValid() );

            // Resolve a property path for a given instance
            auto resolvedPath = ResolvePropertyPath( typeRegistry, pTypeInfo, (uint8_t*) pTypeInstance, propertyValue.m_path );
            if ( !resolvedPath.IsValid() )
            {
                EE_LOG_ERROR( "TypeSystem", "Tried to set the value for an invalid property (%s) for type (%s)", propertyValue.m_path.ToString().c_str(), pTypeInfo->m_ID.ToStringID().c_str() );
                continue;
            }

            // Set actual property value
            auto const& resolvedProperty = resolvedPath.m_pathElements.back();
            Conversion::ConvertBinaryToNativeType( typeRegistry, *resolvedProperty.m_pPropertyInfo, propertyValue.m_byteValue, resolvedProperty.m_pAddress );
        }

        return pTypeInstance;
    }

    //-------------------------------------------------------------------------

    void TypeDescriptorCollection::Reset()
    {
        m_descriptors.clear();
        m_totalRequiredSize = -1;
        m_requiredAlignment = -1;
        m_typeInfos.clear();
        m_typeSizes.clear();
        m_typePaddings.clear();
    }

    void TypeDescriptorCollection::CalculateCollectionRequirements( TypeRegistry const& typeRegistry )
    {
        m_totalRequiredSize = 0;
        m_requiredAlignment = 0;

        m_typeInfos.clear();
        m_typeSizes.clear();
        m_typePaddings.clear();

        int32_t const numDescs = (int32_t) m_descriptors.size();
        if ( numDescs == 0 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_typeInfos.reserve( numDescs );
        m_typeSizes.reserve( numDescs );
        m_typePaddings.reserve( numDescs );

        uintptr_t predictedMemoryOffset = 0;

        for ( auto const& typeDesc : m_descriptors )
        {
            auto pTypeInfo = typeRegistry.GetTypeInfo( typeDesc.m_typeID );
            EE_ASSERT( pTypeInfo != nullptr );
            EE_ASSERT( pTypeInfo->m_size > 0 && pTypeInfo->m_alignment > 0 );

            // Update overall alignment requirements and the required padding
            m_requiredAlignment = Math::Max( m_requiredAlignment, pTypeInfo->m_alignment );
            size_t const requiredPadding = Memory::CalculatePaddingForAlignment( predictedMemoryOffset, pTypeInfo->m_alignment );
            predictedMemoryOffset += (uintptr_t) pTypeInfo->m_size + requiredPadding;

            m_typeInfos.emplace_back( pTypeInfo );
            m_typeSizes.emplace_back( pTypeInfo->m_size );
            m_typePaddings.emplace_back( (uint32_t) requiredPadding );
        }

        m_totalRequiredSize = (uint32_t) predictedMemoryOffset;
    }
}