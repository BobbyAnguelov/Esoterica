#include "PhysX.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    Float3 const Constants::s_gravity = Float3( 0, 0, -9.81f );

    physx::PxConvexMesh* SharedMeshes::s_pUnitCylinderMesh = nullptr;
}