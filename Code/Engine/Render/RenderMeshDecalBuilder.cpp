#include "RenderMeshDecalBuilder.h"
#include "Engine/Render/Shaders/MeshData.esh"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static constexpr uint32_t s_maxDecalsPerTriangle = ShaderTypes::g_meshDecalMaxCountPerTriangle;
    static_assert( s_maxDecalsPerTriangle == ( ( 1U << ( 32U - ShaderTypes::g_meshDecalCountShift ) ) - 1U ), "Per triangle decal count does not match the packed attribute layout" );

    static constexpr float s_maxDecalSpaceError = 1e-3F;

    //-------------------------------------------------------------------------

    namespace
    {
        struct BakeCandidate
        {
            uint32_t                    m_decalIndex = 0;
            uint32_t                    m_triangleIndex = 0;
            TArray<Float2, 3>           m_uv = {};                     // Mesh UV per corner
            TArray<Float2, 3>           m_decalPosition = {};          // Decal space position per corner
        };

        struct TriangleProjection
        {
            Float4                      m_linear = Float4::Zero;       // ( x, y ) from ( u, v ) in x,y and z,w
            Float2                      m_centroidUV = Float2::Zero;
            Float2                      m_centroidDecal = Float2::Zero;
        };

        struct ProjectionBasis
        {
            uint32_t                    m_decalIndex = 0;
            Float4                      m_linear = Float4::Zero;
            Float2                      m_anchorUV = Float2::Zero;
        };

        //-------------------------------------------------------------------------

        static uint32_t ComputeDecalTriangleCandidates
        (
            TArray<Float3, 3> const& trianglePositions,
            TArray<Float2, 3> const& triangleUVs,
            uint32_t triangleIndex,
            TArrayView<MeshDecalBakeData const> decals,
            bool& outTooManyDecals,
            TVector<BakeCandidate>& outCandidates
        )
        {
            uint32_t numEmitted = 0;

            for ( uint32_t decalIdx = 0; decalIdx < decals.size(); decalIdx++ )
            {
                MeshDecalBakeData const& decal = decals[decalIdx];

                Float3 const extents = Float3( decal.m_nonUniformScale ) * decal.m_localTransform.GetScale();
                if ( extents.m_x <= 0.0F || extents.m_y <= 0.0F || extents.m_z <= 0.0F )
                {
                    continue;
                }

                // Project the triangle corners into decal space, the decal volume is [-0.5, 0.5]
                Quaternion const inverseRotation = decal.m_localTransform.GetRotation().GetInverse();
                Vector const inverseExtents( 1.0F / extents.m_x, 1.0F / extents.m_y, 1.0F / extents.m_z );
                Vector const center = decal.m_localTransform.GetTranslationAsPoint();

                TArray<Vector, 3> localPositions;
                for ( uint32_t cornerIdx = 0; cornerIdx < 3; cornerIdx++ )
                {
                    localPositions[cornerIdx] = inverseRotation.RotateVector( Vector( trianglePositions[cornerIdx] ) - center ) * inverseExtents;
                }

                // Contribution, the projected triangle has to overlap the decal volume in depth and in the decal plane
                float const tolerance = 0.01F;

                float const minimumZ = Math::Min( Math::Min( localPositions[0].GetZ(), localPositions[1].GetZ() ), localPositions[2].GetZ() );
                float const maximumZ = Math::Max( Math::Max( localPositions[0].GetZ(), localPositions[1].GetZ() ), localPositions[2].GetZ() );
                if ( minimumZ > ( 0.5F + tolerance ) || maximumZ < ( -0.5F - tolerance ) )
                {
                    continue;
                }

                float const minimumX = Math::Min( Math::Min( localPositions[0].GetX(), localPositions[1].GetX() ), localPositions[2].GetX() );
                float const maximumX = Math::Max( Math::Max( localPositions[0].GetX(), localPositions[1].GetX() ), localPositions[2].GetX() );
                float const minimumY = Math::Min( Math::Min( localPositions[0].GetY(), localPositions[1].GetY() ), localPositions[2].GetY() );
                float const maximumY = Math::Max( Math::Max( localPositions[0].GetY(), localPositions[1].GetY() ), localPositions[2].GetY() );
                if ( minimumX > ( 0.5F + tolerance ) || maximumX < ( -0.5F - tolerance ) ||
                     minimumY >( 0.5F + tolerance ) || maximumY < ( -0.5F - tolerance ) )
                {
                    continue;
                }

                // Quality, reject triangles that are close to edge on to the projection axis, the decal would smear across them.
                // Matches the decal coverage fade threshold used by DecalInstance.
                Vector const triangleNormalLocalUnnormalized = Vector::Cross3( localPositions[1] - localPositions[0], localPositions[2] - localPositions[0] );
                if ( triangleNormalLocalUnnormalized.GetLengthSquared3() <= 1e-12F )
                {
                    continue;
                }

                Vector const triangleNormalLocal = triangleNormalLocalUnnormalized.GetNormalized3();
                if ( Math::Abs( triangleNormalLocal.GetZ() ) < 0.17364818F )
                {
                    continue;
                }

                // Solve the exact per triangle affine mapping the mesh UV to the decal space
                Float2 const edgeUV1 = triangleUVs[1] - triangleUVs[0];
                Float2 const edgeUV2 = triangleUVs[2] - triangleUVs[0];

                float const determinant = edgeUV1.m_x * edgeUV2.m_y - edgeUV1.m_y * edgeUV2.m_x;
                if ( Math::Abs( determinant ) < 1e-8F )
                {
                    continue;
                }

                // Do not silently drop decals when a triangle is covered by more than the packed attribute can store
                if ( numEmitted == s_maxDecalsPerTriangle )
                {
                    outTooManyDecals = true;
                    break;
                }

                BakeCandidate& candidate = outCandidates.emplace_back();
                candidate = {};
                candidate.m_decalIndex = decalIdx;
                candidate.m_triangleIndex = triangleIndex;

                for ( uint32_t cornerIdx = 0; cornerIdx < 3; cornerIdx++ )
                {
                    candidate.m_uv[cornerIdx] = triangleUVs[cornerIdx];
                    candidate.m_decalPosition[cornerIdx] = Float2( localPositions[cornerIdx].GetX(), localPositions[cornerIdx].GetY() );
                }

                numEmitted++;
            }

            return numEmitted;
        }

        // Max decal space error of a triangle when it is projected with the given linear part
        static float ComputeProjectionError( BakeCandidate const& candidate, TriangleProjection const& triangleProjection, Float4 const& linear )
        {
            float maxError = 0.0F;

            for ( uint32_t cornerIdx = 0; cornerIdx < 3; cornerIdx++ )
            {
                float const deltaU = candidate.m_uv[cornerIdx].m_x - triangleProjection.m_centroidUV.m_x;
                float const deltaV = candidate.m_uv[cornerIdx].m_y - triangleProjection.m_centroidUV.m_y;

                float const errorX = ( triangleProjection.m_linear.m_x - linear.m_x ) * deltaU + ( triangleProjection.m_linear.m_y - linear.m_y ) * deltaV;
                float const errorY = ( triangleProjection.m_linear.m_z - linear.m_z ) * deltaU + ( triangleProjection.m_linear.m_w - linear.m_w ) * deltaV;

                maxError = Math::Max( maxError, Math::Max( Math::Abs( errorX ), Math::Abs( errorY ) ) );
            }

            return maxError;
        }
    }

    //-------------------------------------------------------------------------

    MeshDecalBuilder::Error MeshDecalBuilder::Build( GeometryBuilder& geometryBuilder, TArrayView<MeshDecalBakeData const> decals )
    {
        EE_ASSERT( !geometryBuilder.HasDecalAttributes() );

        if ( decals.empty() )
        {
            return Error::None;
        }

        // The decal index is stored in the projection as 16 bits
        if ( decals.size() > size_t( 0xFFFF ) )
        {
            return Error::TooManyDecals;
        }

        if ( geometryBuilder.GetNumTextureCoordinateAttributes() == 0 )
        {
            return Error::NoTextureCoordinates;
        }

        uint32_t const numTriangles = geometryBuilder.GetNumTriangles();
        if ( numTriangles == 0 )
        {
            return Error::None;
        }

        // Collect the bake candidates for every triangle the decal volumes overlap
        //-------------------------------------------------------------------------

        TVector<BakeCandidate> candidates;
        TVector<uint32_t> numTriangleCandidates;
        numTriangleCandidates.resize( numTriangles, 0 );
        bool tooManyDecalsPerTriangle = false;

        TArrayView<uint32_t const> const indices = geometryBuilder.GetIndices();

        for ( uint32_t triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex )
        {
            TArray<Float3, 3> const trianglePositions =
            {
                geometryBuilder.GetPositionAttribute( indices[triangleIndex * 3] ).m_position,
                geometryBuilder.GetPositionAttribute( indices[triangleIndex * 3 + 1] ).m_position,
                geometryBuilder.GetPositionAttribute( indices[triangleIndex * 3 + 2] ).m_position
            };

            TArray<Float2, 3> triangleUVs;
            bool hasUVs = true;
            for ( uint32_t cornerIndex = 0; cornerIndex < 3; cornerIndex++ )
            {
                if ( !geometryBuilder.GetTextureCoordinateAttribute( indices[triangleIndex * 3 + cornerIndex], 0, triangleUVs[cornerIndex] ) )
                {
                    hasUVs = false;
                    break;
                }
            }

            if ( !hasUVs )
            {
                continue;
            }

            numTriangleCandidates[triangleIndex] = ComputeDecalTriangleCandidates( trianglePositions, triangleUVs, triangleIndex, decals, tooManyDecalsPerTriangle, candidates );
        }

        if ( tooManyDecalsPerTriangle )
        {
            return Error::TooManyDecalsPerTriangle;
        }

        uint32_t const numCandidates = uint32_t( candidates.size() );
        if ( numCandidates == 0 )
        {
            return Error::None;
        }

        if ( numCandidates > ShaderTypes::g_meshDecalOffsetMask )
        {
            return Error::TooManyProjections;
        }

        // Exact per triangle linear part and centroids
        //-------------------------------------------------------------------------

        TVector<TriangleProjection> triangleProjections;
        triangleProjections.resize( numCandidates );

        for ( uint32_t candidateIndex = 0; candidateIndex < numCandidates; candidateIndex++ )
        {
            BakeCandidate const& candidate = candidates[candidateIndex];
            TriangleProjection& triangleProjection = triangleProjections[candidateIndex];

            Float2 const uv0 = candidate.m_uv[0];
            float const edgeUV1X = candidate.m_uv[1].m_x - uv0.m_x;
            float const edgeUV1Y = candidate.m_uv[1].m_y - uv0.m_y;
            float const edgeUV2X = candidate.m_uv[2].m_x - uv0.m_x;
            float const edgeUV2Y = candidate.m_uv[2].m_y - uv0.m_y;

            Float2 const decal0 = candidate.m_decalPosition[0];
            float const edgeDecal1X = candidate.m_decalPosition[1].m_x - decal0.m_x;
            float const edgeDecal1Y = candidate.m_decalPosition[1].m_y - decal0.m_y;
            float const edgeDecal2X = candidate.m_decalPosition[2].m_x - decal0.m_x;
            float const edgeDecal2Y = candidate.m_decalPosition[2].m_y - decal0.m_y;

            float const inverseDeterminant = 1.0F / ( edgeUV1X * edgeUV2Y - edgeUV1Y * edgeUV2X );

            triangleProjection.m_linear.m_x = ( edgeDecal1X * edgeUV2Y - edgeDecal2X * edgeUV1Y ) * inverseDeterminant;
            triangleProjection.m_linear.m_y = ( -edgeDecal1X * edgeUV2X + edgeDecal2X * edgeUV1X ) * inverseDeterminant;
            triangleProjection.m_linear.m_z = ( edgeDecal1Y * edgeUV2Y - edgeDecal2Y * edgeUV1Y ) * inverseDeterminant;
            triangleProjection.m_linear.m_w = ( -edgeDecal1Y * edgeUV2X + edgeDecal2Y * edgeUV1X ) * inverseDeterminant;

            triangleProjection.m_centroidUV = ( candidate.m_uv[0] + candidate.m_uv[1] + candidate.m_uv[2] ) * ( 1.0F / 3.0F );
            triangleProjection.m_centroidDecal = ( candidate.m_decalPosition[0] + candidate.m_decalPosition[1] + candidate.m_decalPosition[2] ) * ( 1.0F / 3.0F );
        }

        // Merge similar projections into shared projection bases.
        // Triangles join the basis with the smallest error as long as it stays within tolerance, connectivity is not required for that guarantee.
        //-------------------------------------------------------------------------

        TVector<ProjectionBasis> projectionBases;
        TVector<TVector<uint32_t>> basisMembers;
        TVector<int32_t> candidateBases;
        candidateBases.resize( numCandidates, -1 );

        for ( uint32_t candidateIdx = 0; candidateIdx < numCandidates; candidateIdx++ )
        {
            BakeCandidate const& candidate = candidates[candidateIdx];
            TriangleProjection const& triangleProjection = triangleProjections[candidateIdx];

            int32_t bestBasisIndex = -1;
            float bestError = s_maxDecalSpaceError;

            for ( size_t basisIndex = 0; basisIndex < projectionBases.size(); ++basisIndex )
            {
                ProjectionBasis const& basis = projectionBases[basisIndex];
                if ( basis.m_decalIndex != candidate.m_decalIndex )
                {
                    continue;
                }

                float const error = ComputeProjectionError( candidate, triangleProjection, basis.m_linear );
                if ( error <= bestError )
                {
                    bestError = error;
                    bestBasisIndex = int32_t( basisIndex );
                }
            }

            if ( bestBasisIndex < 0 )
            {
                if ( projectionBases.size() > 0xFFFF )
                {
                    return Error::TooManyProjectionBases;
                }

                bestBasisIndex = int32_t( projectionBases.size() );

                ProjectionBasis& basis = projectionBases.emplace_back();
                basis.m_decalIndex = candidate.m_decalIndex;
                basis.m_linear = triangleProjection.m_linear;
                basis.m_anchorUV = triangleProjection.m_centroidUV;

                basisMembers.emplace_back();
            }

            candidateBases[candidateIdx] = bestBasisIndex;
            basisMembers[bestBasisIndex].emplace_back( candidateIdx );
        }

        // Refine each basis with a least squares fit over its members.
        // The basis is then anchored at the average member centroid.
        //-------------------------------------------------------------------------

        for ( size_t basisIndex = 0; basisIndex < projectionBases.size(); ++basisIndex )
        {
            ProjectionBasis& basis = projectionBases[basisIndex];
            TVector<uint32_t> const& members = basisMembers[basisIndex];

            float sumUU = 0.0F;
            float sumUV = 0.0F;
            float sumVV = 0.0F;
            float sumUTx = 0.0F;
            float sumVTx = 0.0F;
            float sumUTy = 0.0F;
            float sumVTy = 0.0F;

            for ( uint32_t memberIdx : members )
            {
                TriangleProjection const& triangleProjection = triangleProjections[memberIdx];
                Float4 const& exactLinear = triangleProjection.m_linear;

                for ( uint32_t cornerIdx = 0; cornerIdx < 3; cornerIdx++ )
                {
                    float const deltaU = candidates[memberIdx].m_uv[cornerIdx].m_x - triangleProjection.m_centroidUV.m_x;
                    float const deltaV = candidates[memberIdx].m_uv[cornerIdx].m_y - triangleProjection.m_centroidUV.m_y;

                    float const targetX = exactLinear.m_x * deltaU + exactLinear.m_y * deltaV;
                    float const targetY = exactLinear.m_z * deltaU + exactLinear.m_w * deltaV;

                    sumUU += deltaU * deltaU;
                    sumUV += deltaU * deltaV;
                    sumVV += deltaV * deltaV;
                    sumUTx += deltaU * targetX;
                    sumVTx += deltaV * targetX;
                    sumUTy += deltaU * targetY;
                    sumVTy += deltaV * targetY;
                }
            }

            float const determinant = sumUU * sumVV - sumUV * sumUV;
            if ( Math::Abs( determinant ) > 1e-12F )
            {
                float const inverseDeterminant = 1.0F / determinant;

                Float4 fittedLinear;
                fittedLinear.m_x = ( sumVV * sumUTx - sumUV * sumVTx ) * inverseDeterminant;
                fittedLinear.m_y = ( sumUU * sumVTx - sumUV * sumUTx ) * inverseDeterminant;
                fittedLinear.m_z = ( sumVV * sumUTy - sumUV * sumVTy ) * inverseDeterminant;
                fittedLinear.m_w = ( sumUU * sumVTy - sumUV * sumUTy ) * inverseDeterminant;

                float maxError = 0.0F;
                for ( uint32_t memberIdx : members )
                {
                    maxError = Math::Max( maxError, ComputeProjectionError( candidates[memberIdx], triangleProjections[memberIdx], fittedLinear ) );
                }

                if ( maxError <= s_maxDecalSpaceError )
                {
                    basis.m_linear = fittedLinear;
                }
            }

            Float2 anchorUV = Float2::Zero;
            for ( uint32_t memberIdx : members )
            {
                anchorUV += triangleProjections[memberIdx].m_centroidUV;
            }

            basis.m_anchorUV = anchorUV * ( 1.0F / float( members.size() ) );
        }

        //-------------------------------------------------------------------------

        GeometryBuilder::DecalBasisAttribute decalBasisAttribute;

        decalBasisAttribute.m_projectionBases.resize( projectionBases.size() );
        for ( size_t basisIndex = 0; basisIndex < projectionBases.size(); ++basisIndex )
        {
            ProjectionBasis const& basis = projectionBases[basisIndex];

            ShaderTypes::MeshDecalProjectionBasis& dstBasis = decalBasisAttribute.m_projectionBases[basisIndex];
            dstBasis.m_linear[0] = Math::FloatToHalf( basis.m_linear.m_x );
            dstBasis.m_linear[1] = Math::FloatToHalf( basis.m_linear.m_y );
            dstBasis.m_linear[2] = Math::FloatToHalf( basis.m_linear.m_z );
            dstBasis.m_linear[3] = Math::FloatToHalf( basis.m_linear.m_w );
            dstBasis.m_anchorUV[0] = basis.m_anchorUV.m_x;
            dstBasis.m_anchorUV[1] = basis.m_anchorUV.m_y;
        }

        decalBasisAttribute.m_projections.reserve( numCandidates );
        decalBasisAttribute.m_numMaterials = uint32_t( decals.size() );

        for ( uint32_t candidateIdx = 0; candidateIdx < numCandidates; candidateIdx++ )
        {
            BakeCandidate const& candidate = candidates[candidateIdx];
            TriangleProjection const& triangleProjection = triangleProjections[candidateIdx];
            ProjectionBasis const& basis = projectionBases[candidateBases[candidateIdx]];

            Float2 const anchorDelta = basis.m_anchorUV - triangleProjection.m_centroidUV;
            Float2 const anchorPosition = triangleProjection.m_centroidDecal + Float2
            (
                basis.m_linear.m_x * anchorDelta.m_x + basis.m_linear.m_y * anchorDelta.m_y,
                basis.m_linear.m_z * anchorDelta.m_x + basis.m_linear.m_w * anchorDelta.m_y
            );

            ShaderTypes::MeshDecalProjection& projection = decalBasisAttribute.m_projections.emplace_back();
            projection.m_basisIndex = uint16_t( candidateBases[candidateIdx] );
            projection.m_decalIndex = uint16_t( candidate.m_decalIndex );
            projection.m_offset[0] = Math::FloatToHalf( anchorPosition.m_x );
            projection.m_offset[1] = Math::FloatToHalf( anchorPosition.m_y );
        }

        //-------------------------------------------------------------------------

        uint32_t projectionIndex = 0;

        for ( uint32_t triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex )
        {
            uint32_t const numProjections = numTriangleCandidates[triangleIndex];
            if ( numProjections == 0 )
            {
                continue;
            }

            EE_ASSERT( projectionIndex <= ShaderTypes::g_meshDecalOffsetMask );
            geometryBuilder.SetDecalProjectionAttribute( triangleIndex, ( projectionIndex & ShaderTypes::g_meshDecalOffsetMask ) | ( numProjections << ShaderTypes::g_meshDecalCountShift ) );

            projectionIndex += numProjections;
        }

        EE_ASSERT( projectionIndex == numCandidates );

        geometryBuilder.SetDecalBasisAttribute( decalBasisAttribute );

        return Error::None;
    }
}
