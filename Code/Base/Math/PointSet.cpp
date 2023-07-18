#include "PointSet.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PointSet::ApplyTransform( Matrix const& transform )
    {
        m_min = transform.TransformPoint( m_min );
        m_max = transform.TransformPoint( m_max );

        for ( auto& pt : m_points )
        {
            pt = transform.TransformPoint( Vector( pt, 1.0f ) );
        }
    }
}