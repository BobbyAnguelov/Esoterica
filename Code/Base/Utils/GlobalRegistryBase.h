#pragma once
#include "Base\Esoterica.h"

//-------------------------------------------------------------------------

namespace EE
{
    template<typename T>
    class TGlobalRegistryBase
    {

    public:

        TGlobalRegistryBase()
        {
            // Add to global list
            if ( T::s_pHead != nullptr )
            {
                T::s_pTail->m_pNext = static_cast<T*>( this );
                T::s_pTail = static_cast<T*>( this );
            }
            else
            {
                T::s_pHead = static_cast<T*>( this );
                T::s_pTail = static_cast<T*>( this );
            }
        }

        ~TGlobalRegistryBase()
        {
            // Remove from global list
            //-------------------------------------------------------------------------

            // If we are the head of the list, just change the head to our next sibling
            if ( T::s_pHead == this )
            {
                T::s_pHead = m_pNext;

                // If we are also the tail, then empty the list
                if ( T::s_pTail == this )
                {
                    T::s_pTail = nullptr;
                }
            }
            else // Find our previous sibling
            {
                auto pPrevious = T::s_pHead;
                while ( pPrevious != nullptr )
                {
                    if ( pPrevious->m_pNext == this )
                    {
                        break;
                    }

                    pPrevious = pPrevious->m_pNext;
                }

                EE_ASSERT( pPrevious != nullptr );

                // Remove ourselves from the list
                pPrevious->m_pNext = m_pNext;

                // Update the tail of the list if needed
                if ( T::s_pTail == this )
                {
                    T::s_pTail = pPrevious;
                }
            }
        }

        TGlobalRegistryBase( TGlobalRegistryBase const& ) = default;
        TGlobalRegistryBase& operator=( TGlobalRegistryBase const& rhs ) = default;

    protected:

        inline T* GetNextItem() const { return m_pNext; }

    private:

        T* m_pNext = nullptr;
    };

    //-------------------------------------------------------------------------

    #define EE_GLOBAL_REGISTRY( T ) friend TGlobalRegistryBase<T>; inline static T* s_pHead = nullptr; inline static T* s_pTail = nullptr;
}