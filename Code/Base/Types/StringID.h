#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/Containers_ForwardDecl.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------
// String ID
//-------------------------------------------------------------------------
// Deterministic numeric ID generated from a string
// StringIDs are CASE-SENSITIVE!
// Uses the 64bit default hash

namespace eastl
{
    template <typename Value, bool CacheHashCode>
    struct hash_node;

    template <typename T1, typename T2>
    struct pair;
}

//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API StringID
    {
    public:

        using StringIDHashNode = eastl::hash_node<eastl::pair<const uint64_t, String>, false>;

        struct DebuggerInfo
        {
            StringIDHashNode const* const*  m_pBuckets = nullptr;
            size_t                          m_numBuckets = 0;
        };

        #if EE_DEVELOPMENT_TOOLS
        static DebuggerInfo*                s_pDebuggerInfo;
        #endif

        // Initialize global state for StringID system
        static void Initialize();

        // Shutdown global state for StringID system
        static void Shutdown();

    public:

        StringID() = default;
        explicit StringID( nullptr_t ) : m_ID( 0 ) {}
        explicit StringID( char const* pStr );
        explicit StringID( uint64_t ID ) : m_ID( ID ) {}
        explicit StringID( String const& str );
        explicit StringID( InlineString const& str );

        inline bool IsValid() const { return m_ID != 0; }
        inline uint64_t ToUint() const { return m_ID; }
        inline operator uint64_t() const { return m_ID; }

        inline void Clear() { m_ID = 0; }

        char const* c_str() const;

        inline bool operator==( StringID const& rhs ) const { return m_ID == rhs.m_ID; }
        inline bool operator!=( StringID const& rhs ) const { return m_ID != rhs.m_ID; }

    private:

        uint64_t                            m_ID = 0;
    };

    // Statically defined StringID
    //-------------------------------------------------------------------------
    // Use this for all static members and translation unit globals

    class EE_BASE_API StaticStringID
    {
    public:

        // General constructor
        StaticStringID( char const* pStr );

        // Lazily construct the ID on first use
        inline StringID const& GetID() const
        {
            if ( !m_isCreated )
            {
                m_ID = StringID( m_buffer );
                m_isCreated = true;
            }

            return m_ID;
        }

        inline operator StringID() const { return GetID(); }
        inline bool operator==( StringID const& rhs ) const { return GetID() == rhs; }
        inline char const* c_str() const { return GetID().c_str(); }

    private:

        StaticStringID() = delete;

    private:

        char                        m_buffer[64] = { 0 };
        mutable StringID            m_ID;
        mutable bool                m_isCreated = false;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <typename T> struct hash;

    template <>
    struct hash<EE::StringID>
    {
        size_t operator()( EE::StringID const& ID ) const { return (uint64_t) ID; }
    };
}