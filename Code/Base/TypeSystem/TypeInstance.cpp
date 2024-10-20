#include "TypeInstance.h"
#include "Base/Memory/Memory.h"
#include "TypeInfo.h"

//-------------------------------------------------------------------------

namespace EE
{
    TypeInstance::TypeInstance( IReflectedType* pInstance )
        : m_pInstance( pInstance )
    {}

    TypeInstance::TypeInstance( TypeInstance const& rhs )
    {
        m_pInstance = rhs.GetInstanceTypeInfo()->CreateType();
        m_pInstance->GetTypeInfo()->CopyProperties( m_pInstance, rhs.m_pInstance );
    }

    TypeInstance::TypeInstance( TypeInstance&& rhs )
        : m_pInstance( rhs.m_pInstance )
    {
        rhs.m_pInstance = nullptr;
    }

    TypeInstance::~TypeInstance()
    {
        EE::Delete( m_pInstance );
    }

    TypeInstance& TypeInstance::CopyFrom( TypeInstance const& rhs )
    {
        if ( rhs.IsSet() )
        {
            // Recreate the instance if needed
            if ( m_pInstance == nullptr || m_pInstance->GetTypeInfo() != rhs.GetInstanceTypeInfo() )
            {
                EE::Delete( m_pInstance );
                m_pInstance = rhs.GetInstanceTypeInfo()->CreateType();
            }

            m_pInstance->GetTypeInfo()->CopyProperties( m_pInstance, rhs.m_pInstance );
        }
        else // Just destroy the instance
        {
            EE::Delete( m_pInstance );
        }

        return *this;
    }

    TypeInstance& TypeInstance::CopyFrom( IReflectedType const* pTypeInstance )
    {
        if ( pTypeInstance != nullptr )
        {
            // Recreate the instance if needed
            if ( m_pInstance == nullptr || m_pInstance->GetTypeInfo() != pTypeInstance->GetTypeInfo() )
            {
                EE::Delete( m_pInstance );
                m_pInstance = pTypeInstance->GetTypeInfo()->CreateType();
            }

            m_pInstance->GetTypeInfo()->CopyProperties( m_pInstance, pTypeInstance );
        }
        else // Just destroy the instance
        {
            EE::Delete( m_pInstance );
        }

        return *this;
    }

    TypeInstance& TypeInstance::operator=( TypeInstance&& rhs )
    {
        EE::Delete( m_pInstance );
        m_pInstance = rhs.m_pInstance;
        rhs.m_pInstance = nullptr;
        return *this;
    }

    TypeSystem::TypeID TypeInstance::GetInstanceTypeID() const
    {
        return ( m_pInstance != nullptr ) ? m_pInstance->GetTypeInfo()->m_ID : TypeSystem::TypeID();
    }

    TypeSystem::TypeInfo const* TypeInstance::GetInstanceTypeInfo() const
    {
        return ( m_pInstance != nullptr ) ? m_pInstance->GetTypeInfo() : nullptr;
    }

    TypeSystem::TypeInfo const* TypeInstance::GetAllowedBaseTypeInfo() const
    {
        return IReflectedType::s_pTypeInfo;
    }

    void TypeInstance::SetInstance( IReflectedType* pNewInstance )
    {
        EE::Delete( m_pInstance );
        m_pInstance = pNewInstance;
    }

    bool TypeInstance::AreTypesAndPropertyValuesEqual( TypeInstance const& rhs ) const
    {
        if ( rhs.m_pInstance == m_pInstance )
        {
            return true;
        }

        // If both are null, this is equivalent
        if ( m_pInstance == nullptr && rhs.m_pInstance == nullptr )
        {
            return true;
        }

        // Neither are now allowed to be null
        if ( m_pInstance == nullptr || rhs.m_pInstance == nullptr )
        {
            return false;
        }

        // Check that typeinfo is the same
        if ( m_pInstance->GetTypeInfo() != rhs.m_pInstance->GetTypeInfo() )
        {
            return false;
        }

        // Check property values
        return m_pInstance->GetTypeInfo()->AreAllPropertyValuesEqual( m_pInstance, rhs.m_pInstance );
    }

    void TypeInstance::DestroyInstance()
    {
        EE::Delete( m_pInstance );
    }
}