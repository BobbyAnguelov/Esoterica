#include "PropertyPath.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    PropertyPath::PropertyPath( String const& pathString )
    {
        TVector<String> pathStrings;
        StringUtils::Split( pathString, pathStrings, "/" );
        EE_ASSERT( !pathStrings.empty() );

        auto const numPathEntries = pathStrings.size();
        for ( auto i = 0u; i < numPathEntries; i++ )
        {
            if ( isdigit( pathStrings[i][0] ) )
            {
                EE_ASSERT( !m_pathElements.empty() );
                m_pathElements.back().m_arrayElementIdx = atoi( pathStrings[i].c_str() );
            }
            else
            {
                m_pathElements.emplace_back( PathElement( StringID( pathStrings[i] ) ) );
            }
        }
    }

    String PropertyPath::ToString() const
    {
        String pathString;

        int32_t const numElements = (int32_t) m_pathElements.size();
        if ( numElements > 0 )
        {
            for ( auto i = 0; i < numElements - 1; i++ )
            {
                auto const& pathElement = m_pathElements[i];

                pathString += pathElement.m_propertyID.c_str();

                if ( pathElement.IsArrayElement() )
                {
                    pathString += String( String::CtorSprintf(), "/%d/", pathElement.m_arrayElementIdx );
                }
                else
                {
                    pathString += '/';
                }
            }

            // We dont want a trailing slash for the last element
            //-------------------------------------------------------------------------

            auto const& pathElement = m_pathElements.back();
            pathString += pathElement.m_propertyID.c_str();
            if ( pathElement.IsArrayElement() )
            {
                pathString += String( String::CtorSprintf(), "/%d", pathElement.m_arrayElementIdx );
            }
        }

        return pathString;
    }
}