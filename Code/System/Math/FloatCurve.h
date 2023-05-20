#pragma once
#include "Math.h"
#include "NumericRange.h"
#include "System/Types/Arrays.h"
#include "System/Types/String.h"
#include <EASTL/sort.h>

//-------------------------------------------------------------------------
// Interpolation Curve
//-------------------------------------------------------------------------
// A sequence of piece wise cubic hermite splines -
// This curve is useful for when you want remap one float value to another

namespace EE
{
    class EE_SYSTEM_API FloatCurve
    {
        EE_SERIALIZE( m_points );

        #if EE_DEVELOPMENT_TOOLS
        static uint16_t s_pointIdentifierGenerator;
        #endif

    public:

        // Set curve state from a string
        static bool FromString( String const& inStr, FloatCurve& outCurve );

        // The tangent options per point
        enum TangentMode : uint8_t
        {
            Free,
            Locked,
        };

        // A 1D curve point - the ID is needed for the tooling to distinguish points from one another
        struct Point
        {
            EE_SERIALIZE( m_parameter, m_value, m_inTangent, m_outTangent, m_tangentMode );

            inline bool operator==( Point const& rhs ) const
            {
                return m_parameter == rhs.m_parameter && m_value == rhs.m_value && m_inTangent == rhs.m_inTangent && m_outTangent == rhs.m_outTangent && m_tangentMode == rhs.m_tangentMode;
            }

            inline bool operator!=( Point const& rhs ) const { return !operator==( rhs ); }

            float           m_parameter;
            float           m_value;
            float           m_inTangent = 1;
            float           m_outTangent = 1;
            TangentMode     m_tangentMode = TangentMode::Free;

            #if EE_DEVELOPMENT_TOOLS
            uint16_t          m_ID; // Not serialized, runtime generated through edit operations
            #endif
        };

    public:

        // Curve query
        //-------------------------------------------------------------------------

        inline int32_t GetNumPoints() const { return (int32_t) m_points.size(); }
        Point const& GetPoint( int32_t pointIdx ) const { EE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() ); return m_points[pointIdx]; }

        // Get the range for the parameters that this curve covers
        inline FloatRange GetParameterRange() const 
        {
            FloatRange range;
            for( auto i = 0u; i < m_points.size(); i++ )
            {
                range.m_begin = Math::Min( range.m_begin, m_points[i].m_parameter );
                range.m_end = Math::Max( range.m_end, m_points[i].m_parameter );
            }
            return range;
        }

        // Get the range for the values that this curve covers
        // Note: this will evaluate the curve to find the actual value range and so is pretty expensive!
        inline FloatRange GetValueRange() const
        {
            constexpr static float const numPointsToEvaluate = 150;
            FloatRange const parameterRange = GetParameterRange();
            float const stepT = parameterRange.GetLength() / numPointsToEvaluate;

            FloatRange valueRange( Evaluate( 0.0f ) );
            for ( auto i = 1; i < numPointsToEvaluate; i++ )
            {
                float const t = parameterRange.m_begin + ( i * stepT );
                valueRange.GrowRange( Evaluate( t ) );
            }

            return valueRange;
        }

        // Evaluate the curve and return the value for the specified input parameter
        // If the parameter supplied is outside the parameter range the value returned will be that of the nearest extremity point
        float Evaluate( float parameter ) const;

        // Curve manipulation
        //-------------------------------------------------------------------------

        void AddPoint( float parameter, float value, float inTangent = 1.0f, float outTangent = 1.0f );
        void EditPoint( int32_t pointIdx, float parameter, float value );
        void SetPointTangentMode( int32_t pointIdx, TangentMode mode );
        void SetPointOutTangent( int32_t pointIdx, float tangent );
        void SetPointInTangent( int32_t pointIdx, float tangent );
        void RemovePoint( int32_t pointIdx );
        void Clear() { m_points.clear(); }

        #if EE_DEVELOPMENT_TOOLS
        void RegeneratePointIDs();
        #endif

        // Serialization
        //-------------------------------------------------------------------------

        // Set the string state from a string
        // Warning! This will crash on invalid strings, if you are not sure if the string is valid, use the static function supplied
        inline void FromString( String const& inStr ) { bool result = FloatCurve::FromString( inStr, *this ); EE_ASSERT( result ); }

        // Returns the curve state as a string
        String ToString() const;

        // Operators
        //-------------------------------------------------------------------------

        // Equality operators needed for core types
        bool operator==( FloatCurve const& rhs ) const;

        // Equality operators needed for core types
        inline bool operator!=( FloatCurve const& rhs ) const { return !operator==( rhs ); }

    private:

        inline void SortPoints()
        {
            auto SortPredicate = [] ( Point const& a, Point const& b )
            {
                return a.m_parameter < b.m_parameter;
            };

            eastl::sort( m_points.begin(), m_points.end(), SortPredicate );
        }

    private:

        TInlineVector<Point, 8>     m_points; // Space for 4 curves
    };
}