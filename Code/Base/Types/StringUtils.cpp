#include "String.h"

//-------------------------------------------------------------------------

namespace EE::StringUtils
{
    void InsertSpacesAccordingToCapitalization( String& str )
    {
        if ( str.empty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        int32_t i = 1;
        while ( i < str.length() )
        {
            bool const isLetterUppercase = isupper( str[i] ) != 0;
            if ( !isLetterUppercase )
            {
                i++;
                continue;
            }

            bool shouldInsertSpace = false;

            bool const isPreviousLetterUppercase = isupper( str[i-1] ) != 0;
            if ( !isPreviousLetterUppercase )
            {
                shouldInsertSpace = true;
            }
            else // Previous was uppercase
            {
                bool const isLastLetter = i == ( str.length() - 1 );
                if ( !isLastLetter )
                {
                    bool const isNextLetterLowercase = !isupper( str[i + 1] );
                    if ( isNextLetterLowercase )
                    {
                        shouldInsertSpace = true;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( shouldInsertSpace )
            {
                str.insert( str.begin() + i, 1, ' ' );
                i++;
            }

            i++;
        }
    }
}