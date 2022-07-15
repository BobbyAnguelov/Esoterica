#include "PhysicsMaterial.h"
#include "System/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool PhysicsMaterialSettings::IsValid() const
    {
        static StringID const defaultMaterialID( PhysicsMaterial::DefaultID );

        if ( !m_ID.IsValid() || m_ID == StringID( defaultMaterialID ) )
        {
            return false;
        }

        if ( !Math::IsInRangeInclusive( m_staticFriction, 0.0f, FLT_MAX ) || !Math::IsInRangeInclusive( m_dynamicFriction, 0.0f, FLT_MAX ) )
        {
            return false;
        }

        return Math::IsInRangeInclusive( m_restitution, 0.0f, 1.0f );
    }
}