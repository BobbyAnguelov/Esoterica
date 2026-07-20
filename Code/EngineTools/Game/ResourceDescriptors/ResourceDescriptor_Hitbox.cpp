#include "ResourceDescriptor_Hitbox.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool HitboxResourceDescriptor::IsValid() const
    {
        return true;
    }

    void HitboxResourceDescriptor::Clear()
    {
        m_shapes.clear();
    }
}