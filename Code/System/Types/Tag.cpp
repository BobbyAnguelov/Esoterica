#include "Tag.h"
#include "String.h"

//-------------------------------------------------------------------------

namespace EE
{
    Tag Tag::FromTagFormatString( String const& tagFormatString )
    {
        Tag tag;

        //-------------------------------------------------------------------------

        String tempString;
        size_t idx, lastIdx = 0;
        for ( int32_t i = 0; i < 4; i++ )
        {
            idx = tagFormatString.find_first_of( '.', lastIdx );
            if ( idx == String::npos )
            {
                idx = tagFormatString.length();

                if ( idx != lastIdx )
                {
                    tempString.assign( tagFormatString.data() + lastIdx, idx - lastIdx );
                    tag.m_c[i] = StringID( tempString );
                }
                break;
            }
            else
            {
                if ( idx != lastIdx )
                {
                    tempString.assign( tagFormatString.data() + lastIdx, idx - lastIdx );
                    tag.m_c[i] = StringID( tempString );
                }
            }

            lastIdx = idx + 1;
        }

        return tag;
    }

    Tag::Tag( char const* p0, char const* p1, char const* p2, char const* p3 )
    {
        m_c[0] = StringID( p0 );
        m_c[1] = StringID( p1 );
        m_c[2] = StringID( p2 );
        m_c[3] = StringID( p3 );

        EE_ASSERT( IsValid() );
    }

    Tag::Tag( StringID s0, StringID s1, StringID s2, StringID s3 )
    {
        m_c[0] = s0;
        m_c[1] = s1;
        m_c[2] = s2;
        m_c[3] = s3;

        EE_ASSERT( IsValid() );
    }

    bool Tag::IsValid() const
    {
        for ( auto i = 3; i >= 0; i-- )
        {
            // If we have a value set
            if ( m_c[i].IsValid() )
            {
                // Ensure all previous values are set
                for ( auto j = i - 1; j >= 0; j-- )
                {
                    if ( !m_c[j].IsValid() )
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }

    int32_t Tag::GetDepth() const
    {
        for ( int32_t i = 3; i >= 0; i-- )
        {
            // If we have a value set
            if ( m_c[i].IsValid() )
            {
                // Ensure all previous values are set
                for ( int32_t j = i - 1; j >= 0; j-- )
                {
                    if ( !m_c[j].IsValid() )
                    {
                        return 0;
                    }
                }

                return i + 1;
            }
        }

        return 0;
    }

    Tag Tag::GetParentTag() const
    {
        Tag parentTag;
        uint32_t const depth = GetDepth();
        switch ( depth )
        {
            case 1:
            {
                parentTag = Tag( m_c[0] );
            }
            break;

            case 2:
            {
                parentTag = Tag( m_c[0], m_c[1] );
            }
            break;

            case 3:
            {
                parentTag = Tag( m_c[0], m_c[1], m_c[2] );
            }
            break;
        }

        return parentTag;
    }

    StringID Tag::GetValueAtLevel( int32_t level )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( level >= 0 && level <= 3 );

        switch ( level )
        {
            case 0:
            {
                EE_ASSERT( m_c[0].IsValid() );
                return m_c[0];
            }
            break;

            case 1:
            {
                EE_ASSERT( m_c[1].IsValid() );
                return m_c[1];
            }
            break;

            case 2:
            {
                EE_ASSERT( m_c[2].IsValid() );
                return m_c[2];
            }
            break;

            case 3:
            {
                EE_ASSERT( m_c[3].IsValid() );
                return m_c[3];
            }
            break;
        }

        EE_UNREACHABLE_CODE();
        return StringID();
    }

    bool Tag::IsParentOf( Tag const& childTag ) const
    {
        EE_ASSERT( IsValid() && childTag.IsValid() );

        if ( m_c[0] != childTag.m_c[0] )
        {
            return false;
        }

        if ( m_c[1].IsValid() )
        {
            if ( m_c[1] != childTag.m_c[1] )
            {
                return false;
            }

            if ( m_c[2].IsValid() )
            {
                if ( m_c[2] != childTag.m_c[2] )
                {
                    return false;
                }

                // This is kinda silly, since this is just a inefficient tag comparison at this point
                if ( m_c[3].IsValid() )
                {
                    if ( m_c[3] != childTag.m_c[3] )
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool Tag::IsChildOf( Tag const& parentTag ) const
    {
        EE_ASSERT( IsValid() && parentTag.IsValid() );

        if ( m_c[0] != parentTag.m_c[0] )
        {
            return false;
        }

        if ( parentTag.m_c[1].IsValid() )
        {
            if ( m_c[1] != parentTag.m_c[1] )
            {
                return false;
            }

            if ( parentTag.m_c[2].IsValid() )
            {
                if ( m_c[2] != parentTag.m_c[2] )
                {
                    return false;
                }

                // This is kinda silly, since this is just a inefficient tag comparison at this point
                if ( parentTag.m_c[3].IsValid() )
                {
                    if ( m_c[3] != parentTag.m_c[3] )
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    String Tag::ToString() const
    {
        String result;

        uint32_t const depth = GetDepth();
        switch ( depth )
        {
            case 1:
            {
                result.sprintf( "%s", m_c[0].c_str() );
            }
            break;

            case 2:
            {
                result.sprintf( "%s.%s", m_c[0].c_str(), m_c[1].c_str() );
            }
            break;

            case 3:
            {
                result.sprintf( "%s.%s.%s", m_c[0].c_str(), m_c[1].c_str(), m_c[2].c_str() );
            }
            break;

            case 4:
            {
                result.sprintf( "%s.%s.%s.%s", m_c[0].c_str(), m_c[1].c_str(), m_c[2].c_str(), m_c[3].c_str() );
            }
            break;
        }

        return result;
    }
}