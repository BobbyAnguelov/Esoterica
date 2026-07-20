#include "PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool CollisionMesh::IsValid() const
    {
        return !m_shapeData.empty();
    }
}