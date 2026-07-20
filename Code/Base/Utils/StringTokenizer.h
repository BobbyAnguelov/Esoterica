#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API StringTokenIterator final
    {
    public:

        StringTokenIterator( StringView const& str, StringView const& delimiters );

        bool HasNext() const;
        StringView Peek() const;
        StringView PeekNext() const;
        StringView Next();

    private:

        StringView NextToken();

    private:

        StringView          m_state;
        StringView          m_token;
        StringView const    m_delimiters;
    };
}
