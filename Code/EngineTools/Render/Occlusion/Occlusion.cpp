#include "Occlusion.h"
#include "Base/Math/Math.h"
#include "Base/Time/Timers.h"

#include <thread>

//-------------------------------------------------------------------------

namespace EE::Render::Occlusion
{
    static const int32_t    g_searchRadiusInTexels = 32;
    static const int32_t    g_numDirections = 8;
    static const int32_t    g_numSteps = 6;

    //-------------------------------------------------------------------------

    namespace
    {
        struct Surface
        {
            uint8_t*        m_pData = nullptr;
            int32_t         m_width = 0;
            int32_t         m_height = 0;
            int32_t         m_strideInBytes = 0;
            int32_t         m_pixelSizeInBytes = 0;
            int32_t         m_channel = 0;
        };

        struct Directions
        {
            float m_x[g_numDirections] = {};
            float m_y[g_numDirections] = {};
        };

        Directions ComputeDirections()
        {
            Directions directions;

            for ( int32_t directionIndex = 0; directionIndex < g_numDirections; ++directionIndex )
            {
                float const directionAngle = ( 2.0F * Math::Pi * float( directionIndex ) ) / float( g_numDirections );

                directions.m_x[directionIndex] = Math::Cos( directionAngle );
                directions.m_y[directionIndex] = Math::Sin( directionAngle );
            }

            return directions;
        }

        Directions const g_directions = ComputeDirections();

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE int32_t WrapCoordinate( int32_t value, int32_t extent )
        {
            if ( value < 0 )
            {
                return value + extent;
            }

            return ( value >= extent ) ? ( value - extent ) : value;
        }

        EE_FORCE_INLINE uint8_t HeightAt( HeightField const& field, float scaleX, float scaleY, int32_t x, int32_t y )
        {
            int32_t const sourceX = int32_t( float( x ) * scaleX );
            int32_t const sourceY = int32_t( float( y ) * scaleY );

            return field.m_pData[size_t( sourceY ) * size_t( field.m_strideInBytes ) + size_t( sourceX ) * size_t( field.m_pixelSizeInBytes ) + field.m_channel];
        }

        EE_FORCE_INLINE size_t ChannelIndex( Surface const& surface, int32_t x, int32_t y )
        {
            return size_t( y ) * size_t( surface.m_strideInBytes ) + size_t( x ) * size_t( surface.m_pixelSizeInBytes ) + size_t( surface.m_channel );
        }

        void ComputeRange( Surface const& surface, HeightField const& field, float scaleX, float scaleY, float strength, int32_t rowStart, int32_t rowEnd )
        {
            int32_t const searchRadius = Math::Min( g_searchRadiusInTexels, Math::Min( surface.m_width, surface.m_height ) / 2 );

            for ( int32_t y = rowStart; y < rowEnd; ++y )
            {
                for ( int32_t x = 0; x < surface.m_width; ++x )
                {
                    float const centreHeight = float( HeightAt( field, scaleX, scaleY, x, y ) ) * ( 1.0F / 255.0F );

                    float occlusionSum = 0.0F;

                    for ( int32_t directionIndex = 0; directionIndex < g_numDirections; ++directionIndex )
                    {
                        float const directionX = g_directions.m_x[directionIndex];
                        float const directionY = g_directions.m_y[directionIndex];

                        float steepestRise = 0.0F;

                        for ( int32_t stepIndex = 1; stepIndex <= g_numSteps; ++stepIndex )
                        {
                            // Geometric spacing, allows the near field to accumulate the most samples.
                            float const radiusFraction = float( stepIndex ) / float( g_numSteps );
                            int32_t const radius = int32_t( float( searchRadius ) * radiusFraction * radiusFraction ) + 1;

                            int32_t const sampleX = WrapCoordinate( x + int32_t( directionX * float( radius ) ), surface.m_width );
                            int32_t const sampleY = WrapCoordinate( y + int32_t( directionY * float( radius ) ), surface.m_height );

                            float const sampleHeight = float( HeightAt( field, scaleX, scaleY, sampleX, sampleY ) ) * ( 1.0F / 255.0F );
                            float const rise = sampleHeight - centreHeight;
                            if ( rise <= 0.0F )
                            {
                                continue;
                            }

                            float const slope = rise / radiusFraction;
                            steepestRise = ( slope > steepestRise ) ? slope : steepestRise;
                        }

                        occlusionSum += steepestRise / Math::Sqrt( 1.0F + steepestRise * steepestRise );
                    }

                    float const openness = 1.0F - ( occlusionSum / float( g_numDirections ) ) * strength;
                    float const clampedOpenness = ( openness < 0.0F ) ? 0.0F : ( ( openness > 1.0F ) ? 1.0F : openness );

                    surface.m_pData[ChannelIndex( surface, x, y )] = uint8_t( clampedOpenness * 255.0F + 0.5F );
                }
            }
        }

        // Separable box blur of the occlusion channel
        void BlurChannel( Surface const& surface, int32_t radius )
        {
            int32_t const kernelSize = radius * 2 + 1;

            TVector<uint8_t> line;
            line.resize( size_t( Math::Max( surface.m_width, surface.m_height ) ) );

            for ( int32_t passIndex = 0; passIndex < 2; ++passIndex )
            {
                bool const horizontal = ( passIndex == 0 );

                int32_t const numLines = horizontal ? surface.m_height : surface.m_width;
                int32_t const numElements = horizontal ? surface.m_width : surface.m_height;

                size_t const lineStride = horizontal ? size_t( surface.m_strideInBytes ) : size_t( surface.m_pixelSizeInBytes );
                size_t const elementStride = horizontal ? size_t( surface.m_pixelSizeInBytes ) : size_t( surface.m_strideInBytes );

                for ( int32_t lineIndex = 0; lineIndex < numLines; ++lineIndex )
                {
                    size_t const lineStart = size_t( lineIndex ) * lineStride + size_t( surface.m_channel );

                    // Copied out first, so the blur reads a contiguous run while it writes back in place.
                    for ( int32_t element = 0; element < numElements; ++element )
                    {
                        line[size_t( element )] = surface.m_pData[lineStart + size_t( element ) * elementStride];
                    }

                    int32_t sum = 0;
                    for ( int32_t offset = -radius; offset <= radius; ++offset )
                    {
                        sum += line[size_t( WrapCoordinate( offset, numElements ) )];
                    }

                    for ( int32_t element = 0; element < numElements; ++element )
                    {
                        int32_t const average = ( sum + kernelSize / 2 ) / kernelSize;
                        surface.m_pData[lineStart + size_t( element ) * elementStride] = uint8_t( average );

                        // Slide the window one along.
                        sum -= line[size_t( WrapCoordinate( element - radius, numElements ) )];
                        sum += line[size_t( WrapCoordinate( element + radius + 1, numElements ) )];
                    }
                }
            }
        }

        void ApplyContrast( Surface const& surface, float contrast )
        {
            for ( int32_t y = 0; y < surface.m_height; ++y )
            {
                for ( int32_t x = 0; x < surface.m_width; ++x )
                {
                    size_t const index = ChannelIndex( surface, x, y );

                    float const normalised = Math::Pow( float( surface.m_pData[index] ) * ( 1.0F / 255.0F ), contrast );
                    surface.m_pData[index] = uint8_t( normalised * 255.0F + 0.5F );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void Generate( Resource::CompileContext const& ctx, Settings const& settings, uint8_t* pSurfaceData, int32_t width, int32_t height, int32_t strideInBytes, int32_t targetChannel, HeightField const& field )
    {
        Surface surface;
        surface.m_pData = pSurfaceData;
        surface.m_width = width;
        surface.m_height = height;
        surface.m_strideInBytes = strideInBytes;
        surface.m_pixelSizeInBytes = strideInBytes / width;
        surface.m_channel = targetChannel;

        if ( surface.m_pixelSizeInBytes <= targetChannel )
        {
            ctx.LogError( "A computed occlusion map needs a surface with a channel to write into" );
            return;
        }

        if ( field.m_pData == nullptr || field.m_width <= 0 || field.m_height <= 0 )
        {
            ctx.LogError( "A computed occlusion map needs a height field to measure against" );
            return;
        }

        if ( field.m_pixelSizeInBytes <= field.m_channel )
        {
            ctx.LogError( "A computed occlusion map is reading a channel the height image does not have" );
            return;
        }

        float const scaleX = float( field.m_width ) / float( width );
        float const scaleY = float( field.m_height ) / float( height );
        int32_t const blurRadius = Math::Min( settings.m_blurRadius, Math::Max( 0, Math::Min( width, height ) / 2 - 1 ) );

        Milliseconds generationTime;
        {
            ScopedTimer<PlatformClock, Milliseconds> tt( generationTime );

            int32_t const numThreads = int32_t( Math::Max( 1U, std::thread::hardware_concurrency() ) );
            int32_t const rowsPerThread = ( height + numThreads - 1 ) / numThreads;

            TVector<std::thread> threads;
            threads.reserve( numThreads - 1 );

            for ( int32_t threadIndex = 1; threadIndex < numThreads; ++threadIndex )
            {
                int32_t const rowStart = Math::Min( threadIndex * rowsPerThread, height );
                int32_t const rowEnd = Math::Min( rowStart + rowsPerThread, height );

                if ( rowStart >= rowEnd )
                {
                    break;
                }

                threads.emplace_back( [&surface, &field, scaleX, scaleY, &settings, rowStart, rowEnd] () { ComputeRange( surface, field, scaleX, scaleY, settings.m_strength, rowStart, rowEnd ); } );
            }

            ComputeRange( surface, field, scaleX, scaleY, settings.m_strength, 0, Math::Min( rowsPerThread, height ) );

            for ( std::thread& thread : threads )
            {
                thread.join();
            }

            if ( blurRadius > 0 )
            {
                BlurChannel( surface, blurRadius );
            }

            if ( !Math::IsNearEqual( settings.m_contrast, 1.0F ) )
            {
                ApplyContrast( surface, settings.m_contrast );
            }
        }

        ctx.LogMessage( "Occlusion generation time: %.1fms", generationTime.ToFloat() );
    }
}
