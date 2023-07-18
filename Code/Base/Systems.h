#pragma once

#include "Base/_Module/API.h"
#include "Base/Encoding/Hash.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------
// Defines an interface for an engine system
//-------------------------------------------------------------------------
// 
// Engine systems are global (often singleton) systems that are not tied to a game world
// i.e. InputSystem, ResourceSystem, TypeRegistry...
//
// This interface exists primarily for type safety and ease of use
// Each system needs a publicly accessible uint32_t ID called 'SystemID'
// Use the macro provided below to declare new systems
// 
//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API ISystem
    {
    public:

        virtual ~ISystem() = default;
        virtual uint32_t GetSystemID() const = 0;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API SystemRegistry
    {
    public:

        SystemRegistry() {}
        ~SystemRegistry();

        void RegisterSystem( ISystem* pSystem );
        void UnregisterSystem( ISystem* pSystem );

        template<typename T>
        inline T* GetSystem() const
        {
            static_assert( std::is_base_of<EE::ISystem, T>::value, "T is not derived from ISystem" );

            for ( auto pSystem : m_registeredSystems )
            {
                if ( pSystem->GetSystemID() == T::s_systemID )
                {
                    return reinterpret_cast<T*>( pSystem );
                }
            }

            EE_UNREACHABLE_CODE();
            return nullptr;
        }

    private:

        TInlineVector<ISystem*, 20>     m_registeredSystems;
    };
}

//-------------------------------------------------------------------------

#define EE_SYSTEM( TypeName ) \
constexpr static uint32_t const s_systemID = Hash::FNV1a::GetHash32( #TypeName ); \
virtual uint32_t GetSystemID() const override final { return TypeName::s_systemID; }