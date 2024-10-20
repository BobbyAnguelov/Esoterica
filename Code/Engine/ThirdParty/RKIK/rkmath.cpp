//--------------------------------------------------------------------------------------------------
// math.cpp
//
// Copyright(C) by D. Gregorius. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include "rkmath.h"
#include "rkmemory.h"

//--------------------------------------------------------------------------------------------------
// General matrix inversion
//--------------------------------------------------------------------------------------------------
void rkInvert( float* Out, float* M, int N, int Lda )
	{
	float* pr_in = M;
	float* pr_out = Out;

	// Initialize identity matrix
		{  
		float* p = pr_out;
		rkMemZero( p, Lda * N * sizeof( float ) );
		
		for ( int k = N - 1; k >= 0; k-- )
			{
			p[ 0 ] = 1.0f;
			p += Lda + 1;
			}
		}

	// Forward step
		{
		for ( int i = 0; i < N; i++ )
			{
			float f = pr_in[ i ];

			f = 1.0f / f;
				{ 
				// scale this row
				int s = i;
				do
				{
					pr_in[ s ] *= f;
				}
				while ( ++s < N );

				int t = 0;
				do
				{
					pr_out[ t ] *= f;
				}
				while ( ++t <= i );
			}
	
			float* row_to_zero = pr_in;
			float* inverted_row = pr_out;

			for ( int row = i + 1; row < N; row++ )
				{
				row_to_zero += Lda;
				inverted_row += Lda;

				float factor = row_to_zero[ i ];
				if ( factor == 0.0f )
					{ 
					// matrix elem is already zero
					continue;
					}

				int k = 0;
				do
					{
					inverted_row[ k ] -= pr_out[ k ] * factor;
					}
				while ( ++k <= i );

				k = i;
				do
					{
					row_to_zero[ k ] -= pr_in[ k ] * factor;
					}
				while ( ++k < N );
				}

			pr_in += Lda;
			pr_out += Lda;
			}
		}

	// Reverse step (diagonal is already scaled)
		{
		for ( int i = N - 1; i >= 0; i-- )
			{
			pr_in -= Lda;
			pr_out -= Lda;

			float* row_to_zero = pr_in;
			float* inverted_row = pr_out;

			for ( int row = i - 1; row >= 0; row-- )
				{
				row_to_zero -= Lda;
				inverted_row -= Lda;

				float factor = row_to_zero[ i ];
				if ( factor == 0.0f )
					{ 
					// matrix elem is already zero
					continue;
					}

				int k = N - 1;
				do
					{
					inverted_row[ k ] -= pr_out[ k ] * factor;
					}
				while ( --k >= 0 );

				k = N - 1;
				do
					{
					row_to_zero[ k ] -= pr_in[ k ] * factor;
					}
				while ( --k >= i );
				}
			}
		}
	}