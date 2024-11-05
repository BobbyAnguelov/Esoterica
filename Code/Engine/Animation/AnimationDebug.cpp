#include "AnimationDebug.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    InlineString DebugPath::GetFlattenedPath() const
    {
        InlineString pathStr;

        for ( auto const& element : m_path )
        {
            pathStr.append_sprintf( "%s/", element.m_pathString.c_str() );
        }

        if ( !pathStr.empty() )
        {
            pathStr.pop_back();
        }

        return pathStr;
    }

    void DebugPath::PopFront( size_t numElements )
    {
        EE_ASSERT( numElements <= m_path.size() );

        if ( numElements > 0 )
        {
            m_path.erase( m_path.begin(), m_path.begin() + numElements );
        }
    }

    bool DebugPath::IsParentOf( DebugPath const& potentialChild ) const
    {
        for ( size_t i = 0; i < m_path.size(); i++ )
        {
            if ( m_path[i].m_itemID != potentialChild.m_path[i].m_itemID )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    DebugPathTracker::~DebugPathTracker()
    {
        EE_ASSERT( m_basePath.empty() );
    }

    void DebugPathTracker::Clear()
    {
        EE_ASSERT( m_basePath.empty() );
        m_itemPaths.clear();
    }
}
#endif