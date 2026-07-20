#pragma once
#include "Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    template<typename T>
    class TPath
    {

    public:

        TPath() = default;
        TPath( TPath const &rhs ) { m_elements = rhs.m_elements; }

        inline void Clear() { m_elements.clear(); }

        inline bool IsEmpty() const { return m_elements.IsEmpty(); }

        inline int32_t Size() const { return (int32_t) m_elements.size(); }

        inline bool Empty() const { return m_elements.empty(); }

        inline int32_t GetNumElements() const { return (int32_t) m_elements.size(); }

        inline void Append( TPath<T> const &rhs )
        {
            for ( auto const &v : rhs )
            {
                m_elements.emplace_back( v );
            }
        }

        bool IsParentOf( TPath<T> const &potentialChild ) const
        {
            if ( potentialChild.size() < m_elements.size() )
            {
                return false;
            }

            for ( int32_t i = 0; i < m_elements.size(); i++ )
            {
                if ( m_elements[i] != potentialChild.m_path[i] )
                {
                    return false;
                }
            }

            return true;
        }

        void PopFront( int32_t n = 1 )
        {
            EE_ASSERT( n > 0 );
            EE_ASSERT( n <= m_elements.size() );

            if ( n > 0 )
            {
                m_elements.erase( m_elements.begin(), m_elements.begin() + n );
            }
        }

        void PushFront( T const& v )
        {
            m_elements.insert( m_elements.begin(), v );
        }

        void PopBack( int32_t n = 1 )
        {
            EE_ASSERT( n > 0 );
            EE_ASSERT( n <= m_elements.size() );

            if ( n > 0 )
            {
                m_elements.erase( m_elements.end() - n, m_elements.end() );
            }
        }

        void PushBack( T const& v )
        {
            m_elements.emplace_back( v );
        }

        void PushBack( T&& v )
        {
            m_elements.emplace_back( eastl::forward( v ) );
        }

        inline bool operator==( TPath const& rhs ) const
        {
            if ( m_elements.size() != rhs.m_elements.size() )
            {
                return false;
            }

            for ( size_t i = 0; i < m_elements.size(); i++ )
            {
                if ( m_elements[i] != rhs.m_elements[i] )
                {
                    return false;
                }
            }

            return true;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TInlineVector<T, 10>::iterator begin() { return m_elements.begin(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::iterator end() { return m_elements.end(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::const_iterator begin() const { return m_elements.begin(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::const_iterator end() const { return m_elements.end(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::reverse_iterator rbegin() { return m_elements.rbegin(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::reverse_iterator rend() { return m_elements.rend(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::const_reverse_iterator rbegin() const { return m_elements.rbegin(); }
        EE_FORCE_INLINE TInlineVector<T, 10>::const_reverse_iterator rend() const { return m_elements.rend(); }
        EE_FORCE_INLINE T &operator[]( uint32_t i ) { return m_elements[i]; }
        EE_FORCE_INLINE T const &operator[]( uint32_t i ) const { return m_elements[i]; }

    public:

        TInlineVector<T, 10> m_elements;
    };
}