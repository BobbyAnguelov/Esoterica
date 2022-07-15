#include "PhysicsMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool PhysicsMesh::IsValid() const
    {
        return m_pMesh != nullptr;
    }
}