#pragma once
#include "Base/Types/Arrays.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct IKChainSolver
    {
        struct ChainNode
        {
            Transform   m_transform;
            float       m_weight;
        };

        // The nodes array is both input and output
        // Pivot index: prerotate entire chain around this point (one of the nodes) (optional)
        // Allowed stretch: can we stretch the bones and by how much. <= 0.0f means no stretch allowed
        // Stiffness value: [0, 1]
        // Max Iterations: [1, 20]
        static bool SolveChain( TInlineVector<ChainNode, 10>& nodes, Transform const& targetTransform, int32_t pivotIndex = InvalidIndex, float allowedStretch = 0.0f, float stiffness = 1.0f, int32_t maxIterations = 6 );
    };
}