#pragma once
#include "ProceduralBoneConstraint.h"
#include "Engine/Render/RenderMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API ProceduralBoneSettings
    {
        friend class ProceduralBoneSolver;
        friend class MeshLoader;

    private:

        SkeletalMesh const*                                 m_pMesh = nullptr;
        TVector<ProceduralBoneConstraint::Settings*>        m_constraintSettings;
        TVector<uint32_t>                                   m_constraintStartOffsets;
        size_t                                              m_constraintsRequiredMemory = 0;
        size_t                                              m_constraintsRequiredAlignment = 0;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ProceduralBoneSolver final
    {
    public:

        ProceduralBoneSolver( SkeletalMesh const* pMesh );
        ~ProceduralBoneSolver();

        void Solve( TVector<Transform>& parentSpacePose );

    private:

        ProceduralBoneSolver( ProceduralBoneSolver const& ) = delete;
        ProceduralBoneSolver& operator=( ProceduralBoneSolver const& ) = delete;

    private:

        SkeletalMesh const*                                 m_pMesh = nullptr;
        ProceduralBoneSettings const*                       m_pSettings = nullptr;
        uint8_t*                                            m_pAllocatedMemory = nullptr;
        TVector<ProceduralBoneConstraint*>                  m_constraints;
    };
} 