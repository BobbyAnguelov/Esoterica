#include "TypeDescriptors.h"
#include "TypeRegistry.h"
#include "TypeInstance.h"
#include "Base/Math/Math.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    // Resolved property path
    //-------------------------------------------------------------------------
    // Resolves a given property path with a given type instance to calculate the final address of the property this path refers to

    namespace
    {
        struct ResolvedPropertyPathElement
        {
            inline bool IsDynamicArray() const { return m_pPropertyInfo->IsDynamicArrayProperty() && m_arrayElementIdx == InvalidIndex; }
            inline bool IsArrayElement() const { return m_pPropertyInfo->IsArrayProperty() && m_arrayElementIdx != InvalidIndex; }
            inline bool IsTypeInstance() const { return m_pPropertyInfo->IsTypeInstanceProperty(); }

        public:

            StringID                m_propertyID;
            int32_t                 m_arrayElementIdx;
            PropertyInfo const*     m_pPropertyInfo;
            IReflectedType*         m_pParentInstance = nullptr;
            uint8_t*                m_pPropertyAddress;
        };

        struct ResolvedPropertyPath
        {
            inline bool IsValid() const { return !m_pathElements.empty(); }
            inline void Reset() { m_pathElements.clear(); }

            TInlineVector<ResolvedPropertyPathElement, 6> m_pathElements;
        };

        static ResolvedPropertyPath ResolvePropertyPath( TypeRegistry const& typeRegistry, IReflectedType* const pTypeInstance, PropertyPath const& path )
        {
            ResolvedPropertyPath resolvedPath;

            IReflectedType* pParentInstance = pTypeInstance;

            // Resolve property path
            size_t const numPathElements = path.GetNumElements();
            for ( size_t i = 0; i < numPathElements; i++ )
            {
                // Get the typeinfo for the current parent
                TypeInfo const* const pParentTypeInfo = pParentInstance->GetTypeInfo();
                EE_ASSERT( pParentTypeInfo != nullptr );

                // Get the property info for the path element
                PropertyInfo const* pFoundPropertyInfo = pParentTypeInfo->GetPropertyInfo( path[i].m_propertyID );
                if ( pFoundPropertyInfo == nullptr )
                {
                    resolvedPath.Reset();
                    break;
                }

                // Create resolved element and set property address
                //-------------------------------------------------------------------------

                ResolvedPropertyPathElement& resolvedPathElement = resolvedPath.m_pathElements.emplace_back();
                resolvedPathElement.m_propertyID = path[i].m_propertyID;
                resolvedPathElement.m_arrayElementIdx = path[i].m_arrayElementIdx;
                resolvedPathElement.m_pPropertyInfo = pFoundPropertyInfo;
                resolvedPathElement.m_pParentInstance = pParentInstance;

                bool const isArrayProperty = pFoundPropertyInfo->IsArrayProperty();
                bool const isArrayElementPath = path[i].m_arrayElementIdx != InvalidIndex;

                // Calculate the address of the resolved property
                if ( isArrayProperty && isArrayElementPath )
                {
                    resolvedPathElement.m_pPropertyAddress = pParentTypeInfo->GetArrayElementDataPtr( pParentInstance, path[i].m_propertyID.ToUint(), path[i].m_arrayElementIdx );
                }
                else // Set address to type using the recorded offset
                {
                    resolvedPathElement.m_pPropertyAddress = reinterpret_cast<uint8_t*>( pParentInstance ) + pFoundPropertyInfo->m_offset;
                }

                // Handle Structures and Type Instances
                //-------------------------------------------------------------------------

                if ( !isArrayProperty || isArrayElementPath )
                {
                    if ( pFoundPropertyInfo->IsTypeInstanceProperty() )
                    {
                        auto pInstanceContainer = (TypeInstance*) resolvedPathElement.m_pPropertyAddress;
                        if ( pInstanceContainer->IsSet() )
                        {
                            pParentInstance = pInstanceContainer->Get();
                        }
                        else if ( i != ( numPathElements - 1 ) )
                        {
                            resolvedPath.Reset();
                            break;
                        }
                    }
                    else if ( resolvedPathElement.m_pPropertyInfo->IsStructureProperty() )
                    {
                        pParentInstance = reinterpret_cast<IReflectedType*>( resolvedPathElement.m_pPropertyAddress );
                    }
                }
            }

            return resolvedPath;
        }
    }

    // Type Description
    //-------------------------------------------------------------------------

    namespace
    {
        struct TypeDescriber
        {
            static void DescribeType( TypeRegistry const& typeRegistry, TypeDescriptor& typeDesc, TypeID typeID, IReflectedType const* pTypeInstance, PropertyPath& path )
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
                        int32_t const numArrayElements =  propInfo.IsStaticArrayProperty() ? (int32_t) propInfo.m_arraySize : (int32_t) pTypeInfo->GetArraySize( pTypeInstance, propInfo.m_ID.ToUint() );
                        int32_t const numArrayElementsDefault = propInfo.IsStaticArrayProperty() ? (int32_t) propInfo.m_arraySize : (int32_t) pTypeInfo->GetArraySize( pTypeInfo->GetDefaultInstance(), propInfo.m_ID.ToUint() );

                        // Describe array size
                        if ( propInfo.IsDynamicArrayProperty() && ( numArrayElements != numArrayElementsDefault ) )
                        {
                            PropertyDescriptor& propertyDesc = typeDesc.m_properties.emplace_back( PropertyDescriptor() );
                            propertyDesc.m_path = path;
                            propertyDesc.m_path.Append( propInfo.m_ID );
                            TypeSystem::Conversion::ConvertNativeTypeToString( typeRegistry, GetCoreTypeID( CoreTypeID::Int32 ), TypeID(), &numArrayElements, propertyDesc.m_stringValue );
                            TypeSystem::Conversion::ConvertNativeTypeToBinary( typeRegistry, GetCoreTypeID( CoreTypeID::Int32 ), TypeID(), &numArrayElements, propertyDesc.m_byteValue );
                        }

                        // Describe array elements
                        if ( numArrayElements > 0 )
                        {
                            uint64_t const arrayID = propInfo.m_ID.ToUint();
                            uint8_t const* pArrayElementAddress = pTypeInfo->GetArrayElementDataPtr( const_cast<IReflectedType*>( pTypeInstance ), arrayID, 0 );
                            for ( int32_t i = 0; i < numArrayElements; i++ )
                            {
                                path.Append( propInfo.m_ID, i );
                                DescribeProperty( typeRegistry, typeDesc, pTypeInfo, pTypeInstance, propInfo, pArrayElementAddress, path, i );
                                pArrayElementAddress += elementByteSize;
                                path.RemoveLastElement();
                            }
                        }
                    }
                    else // Regular property
                    {
                        path.Append( propInfo.m_ID );
                        auto pPropertyInstance = propInfo.GetPropertyAddress( pTypeInstance );
                        DescribeProperty( typeRegistry, typeDesc, pTypeInfo, pTypeInstance, propInfo, pPropertyInstance, path );
                        path.RemoveLastElement();
                    }
                }
            }

            static void DescribeProperty( TypeRegistry const& typeRegistry, TypeDescriptor& typeDesc, TypeInfo const* pParentTypeInfo, IReflectedType const* pParentInstance, PropertyInfo const& propertyInfo, void const* pPropertyInstance, PropertyPath& path, int32_t arrayElementIdx = InvalidIndex )
            {
                if ( propertyInfo.IsTypeInstanceProperty() )
                {
                    auto pInstanceContainer = (TypeInstance const*) pPropertyInstance;
                    if ( !pParentTypeInfo->IsPropertyValueSetToDefault( pParentInstance, propertyInfo.m_ID.ToUint(), arrayElementIdx ) )
                    {
                        PropertyDescriptor& propertyDesc = typeDesc.m_properties.emplace_back( PropertyDescriptor() );
                        propertyDesc.m_path = path;

                        TypeID instanceTypeID = pInstanceContainer->GetInstanceTypeID();
                        if ( instanceTypeID.IsValid() )
                        {
                            propertyDesc.m_stringValue = instanceTypeID.c_str();
                        }

                        TypeSystem::Conversion::ConvertNativeTypeToBinary( typeRegistry, GetCoreTypeID( CoreTypeID::TypeID ), TypeID(), &instanceTypeID, propertyDesc.m_byteValue );

                        if ( pInstanceContainer->IsSet() )
                        {
                            DescribeType( typeRegistry, typeDesc, instanceTypeID, pInstanceContainer->Get(), path );
                        }
                    }
                }
                else if ( IsCoreType( propertyInfo.m_typeID ) || propertyInfo.IsEnumProperty() )
                {
                    if ( !pParentTypeInfo->IsPropertyValueSetToDefault( pParentInstance, propertyInfo.m_ID.ToUint(), arrayElementIdx ) )
                    {
                        PropertyDescriptor& propertyDesc = typeDesc.m_properties.emplace_back( PropertyDescriptor() );
                        propertyDesc.m_path = path;
                        Conversion::ConvertNativeTypeToBinary( typeRegistry, propertyInfo, pPropertyInstance, propertyDesc.m_byteValue );
                        Conversion::ConvertNativeTypeToString( typeRegistry, propertyInfo, pPropertyInstance, propertyDesc.m_stringValue );
                    }
                }
                else
                {
                    DescribeType( typeRegistry, typeDesc, propertyInfo.m_typeID, (IReflectedType*) pPropertyInstance, path );
                }
            }

            //-------------------------------------------------------------------------

            static void GetAllReferencedResources( TypeRegistry const& typeRegistry, TypeID typeID, IReflectedType const* pTypeInstance, TVector<ResourceID>& outReferencedResources )
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
                        uint64_t const arrayID = propInfo.m_ID.ToUint();
                        size_t const numArrayElements = propInfo.IsStaticArrayProperty() ? propInfo.m_arraySize : pTypeInfo->GetArraySize( pTypeInstance, propInfo.m_ID.ToUint() );
                        if ( numArrayElements > 0 )
                        {
                            uint8_t const* pArrayElementAddress = pTypeInfo->GetArrayElementDataPtr( const_cast<IReflectedType*>( pTypeInstance ), arrayID, 0 );
                            for ( auto i = 0u; i < numArrayElements; i++ )
                            {
                                GetAllReferencedResourcesFromProperty( typeRegistry, pTypeInfo, pTypeInstance, propInfo, pArrayElementAddress, outReferencedResources, i );
                                pArrayElementAddress += elementByteSize;
                            }
                        }
                    }
                    else
                    {
                        auto pPropertyInstance = propInfo.GetPropertyAddress( pTypeInstance );
                        GetAllReferencedResourcesFromProperty( typeRegistry, pTypeInfo, pTypeInstance, propInfo, pPropertyInstance, outReferencedResources );
                    }
                }
            }

            static void GetAllReferencedResourcesFromProperty( TypeRegistry const& typeRegistry, TypeInfo const* pParentTypeInfo, IReflectedType const* pParentInstance, PropertyInfo const& propertyInfo, void const* pPropertyInstance, TVector<ResourceID>& outReferencedResources, int32_t arrayElementIdx = InvalidIndex )
            {
                if ( propertyInfo.IsTypeInstanceProperty() )
                {
                    auto pInstanceContainer = (TypeInstance const*) pPropertyInstance;
                    if ( pInstanceContainer->IsSet() )
                    {
                        GetAllReferencedResources( typeRegistry, pInstanceContainer->GetInstanceTypeInfo()->m_ID, pInstanceContainer->Get(), outReferencedResources );
                    }
                }
                else if ( IsCoreType( propertyInfo.m_typeID ) || propertyInfo.IsEnumProperty() )
                {
                    if ( propertyInfo.IsResourcePtrProperty() )
                    {
                        auto pResourcePtr = reinterpret_cast<Resource::ResourcePtr const*>( pPropertyInstance );
                        if ( pResourcePtr->IsSet() )
                        {
                            ResourceID const resID( pResourcePtr->GetResourceID() );
                            if ( !VectorContains( outReferencedResources, resID ) )
                            {
                                outReferencedResources.emplace_back( resID );
                            }
                        }
                    }
                }
                else
                {
                    GetAllReferencedResources( typeRegistry, propertyInfo.m_typeID, (IReflectedType*) pPropertyInstance, outReferencedResources );
                }
            }
        };
    }

    //-------------------------------------------------------------------------

    TypeDescriptor::TypeDescriptor( TypeRegistry const& typeRegistry, IReflectedType* pTypeInstance )
    {
        EE_ASSERT( pTypeInstance != nullptr );
        DescribeType( typeRegistry, pTypeInstance, *this );
    }

    void TypeDescriptor::DescribeType( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, TypeDescriptor& outDesc )
    {
        // Reset descriptor
        outDesc.m_typeID = pTypeInstance->GetTypeID();
        outDesc.m_properties.clear();

        // Fill property values
        PropertyPath path;
        TypeDescriber::DescribeType( typeRegistry, outDesc, outDesc.m_typeID, pTypeInstance, path );
    }

    void TypeDescriptor::GetAllReferencedResources( TypeRegistry const& typeRegistry, IReflectedType const* pTypeInstance, TVector<ResourceID>& outReferencedResources )
    {
        TypeDescriber::GetAllReferencedResources( typeRegistry, pTypeInstance->GetTypeInfo()->m_ID, pTypeInstance, outReferencedResources );
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

    void* TypeDescriptor::RestorePropertyState( TypeRegistry const& typeRegistry, TypeInfo const* pTypeInfo, IReflectedType* pTypeInstance ) const
    {
        EE_ASSERT( pTypeInfo != nullptr );
        EE_ASSERT( IsValid() && pTypeInfo->m_ID == m_typeID );

        // Restore property state
        //-------------------------------------------------------------------------

        for ( auto const& propertyDesc : m_properties )
        {
            EE_ASSERT( propertyDesc.IsValid() );

            // Resolve a property path for a given instance
            auto resolvedPath = ResolvePropertyPath( typeRegistry, pTypeInstance, propertyDesc.m_path );
            if ( !resolvedPath.IsValid() )
            {
                EE_LOG_ERROR( "TypeSystem", "Type Descriptor", "Tried to set the value for an invalid property (%s) for type (%s)", propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.ToStringID().c_str() );
                continue;
            }

            //-------------------------------------------------------------------------

            ResolvedPropertyPathElement const& resolvedProperty = resolvedPath.m_pathElements.back();

            if ( resolvedProperty.IsDynamicArray() && !resolvedProperty.IsArrayElement() )
            {
                int32_t numElements = 0;
                if ( TypeSystem::Conversion::ConvertBinaryToNativeType( typeRegistry, GetCoreTypeID( CoreTypeID::Int32 ), TypeID(), propertyDesc.m_byteValue, &numElements ) )
                {
                    EE_ASSERT( numElements >= 0 && numElements < 100000 );
                    auto pParentTypeInfo = typeRegistry.GetTypeInfo( resolvedProperty.m_pPropertyInfo->m_parentTypeID );
                    EE_ASSERT( pParentTypeInfo != nullptr );
                    pParentTypeInfo->SetArraySize( resolvedProperty.m_pParentInstance, resolvedProperty.m_pPropertyInfo->m_ID.ToUint(), numElements );
                }
                else
                {
                    EE_LOG_ERROR( "TypeSystem", "Type Descriptor", "Failed to convert array size value for property (%s) on type: %s", propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.ToStringID().c_str() );
                }
            }
            else if ( resolvedProperty.IsTypeInstance() )
            {
                TypeID instanceTypeID;
                if ( TypeSystem::Conversion::ConvertBinaryToNativeType( typeRegistry, GetCoreTypeID( CoreTypeID::TypeID ), TypeID(), propertyDesc.m_byteValue, &instanceTypeID ) )
                {
                    auto pInstanceContainer = (TypeInstance*) resolvedProperty.m_pPropertyAddress;

                    if ( instanceTypeID.IsValid() )
                    {
                        TypeInfo const* pInstanceTypeInfo = typeRegistry.GetTypeInfo( instanceTypeID );
                        if ( pInstanceTypeInfo == nullptr )
                        {
                            EE_LOG_ERROR( "TypeSystem", "Type Descriptor", "Invalid instance type (%s) for an property (%s) for type (%s)", instanceTypeID.c_str(), propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.ToStringID().c_str() );
                            continue;
                        }

                        pInstanceContainer->CreateInstance( pInstanceTypeInfo );
                    }
                    else
                    {
                        pInstanceContainer->DestroyInstance();
                    }
                }
                else
                {
                    EE_LOG_ERROR( "TypeSystem", "Type Descriptor", "Failed to read type instance ID value for property (%s) on type: %s", propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.ToStringID().c_str() );
                }
            }
            else // Set actual property value
            {
                if ( !Conversion::ConvertBinaryToNativeType( typeRegistry, *resolvedProperty.m_pPropertyInfo, propertyDesc.m_byteValue, resolvedProperty.m_pPropertyAddress ) )
                {
                    EE_LOG_ERROR( "TypeSystem", "Type Descriptor", "Failed to convert property value for property (%s) on type: %s", propertyDesc.m_path.ToString().c_str(), pTypeInfo->m_ID.ToStringID().c_str() );
                }
            }
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