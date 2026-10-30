#include "ConeStepMapping.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "Base/Math/Math.h"
#include "Base/Time/Timers.h"

#include <thread>

//-------------------------------------------------------------------------

namespace EE::Render::ConeStepMap
{
    static const float      g_bakeRadiusUV = 1.0F / 32.0F; // Needs to match the constant in Parallax.esh!
    static const int32_t    g_numSamplesPerRadius = 2;
    static const int32_t    g_numDownsampleSteps = 8;
    static const int32_t    g_coneChannel = 0;
    static const int32_t    g_heightChannel = 1;
    static const int32_t    g_numRayMarchSteps = 12;

    //-------------------------------------------------------------------------

    namespace
    {
        EE_FORCE_INLINE int32_t WrapCoordinate( int32_t value, int32_t extent )
        {
            if ( value < 0 )
            {
                return value + extent;
            }

            return ( value >= extent ) ? ( value - extent ) : value;
        }

        EE_FORCE_INLINE int32_t RoundToInt( float value )
        {
            return Math::RoundToInt32( value );
        }

        EE_FORCE_INLINE int32_t ComputeGridExtent( int32_t sourceExtent )
        {
            int32_t const bandwidthExtent = Math::CeilingToInt32( float( g_numSamplesPerRadius ) / g_bakeRadiusUV );
            int32_t const extent = Math::Max( sourceExtent / g_numDownsampleSteps, Math::Min( bandwidthExtent, sourceExtent ) );

            return Math::Max( extent, 1 );
        }

        struct Grid
        {
            int32_t                 m_width = 0;
            int32_t                 m_height = 0;
            float                   m_bakeRadiusUV = 0.0F;  // Search radius for the cone bake, in UV
            TVector<float>          m_heights;              // 0 at the surface, 1 at full displacement
            TVector<uint8_t>        m_cones;                // Encoded cone ratio as a fraction of the bake radius
        };

        EE_FORCE_INLINE float HeightAt( Grid const& grid, int32_t x, int32_t y )
        {
            int32_t const wrappedX = WrapCoordinate( x, grid.m_width );
            int32_t const wrappedY = WrapCoordinate( y, grid.m_height );
            return grid.m_heights[size_t( wrappedY ) * size_t( grid.m_width ) + size_t( wrappedX )];
        }

        // Widest cone at (x, y), expressed in UV per unit of height
        float ComputeConeRatio( Grid const& grid, int32_t x, int32_t y, int32_t radiusX, int32_t radiusY )
        {
            float const texelSizeU = 1.0F / float( grid.m_width );
            float const texelSizeV = 1.0F / float( grid.m_height );
            float const radiusUVSquared = grid.m_bakeRadiusUV * grid.m_bakeRadiusUV;

            float const sourceHeight = HeightAt( grid, x, y );
            float ratio = g_bakeRadiusUV;

            for ( int32_t offsetY = -radiusY; offsetY <= radiusY; ++offsetY )
            {
                float const offsetV = float( offsetY ) * texelSizeV;

                for ( int32_t offsetX = -radiusX; offsetX <= radiusX; ++offsetX )
                {
                    float const offsetU = float( offsetX ) * texelSizeU;
                    float const distanceSquaredUV = offsetU * offsetU + offsetV * offsetV;

                    if ( distanceSquaredUV <= 0.0F || distanceSquaredUV > radiusUVSquared )
                    {
                        continue;
                    }

                    float const destinationHeight = HeightAt( grid, x + offsetX, y + offsetY );

                    // The ray starts at the top of the volume over the source texel and passes through the destination texel's surface point, so it drops by this much to reach it and keeps the same slope beyond.
                    float const dropToDestination = 1.0F - destinationHeight;
                    if ( dropToDestination <= 1e-4F )
                    {
                        continue;
                    }

                    float const lateralPerDrop = Math::Sqrt( distanceSquaredUV ) / dropToDestination;

                    // Follow the ray down until it leaves the relief, stopping once it is out of the radius
                    float exitRayDrop = -1.0F;
                    float exitLateral = 0.0F;

                    for ( int32_t step = 1; step <= g_numRayMarchSteps; ++step )
                    {
                        float const drop = destinationHeight * float( step ) / float( g_numRayMarchSteps );
                        float const rayDrop = dropToDestination + drop;
                        float const lateral = lateralPerDrop * rayDrop;

                        if ( lateral > grid.m_bakeRadiusUV )
                        {
                            break;
                        }

                        float const lateralFraction = rayDrop / dropToDestination;
                        float const surfaceHeight = HeightAt
                        (
                            grid,
                            x + RoundToInt( float( offsetX ) * lateralFraction ),
                            y + RoundToInt( float( offsetY ) * lateralFraction )
                        );

                        // Outside the relief again, so this is where the ray left through the surface.
                        if ( ( 1.0F - rayDrop ) > surfaceHeight + 1e-4F )
                        {
                            exitRayDrop = rayDrop;
                            exitLateral = lateral;
                            break;
                        }
                    }

                    if ( exitRayDrop < 0.0F )
                    {
                        continue;
                    }

                    float const exitHeight = 1.0F - exitRayDrop;
                    if ( exitHeight <= sourceHeight )
                    {
                        continue;
                    }

                    float const candidate = exitLateral / ( exitHeight - sourceHeight );
                    if ( candidate < ratio )
                    {
                        ratio = candidate;
                    }
                }
            }

            return ratio;
        }

        void GenerateRange( Grid& grid, int32_t radiusX, int32_t radiusY, int32_t rowStart, int32_t rowEnd )
        {
            for ( int32_t y = rowStart; y < rowEnd; ++y )
            {
                for ( int32_t x = 0; x < grid.m_width; ++x )
                {
                    float const ratio = ComputeConeRatio( grid, x, y, radiusX, radiusY );
                    float const normalized = ratio / g_bakeRadiusUV;

                    float const encoded = Math::Sqrt( ( normalized < 1.0F ) ? normalized : 1.0F );
                    float const stored = Math::Max( encoded * 255.0F, 1.0F );

                    grid.m_cones[size_t( y ) * size_t( grid.m_width ) + size_t( x )] = uint8_t( stored );
                }
            }
        }

        void GenerateGrid( Grid& grid, int32_t radiusX, int32_t radiusY )
        {
            int32_t const numThreads = int32_t( Math::Max( 1U, std::thread::hardware_concurrency() ) );
            int32_t const rowsPerThread = ( grid.m_height + numThreads - 1 ) / numThreads;

            TVector<std::thread> threads;
            threads.reserve( numThreads - 1 );

            for ( int32_t threadIndex = 1; threadIndex < numThreads; ++threadIndex )
            {
                int32_t const rowStart = Math::Min( threadIndex * rowsPerThread, grid.m_height );
                int32_t const rowEnd = Math::Min( rowStart + rowsPerThread, grid.m_height );

                if ( rowStart >= rowEnd )
                {
                    break;
                }

                threads.emplace_back( [&grid, radiusX, radiusY, rowStart, rowEnd] () { GenerateRange( grid, radiusX, radiusY, rowStart, rowEnd ); } );
            }

            GenerateRange( grid, radiusX, radiusY, 0, Math::Min( rowsPerThread, grid.m_height ) );

            for ( std::thread& thread : threads )
            {
                thread.join();
            }
        }
    }

    //-------------------------------------------------------------------------

    void Generate( Resource::CompileContext const& ctx, uint8_t* pSurfaceData, int32_t width, int32_t height, int32_t strideInBytes, int32_t searchRadiusInTexels )
    {
        int32_t const bytesPerPixel = strideInBytes / width;
        if ( bytesPerPixel <= g_heightChannel )
        {
            ctx.LogError( "A cone step map needs a surface with room for a height and a cone channel" );
            return;
        }

        Grid grid;
        grid.m_width = ComputeGridExtent( width );
        grid.m_height = ComputeGridExtent( height );
        grid.m_heights.resize( size_t( grid.m_width ) * size_t( grid.m_height ) );
        grid.m_cones.resize( size_t( grid.m_width ) * size_t( grid.m_height ) );

        // Downsample by taking the highest point of each block
        for ( int32_t y = 0; y < grid.m_height; ++y )
        {
            int32_t const sourceYStart = ( y * height ) / grid.m_height;
            int32_t const sourceYEnd = Math::Max( sourceYStart + 1, ( ( y + 1 ) * height ) / grid.m_height );

            for ( int32_t x = 0; x < grid.m_width; ++x )
            {
                int32_t const sourceXStart = ( x * width ) / grid.m_width;
                int32_t const sourceXEnd = Math::Max( sourceXStart + 1, ( ( x + 1 ) * width ) / grid.m_width );

                uint8_t highest = 0;
                for ( int32_t sourceY = sourceYStart; sourceY < sourceYEnd; ++sourceY )
                {
                    for ( int32_t sourceX = sourceXStart; sourceX < sourceXEnd; ++sourceX )
                    {
                        uint8_t const sourceHeight = pSurfaceData[sourceY * strideInBytes + sourceX * bytesPerPixel + g_heightChannel];
                        highest = ( sourceHeight > highest ) ? sourceHeight : highest;
                    }
                }

                grid.m_heights[size_t( y ) * size_t( grid.m_width ) + size_t( x )] = float( highest ) * ( 1.0F / 255.0F );
            }
        }

        float const texelSizeU = 1.0F / float( grid.m_width );
        float const texelSizeV = 1.0F / float( grid.m_height );

        grid.m_bakeRadiusUV = Math::Max( float( searchRadiusInTexels ) / float( width ), g_bakeRadiusUV );

        int32_t const radiusX = Math::Clamp( Math::CeilingToInt32( grid.m_bakeRadiusUV / texelSizeU ), 1, Math::Max( 1, grid.m_width / 2 ) );
        int32_t const radiusY = Math::Clamp( Math::CeilingToInt32( grid.m_bakeRadiusUV / texelSizeV ), 1, Math::Max( 1, grid.m_height / 2 ) );

        Milliseconds generationTime;
        {
            ScopedTimer<PlatformClock, Milliseconds> tt( generationTime );

            // Default / empty height fields result in zeroed out cones
            bool hasHeight = false;
            for ( float const gridHeight : grid.m_heights )
            {
                if ( gridHeight > 0.0F )
                {
                    hasHeight = true;
                    break;
                }
            }

            if ( hasHeight )
            {
                GenerateGrid( grid, radiusX, radiusY );
            }
            else
            {
                eastl::fill( grid.m_cones.begin(), grid.m_cones.end(), 0 );
            }

            for ( int32_t y = 0; y < height; ++y )
            {
                int32_t const gridY = ( y * grid.m_height ) / height;

                for ( int32_t x = 0; x < width; ++x )
                {
                    int32_t const gridX = ( x * grid.m_width ) / width;
                    pSurfaceData[y * strideInBytes + x * bytesPerPixel + g_coneChannel] = grid.m_cones[size_t( gridY ) * size_t( grid.m_width ) + size_t( gridX )];
                }
            }

            float sum = 0.0F;
            for ( uint8_t const cone : grid.m_cones )
            {
                sum += float( cone );
            }

            size_t const numSaturated = size_t( eastl::count( grid.m_cones.begin(), grid.m_cones.end(), uint8_t( 255 ) ) );
            ctx.LogMessage( "Cone ratio: mean %.0f of 255, %.1f%% saturated", sum / float( grid.m_cones.size() ), 100.0F * float( numSaturated ) / float( grid.m_cones.size() ) );
        }

        ctx.LogMessage( "Cone step map generation time: %.1fms", generationTime.ToFloat() );
    }
}
