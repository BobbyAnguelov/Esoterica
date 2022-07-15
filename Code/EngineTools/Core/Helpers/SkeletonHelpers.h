#pragma once
#include "System/Memory/Memory.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct BoneInfo
    {
        inline void DestroyChildren()
        {
            for ( auto& pChild : m_children )
            {
                pChild->DestroyChildren();
                EE::Delete( pChild );
            }

            m_children.clear();
        }

    public:

        int32_t                         m_boneIdx;
        TInlineVector<BoneInfo*, 5>     m_children;
        bool                            m_isExpanded = true;
    };
}