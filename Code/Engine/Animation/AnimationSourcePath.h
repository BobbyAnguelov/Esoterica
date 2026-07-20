#pragma once
#include "Base/Types/Arrays.h"
#include "Base/Encoding/Hash.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    //-------------------------------------------------------------------------
    // Specific int16 based path (primarily for graph paths)
    //-------------------------------------------------------------------------

    struct SourcePath
    {

    public:

        SourcePath() = default;
        SourcePath( SourcePath const& rhs ) { operator=( rhs ); }
        SourcePath( SourcePath&& rhs ) { operator=( eastl::move( rhs ) ); }

        //-------------------------------------------------------------------------

        void Clear() { m_path.clear(); m_hash = 0; }

        inline bool IsValid() const { return !m_path.empty(); }

        inline bool IsEmpty() const { return m_path.empty(); }

        inline int32_t Size() const { return (int32_t) m_path.size(); }

        void Push( int16_t nodeIdx )
        {
            m_path.emplace_back( nodeIdx );
            UpdateHash();
        }

        void Pop()
        {
            EE_ASSERT( !m_path.empty() );
            m_path.pop_back();
            UpdateHash();
        }

        void UpdateHash()
        {
            if ( m_path.empty() )
            {
                m_hash = 0;
            }
            else
            {
                m_hash = Hash::GetHash64( m_path.data(), m_path.size() * sizeof( int16_t ) );
            }
        }

        inline bool IsUnderOrEqual( SourcePath const &other ) const
        {
            if ( other.Size() < Size() )
            {
                return false;
            }

            for ( int32_t i = 0; i < Size(); i++ )
            {
                if ( m_path[i] != other.m_path[i] )
                {
                    return false;
                }
            }

            return true;
        }

        void PopFront( size_t numElements )
        {
            EE_ASSERT( numElements <= m_path.size() );

            if ( numElements > 0 )
            {
                m_path.erase( m_path.begin(), m_path.begin() + numElements );
            }
        }

        inline int16_t Front() const { return m_path.front(); }
        inline int16_t Back() const { return m_path.back(); }

        //-------------------------------------------------------------------------

        inline int16_t& operator[]( size_t i ) { return m_path[i]; }
        inline int16_t operator[]( size_t i ) const { return m_path[i]; }

        inline bool operator==( SourcePath const& rhs ) const { return m_hash == rhs.m_hash; }
        inline bool operator!=( SourcePath const& rhs ) const { return m_hash != rhs.m_hash; }

        SourcePath& operator=( SourcePath const& rhs )
        {
            m_path = rhs.m_path;
            m_hash = rhs.m_hash;
            return *this;
        }

        SourcePath& operator=( SourcePath&& rhs )
        {
            m_path.swap( rhs.m_path );
            m_hash = rhs.m_hash;
            return *this;
        }

    public:

        TInlineVector<int16_t, 6>   m_path;
        uint64_t                    m_hash = 0;
    };
}