#include "IKChainSolver.h"
#include "Base/Math/Lerp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct ChainLink
    {
        Vector radialVector;
        float m_length;
        float m_weight1;
        float m_weight2;
    };

    //-------------------------------------------------------------------------

    bool IKChainSolver::SolveChain( TInlineVector<ChainNode, 10>& nodes, Transform const& targetTransform, int32_t pivotIndex, float allowedStretch, float stiffness, int32_t maxIterations )
    {
        Vector targetPosition = targetTransform.GetTranslation();

        int32_t nodeCount = (int32_t) nodes.size();
        EE_ASSERT( nodeCount >= 2 );

        TInlineVector<Vector, 10> points;
        points.resize( nodeCount );

        for ( int32_t i = 0; i < nodeCount; ++i )
        {
            points[i] = nodes[i].m_transform.GetTranslation();
        }

        int32_t linkCount = nodeCount - 1;
        TInlineVector<ChainLink,9> links;
        links.resize( linkCount );

        for ( int32_t linkIdx = 0; linkIdx < linkCount; ++linkIdx )
        {
            links[linkIdx].radialVector = points[linkIdx + 1] - points[linkIdx];
            links[linkIdx].m_length = links[linkIdx].radialVector.GetLength3();
            if ( links[linkIdx].m_length < 0.001f )
            {
                // Link m_length less than 1mm
                return false;
            }

            if ( linkIdx == 0 )
            {
                links[linkIdx].m_weight1 = 0.0f;
                links[linkIdx].m_weight2 = 1.0f;
            }
            else if ( linkIdx == linkCount - 1 )
            {
                links[linkIdx].m_weight1 = 1.0f;
                links[linkIdx].m_weight2 = 0.0f;
            }
            else
            {
                float weight1 = nodes[linkIdx].m_weight;
                float weight2 = nodes[linkIdx + 1].m_weight;
                float totalWeight = weight1 + weight2;
                float invTotalWeight = totalWeight > 0.0f ? 1.0f / totalWeight : 0.0f;
                links[linkIdx].m_weight1 = weight1 * invTotalWeight;
                links[linkIdx].m_weight2 = weight2 * invTotalWeight;
            }
        }

        // Pre-rotate the entire chain about the supplied pivot
        // and scale so it reaches the target exactly. This becomes
        // the starting configuration for the solver.
        if ( 0 <= pivotIndex && pivotIndex < nodeCount - 1 )
        {
            Vector pivot = points[pivotIndex];
            Vector radialVector1 = points[nodeCount - 1] - pivot;
            Vector radialVector2 = targetPosition - pivot;

            float length1 = radialVector1.GetLength3();
            float length2 = radialVector2.GetLength3();

            if ( length1 < 0.001f || length2 < 0.001f )
            {
                return false;
            }

            Quaternion quat = Quaternion::FromRotationBetweenVectors( radialVector1, radialVector2 );
            float scale = length1 / length2;

            for ( int32_t i = pivotIndex + 1; i < nodeCount; ++i )
            {
                Vector radialVector = points[i] - pivot;
                Vector PointTarget = quat.RotateVector( radialVector * scale ) + pivot;

                // Apply pre-rotation scaled by stiffness
                points[i] = Math::Lerp( points[i], PointTarget, stiffness );
            }
        }

        points[nodeCount - 1] = targetPosition;

        // Iterative solver, similar to position based dynamics.
        for ( int32_t iteration = 0; iteration < maxIterations; ++iteration )
        {
            for ( int32_t linkIdx = 0; linkIdx < linkCount; ++linkIdx )
            {
                Vector point1 = points[linkIdx];
                Vector point2 = points[linkIdx + 1];
                Vector delta = point2 - point1;

                // Add a small value to avoid divide by zero
                float currentLength = delta.GetLength3() + 0.0001f;

                ChainLink const& link = links[linkIdx];
                float stretch = ( currentLength - link.m_length ) / currentLength;
                Vector impulse = Vector( -stiffness * stretch ) * delta;

                points[linkIdx] = point1 - ( impulse * link.m_weight1 );
                points[linkIdx + 1] = point2 + ( impulse * link.m_weight2 );
            }
        }

        EE_ASSERT( 0.0f <= allowedStretch && allowedStretch <= 1.0f );

        // Cinch to allowed stretch, starting at the base
        for ( int32_t linkIdx = 0; linkIdx < linkCount; ++linkIdx )
        {
            Vector delta = points[linkIdx + 1] - points[linkIdx];
            float currentLength = delta.GetLength3();
            if ( currentLength < 0.0001f )
            {
                continue;
            }

            ChainLink const& link = links[linkIdx];

            float minLength = link.m_length - allowedStretch * link.m_length;
            float maxLength = link.m_length + allowedStretch * link.m_length;
            float length = Math::Clamp( currentLength, minLength, maxLength );
            points[linkIdx + 1] = points[linkIdx] + ( delta * ( length / currentLength ) );
        }

        for ( int32_t linkIdx = 0; linkIdx < linkCount; ++linkIdx )
        {
            Vector radialVector = Vector( points[linkIdx + 1] - points[linkIdx] );
            Quaternion deltaQuat = Quaternion::FromRotationBetweenVectors( links[linkIdx].radialVector, radialVector );
            Quaternion quat = nodes[linkIdx].m_transform.GetRotation();
            quat = quat * deltaQuat;
            nodes[linkIdx].m_transform.SetRotation( quat );
            nodes[linkIdx + 1].m_transform.SetTranslation( points[linkIdx + 1] );
        }

        // Snap effector rotation to target
        nodes[nodeCount - 1].m_transform.SetRotation( targetTransform.GetRotation() );

        return true;
    }
}