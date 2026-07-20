#pragma once
#include "Matrix.h"

//-------------------------------------------------------------------------

namespace EE
{
    // This is a storage type primarily used for render resources, avoid using it
    struct Matrix43
    {
        EE_SERIALIZE( m_rows );

        Matrix43() = default;
        explicit Matrix43( NoInit_t ) {}
        explicit Matrix43( ZeroInit_t ) { memset( this, 0, sizeof( Matrix ) ); }
        explicit Matrix43( Matrix const& m ) { FromMatrix( m ); }

        void FromMatrix( Matrix const& m )
        {
            m_rows[0] = m.GetRow( 0 ).ToFloat3();
            m_rows[1] = m.GetRow( 1 ).ToFloat3();
            m_rows[2] = m.GetRow( 2 ).ToFloat3();
            m_rows[3] = m.GetRow( 3 ).ToFloat3();
        }

        Matrix ToMatrix() const
        {
            return Matrix( Vector( m_rows[0] ), Vector( m_rows[1] ), Vector( m_rows[2] ), Vector( m_rows[3] ) );
        }

    public:

        Float3 m_rows[4] = { Float3( 1, 0, 0 ), Float3( 0, 1, 0 ), Float3( 0, 0, 1 ), Float3::Zero };
    };
}