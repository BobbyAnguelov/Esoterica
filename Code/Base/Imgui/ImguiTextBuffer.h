#pragma once
#include "Base/Types/String.h"
#include "Base/Memory/Memory.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------
// ImGuiX Text Buffer
//-------------------------------------------------------------------------
// Useful for InputText Fields, especially ones that you want to be read-only

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    struct TextBuffer
    {
        constexpr static size_t const s_defaultBufferSize = 256;

    public:

        TextBuffer( size_t bufferSize = s_defaultBufferSize )
        {
            Clear( bufferSize );
        }

        inline void Fill( String const& str )
        {
            m_buffer.resize( Math::Max( m_buffer.size(), str.size() + 1 ) );
            Memory::MemsetZero( m_buffer.data(), m_buffer.size() );
            memcpy( m_buffer.data(), str.data(), str.length() );
        }

        inline void Fill( InlineString const& str )
        {
            m_buffer.resize( Math::Max( m_buffer.size(), str.size() + 1 ) );
            Memory::MemsetZero( m_buffer.data(), m_buffer.size() );
            memcpy( m_buffer.data(), str.data(), str.length() );
        }

        inline void Fill( char const* pStr )
        {
            size_t const strLength = strlen( pStr );
            m_buffer.resize( Math::Max( m_buffer.size(), strLength + 1 ) );
            Memory::MemsetZero( m_buffer.data(), m_buffer.size() );
            memcpy( m_buffer.data(), pStr, strLength );
        }

        inline void Append( String const& str )
        {
            size_t const currentContentsLength = strlen( m_buffer.data() );
            size_t const requiredSize = currentContentsLength + str.length() + 1;

            m_buffer.resize( Math::Max( m_buffer.size(), requiredSize ) );
            Memory::MemsetZero( m_buffer.data() + currentContentsLength, m_buffer.size() - currentContentsLength );
            memcpy( m_buffer.data() + currentContentsLength, str.data(), str.length() );
        }

        inline void Append( InlineString const& str )
        {
            size_t const currentContentsLength = strlen( m_buffer.data() );
            size_t const requiredSize = currentContentsLength + str.length() + 1;

            m_buffer.resize( Math::Max( m_buffer.size(), requiredSize ) );
            Memory::MemsetZero( m_buffer.data() + currentContentsLength, m_buffer.size() - currentContentsLength );
            memcpy( m_buffer.data() + currentContentsLength, str.data(), str.length() );
        }

        inline void Append( char const* pStr )
        {
            size_t const strLength = strlen( pStr );
            size_t const currentContentsLength = strlen( m_buffer.data() );
            size_t const requiredSize = currentContentsLength + strLength + 1;

            m_buffer.resize( Math::Max( m_buffer.size(), requiredSize ) );
            Memory::MemsetZero( m_buffer.data() + currentContentsLength, m_buffer.size() - currentContentsLength );
            memcpy( m_buffer.data() + currentContentsLength, pStr, strLength );
        }

        inline void Clear( size_t size = s_defaultBufferSize )
        {
            m_buffer.resize( Math::Max( m_buffer.size(), size_t( size ) ) );
            Memory::MemsetZero( m_buffer.data(), m_buffer.size() );
        }

        String GetString() const { return String( m_buffer.data() ); }
        StringView GetStringView() const { return StringView( m_buffer.data() ); }

        inline char* Data() { return m_buffer.data(); }
        inline char const* Data() const { return m_buffer.data(); }

        inline bool Empty() const { return m_buffer.empty(); }

        inline size_t Size() const { return m_buffer.size(); }

    public:

        TVector<char>                           m_buffer;
    };
}
#endif