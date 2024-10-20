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