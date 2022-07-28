#pragma once

#include "System/_Module/API.h"
#include "System/Types/Containers_ForwardDecl.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------
// String ID
//-------------------------------------------------------------------------
// Deterministic numeric ID generated from a string
// StringIDs are CASE-SENSITIVE!
// Uses the 32bit default hash

namespace eastl
{
    template <typename Value, bool bCacheHashCode>
    struct hash_node;

    template <typename T1, typename T2>
    struct pair;
}

//-------------------------------------------------------------------------

namespace EE
{
    class StringID_CustomAllocator;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API StringID
    {
    public:

        using StringIDHashNode = eastl::hash_node<eastl::pair<const uint32_t, eastl::basic_string<char, EE::StringID_CustomAllocator>>, false>;

        struct DebuggerInfo
        {
            StringIDHashNode const* const*  m_pBuckets = nullptr;
            size_t                          m_numBuckets = 0;
        };

        static DebuggerInfo const*          s_pDebuggerInfo;

    public:

        StringID() = default;
        explicit StringID( nullptr_t ) : m_ID( 0 ) {}
        explicit StringID( char const* pStr );
        explicit StringID( uint32_t ID ) : m_ID( ID ) {}
        explicit StringID( String const& str );

        inline bool IsValid() const { return m_ID != 0; }
        inline uint32_t GetID() const { return m_ID; }
        inline operator uint32_t() const { return m_ID; }

        inline void Clear() { m_ID = 0; }

        char const* c_str() const;

        inline bool operator==( StringID const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( StringID const& rhs ) const { return m_ID != rhs.m_ID; }

    private:

        uint32_t                            m_ID = 0;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <typename T> struct hash;

    template <>
    struct hash<EE::StringID>
    {
        size_t operator()( EE::StringID const& ID ) const { return (uint32_t) ID; }
    };
}