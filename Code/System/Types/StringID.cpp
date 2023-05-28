#include "StringID.h"
#include "System/Memory/Memory.h"
#include "System/Encoding/Hash.h"
#include "System/Threading/Threading.h"
#include "String.h"
#include "EASTL/hash_map.h"

//-------------------------------------------------------------------------

namespace EE
{
    class StringID_CustomAllocator
    {
    public:

        EASTL_ALLOCATOR_EXPLICIT StringID_CustomAllocator( const char* pName = EASTL_NAME_VAL( EASTL_ALLOCATOR_DEFAULT_NAME ) ) {}
        StringID_CustomAllocator( const StringID_CustomAllocator& x ) {}
        StringID_CustomAllocator( const StringID_CustomAllocator& x, const char* pName ) {}
        StringID_CustomAllocator& operator=( const StringID_CustomAllocator& x ) { return *this; }
        const char* get_name() const { return "StringID"; }
        void set_name( const char* pName ) {}

        void* allocate( size_t n, int flags = 0 )
        {
            return allocate( n, EASTL_SYSTEM_ALLOCATOR_MIN_ALIGNMENT, 0, flags );
        }

        void* allocate( size_t n, size_t alignment, size_t offset, int flags = 0 )
        {
            size_t adjustedAlignment = ( alignment > EA_PLATFORM_PTR_SIZE ) ? alignment : EA_PLATFORM_PTR_SIZE;

            void* p = new char[n + adjustedAlignment + EA_PLATFORM_PTR_SIZE];
            void* pPlusPointerSize = (void*) ( (uintptr_t) p + EA_PLATFORM_PTR_SIZE );
            void* pAligned = (void*) ( ( (uintptr_t) pPlusPointerSize + adjustedAlignment - 1 ) & ~( adjustedAlignment - 1 ) );

            void** pStoredPtr = (void**) pAligned - 1;
            EASTL_ASSERT( pStoredPtr >= p );
            *( pStoredPtr ) = p;

            EASTL_ASSERT( ( (size_t) pAligned & ~( alignment - 1 ) ) == (size_t) pAligned );
            return pAligned;
        }

        void deallocate( void* p, size_t n )
        {
            if ( p != nullptr )
            {
                void* pOriginalAllocation = *( (void**) p - 1 );
                delete[]( char* )pOriginalAllocation;
            }
        }
    };

    inline bool operator==( const StringID_CustomAllocator&, const StringID_CustomAllocator& ) { return true; }
    inline bool operator!=( const StringID_CustomAllocator&, const StringID_CustomAllocator& ) { return false; }

    //-------------------------------------------------------------------------

    using CachedString = eastl::basic_string<char, StringID_CustomAllocator>;

    class StringIDHashMap : public eastl::hash_map<uint32_t, CachedString, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, StringID_CustomAllocator>
    {
    public:
      
        eastl::hash_node<value_type, false> const* const* GetBuckets() const { return mpBucketArray; }
    };

    //-------------------------------------------------------------------------

    StringIDHashMap g_stringCache;
    Threading::Mutex g_stringCacheMutex;

    // Natvis/Debugger info to print out human-readable strings
    StringID::DebuggerInfo g_debuggerInfo;
    EE::StringID::DebuggerInfo const* StringID::s_pDebuggerInfo = &g_debuggerInfo;

    //-------------------------------------------------------------------------

    StringID::StringID( char const* pStr )
    {
        if ( pStr != nullptr && strlen( pStr ) > 0 )
        {
            m_ID = Hash::GetHash32( pStr );

            // Cache the string
            Threading::ScopeLock lock( g_stringCacheMutex );
            auto iter = g_stringCache.find( m_ID );
            if ( iter == g_stringCache.end() )
            {
                g_stringCache[m_ID] = CachedString( pStr );
                g_debuggerInfo.m_pBuckets = g_stringCache.GetBuckets();
                g_debuggerInfo.m_numBuckets = g_stringCache.bucket_count();
            }
        }
    }

    StringID::StringID( String const& str )
        : StringID( str.c_str() )
    {}

    char const* StringID::c_str() const
    {
        if ( m_ID == 0 )
        {
            return nullptr;
        }

        {
            // Get cached string
            Threading::ScopeLock lock( g_stringCacheMutex );
            auto iter = g_stringCache.find( m_ID );
            if ( iter != g_stringCache.end() )
            {
                return iter->second.c_str();
            }
        }

        // ID likely directly created via uint32_t
        return nullptr;
    }
}