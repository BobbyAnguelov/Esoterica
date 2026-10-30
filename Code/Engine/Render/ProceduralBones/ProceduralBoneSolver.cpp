#include "ProceduralBoneSolver.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    ProceduralBoneSolver::ProceduralBoneSolver( SkeletalMesh const* pMesh )
        : m_pMesh( pMesh )
        , m_pSettings( pMesh->GetProceduralBoneSettings() )
    {
        EE_ASSERT( m_pSettings != nullptr );

        size_t const numConstraints = m_pSettings->m_constraintStartOffsets.size();
        EE_ASSERT( m_pSettings->m_constraintSettings.size() == numConstraints );

        m_pAllocatedMemory = reinterpret_cast<uint8_t*>( EE::Alloc( m_pSettings->m_constraintsRequiredMemory, m_pSettings->m_constraintsRequiredAlignment ) );
        m_constraints.reserve( numConstraints );

        for ( size_t i = 0; i < numConstraints; i++ )
        {
            m_constraints.emplace_back( reinterpret_cast<ProceduralBoneConstraint*>( m_pAllocatedMemory + m_pSettings->m_constraintStartOffsets[i] ) );
            m_pSettings->m_constraintSettings[i]->CreateConstraint( pMesh, m_constraints[i] );
        }
    }

    ProceduralBoneSolver::~ProceduralBoneSolver()
    {
        for ( ProceduralBoneConstraint* pConstraint : m_constraints )
        {
            pConstraint->~ProceduralBoneConstraint();
        }
        m_constraints.clear();

        EE::Free( m_pAllocatedMemory );
    }

    void ProceduralBoneSolver::Solve( TVector<Transform>& parentSpacePose )
    {
        for ( auto pConstraint : m_constraints )
        {
            pConstraint->Solve( m_pMesh, parentSpacePose );
        }
    }
}