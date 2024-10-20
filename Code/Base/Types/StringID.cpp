#include "StringID.h"
#include "Base/Memory/Memory.h"
#include "Base/Encoding/Hash.h"
#include "Base/Threading/Threading.h"
#include "String.h"
#include "EASTL/hash_map.h"

//-------------------------------------------------------------------------

namespace EE
{
    class StringIDHashMap : public eastl::hash_map<uint64_t, String>
    {
    public:
      
        eastl::hash_node<value_type, false> const* const* GetBuckets() const { return mpBucketArray; }
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    StringID::DebuggerInfo*             StringID::s_pDebuggerInfo = nullptr;
    #endif

    static Threading::Mutex*            g_pStringCacheMutex = nullptr;
    static StringIDHashMap*             g_pStringCache = nullptr;

    //-------------------------------------------------------------------------

    void StringID::Initialize()
    {
        g_pStringCacheMutex = EE::New<Threading::Mutex>();
        g_pStringCache = EE::New<StringIDHashMap>();

        #if EE_DEVELOPMENT_TOOLS
        s_pDebuggerInfo = EE::New<StringID::DebuggerInfo>();
        #endif
    }

    void StringID::Shutdown()
    {
        #if EE_DEVELOPMENT_TOOLS
        EE::Delete( s_pDebuggerInfo );
        #endif

        EE::Delete( g_pStringCache);
        EE::Delete( g_pStringCacheMutex );
    }

    //-------------------------------------------------------------------------

    StringID::StringID( char const* pStr )
    {
        // If this is nullptr then you are likely trying to statically allocate a stringID, this is not allowed and you need to use the "StaticStringID" type instead!
        EE_ASSERT( g_pStringCacheMutex != nullptr ); 

        if ( pStr != nullptr && strlen( pStr ) > 0 )
        {
            m_ID = Hash::GetHash64( pStr );

            // Cache the string
            Threading::ScopeLock lock( *g_pStringCacheMutex );
            auto iter = g_pStringCache->find( m_ID );
            if ( iter == g_pStringCache->end() )
            {
                ( *g_pStringCache )[m_ID] = String( pStr );

                #if EE_DEVELOPMENT_TOOLS
                s_pDebuggerInfo->m_pBuckets = g_pStringCache->GetBuckets();
                s_pDebuggerInfo->m_numBuckets = g_pStringCache->bucket_count();
                #endif
            }
            else
            {
                #if EE_DEVELOPMENT_TOOLS
                EE_ASSERT( iter->second == pStr );
                #endif
            }
        }
    }

    StringID::StringID( String const& str )
        : StringID( str.c_str() )
    {}

    StringID::StringID( InlineString const& str )
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
            Threading::ScopeLock lock( *g_pStringCacheMutex );
            auto iter = g_pStringCache->find( m_ID );
            if ( iter != g_pStringCache->end() )
            {
                return iter->second.c_str();
            }
        }

        // ID likely directly created via uint64_t
        return nullptr;
    }

    //-------------------------------------------------------------------------

    StaticStringID::StaticStringID( char const* pStr )
    {
        EE_ASSERT( pStr != nullptr );
        size_t const length = strlen( pStr );
        EE_ASSERT( length < 64 );
        memcpy( m_buffer, pStr, length);
    }
}