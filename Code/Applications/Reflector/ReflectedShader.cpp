#include "ReflectedShader.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    static String GenerateFriendlyName( String const& name )
    {
        String friendlyName = name;

        if ( name.length() <= 1 )
        {
            return friendlyName;
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "m_", "" );

        if ( friendlyName.length() > 1 && friendlyName[0] == 'p' && isupper( friendlyName[1] ) )
        {
            friendlyName = friendlyName.substr( 1, friendlyName.length() - 1 );
        }

        //-------------------------------------------------------------------------

        //StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "_", " " );

        friendlyName[0] = (char) toupper( friendlyName[0] );

        return friendlyName;
    }

    //-------------------------------------------------------------------------------------------------------

    ReflectedShader::CompiledData::CompiledData( String const& ID, uint8_t const* pData, size_t dataSize )
        : m_ID( ID )
    {
        EE_ASSERT( !m_ID.empty() );
        EE_ASSERT( pData != nullptr );
        EE_ASSERT( dataSize > 0 );

        m_byteCode.resize( dataSize );
        memcpy( m_byteCode.data(), pData, dataSize );
    }


    ReflectedShader::ParameterInfo::ParameterInfo( char const * pType, char const * pName, uint32_t stride, uint32_t offset, bool isHandle, TVector<String> const & templateArguments, TVector<ParameterAnnotation> const & annotations )
        : m_type( pType )
        , m_name( pName )
        , m_strideInBytes( stride )
        , m_offsetInBytes( offset )
        , m_isHandle( isHandle )
        , m_templateArguments( templateArguments )
        , m_annotations( annotations )
    {
        m_friendlyName = GenerateFriendlyName( m_name );
    }
}
