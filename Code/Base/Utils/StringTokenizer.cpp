#include "StringTokenizer.h"

//-------------------------------------------------------------------------

namespace EE
{
    StringTokenIterator::StringTokenIterator( StringView const& str, StringView const& delimiters )
        : m_state( str )
        , m_delimiters( delimiters )
    {}

    bool StringTokenIterator::HasNext() const
    {
        return !m_state.empty();
    }

    StringView StringTokenIterator::Peek() const
    {
        return m_token;
    }

    StringView StringTokenIterator::PeekNext() const
    {
        size_t tokenStart = m_state.find_first_not_of( m_delimiters );
        if ( tokenStart == StringView::npos )
        {
            return {};
        }

        size_t tokenEnd = m_state.find_first_of( m_delimiters, tokenStart + 1 );
        return m_state.substr( tokenStart, tokenEnd );
    }

    StringView StringTokenIterator::Next()
    {
        m_token = NextToken();
        return m_token;
    }

    StringView StringTokenIterator::NextToken()
    {
        size_t tokenStart = m_state.find_first_not_of( m_delimiters );
        if ( tokenStart == StringView::npos )
        {
            StringView token = m_state;
            m_state = {};

            return token;
        }

        size_t tokenEnd = m_state.find_first_of( m_delimiters, tokenStart + 1 );
        if ( tokenEnd == StringView::npos )
        {
            StringView token = m_state;
            m_state = {};

            return token;
        }

        StringView token = m_state.substr( tokenStart, tokenEnd - tokenStart );
        m_state = m_state.substr( tokenEnd );

        return token;
    }
}
