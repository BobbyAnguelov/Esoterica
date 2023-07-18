#include "FloatCurve.h"
#include "Curves.h"
#include "Vector.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    // ID generator needed for curve editor
    uint16_t FloatCurve::s_pointIdentifierGenerator = 0;
    #endif

    //-------------------------------------------------------------------------

    float FloatCurve::Evaluate( float parameter ) const
    {
        float result = 0;

        if ( m_points.empty() )
        {
            return result;
        }

        if ( m_points.size() == 1 )
        {
            return m_points[0].m_value;
        }

        //-------------------------------------------------------------------------

        FloatRange const parameterRange = GetParameterRange();

        if ( parameterRange.ContainsExclusive( parameter ) )
        {
            int32_t const numPoints = GetNumPoints();
            int32_t const numCurves = numPoints - 1;
            for ( int32_t i = 0; i < numCurves; i++ )
            {
                int32_t const startIdx = i;
                int32_t const endIdx = i + 1;

                // If the parameter is within this curve, evaluate it
                if ( parameter >= m_points[startIdx].m_parameter && parameter <= m_points[endIdx].m_parameter )
                {
                    float const T = ( parameter - m_points[startIdx].m_parameter ) / ( m_points[endIdx].m_parameter - m_points[startIdx].m_parameter );
                    result = Math::CubicHermite::GetPoint( m_points[startIdx].m_value, m_points[startIdx].m_outTangent, m_points[endIdx].m_value, m_points[endIdx].m_inTangent, T );
                    break;
                }
            }
        }
        else // Outside curve range
        {
            if ( parameter <= parameterRange.m_begin )
            {
                result = m_points[0].m_value;
            }
            else
            {
                result = m_points.back().m_value;
            }
        }

        return result;
    }

    void FloatCurve::AddPoint( float parameter, float value, float inTangent, float outTangent )
    {
        m_points.push_back( { parameter, value, inTangent, outTangent } );
        SortPoints();

        #if EE_DEVELOPMENT_TOOLS
        RegeneratePointIDs();
        #endif
    }

    void FloatCurve::EditPoint( int32_t pointIdx, float parameter, float value )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );
        m_points[pointIdx].m_parameter = parameter;
        m_points[pointIdx].m_value = value;
        SortPoints();
    }

    void FloatCurve::SetPointTangentMode( int32_t pointIdx, TangentMode mode )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );
        m_points[pointIdx].m_tangentMode = mode;
    }

    void FloatCurve::SetPointOutTangent( int32_t pointIdx, float tangent )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );
        m_points[pointIdx].m_outTangent = tangent;
    }

    void FloatCurve::SetPointInTangent( int32_t pointIdx, float tangent )
    {
        m_points[pointIdx].m_inTangent = tangent;
    }

    void FloatCurve::RemovePoint( int32_t pointIdx )
    {
        EE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );

        m_points.erase( m_points.begin() + pointIdx );

        #if EE_DEVELOPMENT_TOOLS
        RegeneratePointIDs();
        #endif
    }

    //-------------------------------------------------------------------------
    // Core Type Requirements
    //-------------------------------------------------------------------------

    bool FloatCurve::operator==( FloatCurve const& rhs ) const
    {
        if ( m_points.size() != rhs.m_points.size() )
        {
            return false;
        }

        int32_t numPoints = GetNumPoints();
        for ( int32_t i = 0; i < numPoints; i++ )
        {
            if ( m_points[i] != rhs.m_points[i] )
            {
                return false;
            }
        }

        return true;
    }

    #if EE_DEVELOPMENT_TOOLS
    void FloatCurve::RegeneratePointIDs()
    {
        for ( auto& point : m_points )
        {
            point.m_ID = ++s_pointIdentifierGenerator;
        }
    }
    #endif

    bool FloatCurve::FromString( String const& inStr, FloatCurve& outCurve )
    {
        outCurve.Clear();

        // Read number of points
        //-------------------------------------------------------------------------

        size_t startIdx = 0;
        size_t endIdx = inStr.find_first_of( ',', startIdx );
        if ( endIdx == String::npos )
        {
            return false;
        }

        char* pCaret = nullptr;
        uint64_t numPoints = std::strtoul( &inStr.c_str()[startIdx], &pCaret, 0 );

        //-------------------------------------------------------------------------

        for ( auto i = 0u; i < numPoints; i++ )
        {
            Point& p = outCurve.m_points.emplace_back();

            if ( pCaret[0] != ',' ) { return false; }
            p.m_parameter = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_value = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_inTangent = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_outTangent = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_tangentMode = (TangentMode) std::strtoul( ++pCaret, &pCaret, 0 );
        }

        outCurve.SortPoints();

        #if EE_DEVELOPMENT_TOOLS
        outCurve.RegeneratePointIDs();
        #endif

        return true;
    }

    String FloatCurve::ToString() const
    {
        TInlineString<100> pointStr;

        String curveStr;
        curveStr.reserve( GetNumPoints() * 30 ); // rough over-estimate of 30 characters per point
        curveStr.sprintf( "%u", (uint32_t) GetNumPoints() );

        for ( auto& point : m_points )
        {
            pointStr.sprintf( ",%f,%f,%f,%f,%u", point.m_parameter, point.m_value, point.m_inTangent, point.m_outTangent, (uint8_t) point.m_tangentMode );
            curveStr.append( pointStr.c_str() );
        }

        return curveStr;
    }
}