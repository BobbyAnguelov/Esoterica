#if _WIN32
#include "Base/Threading/Threading.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

//-------------------------------------------------------------------------

namespace EE
{
    namespace Threading
    {
        ProcessorInfo GetProcessorInfo()
        {
            DWORD requiredSize = 0;
            GetLogicalProcessorInformation( nullptr, &requiredSize );
            int32_t const numEntries = requiredSize / sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );

            auto pProcInfos = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>( alloca( sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION ) * numEntries ) );
            bool const result = GetLogicalProcessorInformation( pProcInfos, &requiredSize );
            EE_ASSERT( result );

            //-------------------------------------------------------------------------

            auto CountSetBits = [] ( ULONG_PTR bitMask )
            {
                DWORD LSHIFT = sizeof( ULONG_PTR ) * 8 - 1;
                DWORD bitSetCount = 0;
                ULONG_PTR bitTest = (ULONG_PTR) 1 << LSHIFT;
                DWORD i;

                for ( i = 0; i <= LSHIFT; ++i )
                {
                    bitSetCount += ( ( bitMask & bitTest ) ? 1 : 0 );
                    bitTest /= 2;
                }

                return bitSetCount;
            };

            //-------------------------------------------------------------------------

            ProcessorInfo procInfo;
            for ( auto i = 0; i < numEntries; i++ )
            {
                if ( pProcInfos[i].Relationship == RelationProcessorCore )
                {
                    procInfo.m_numPhysicalCores++;
                    procInfo.m_numLogicalCores += (uint16_t) CountSetBits( pProcInfos[i].ProcessorMask );
                }
            }

            return procInfo;
        }

        //-------------------------------------------------------------------------

        ThreadID GetCurrentThreadID()
        {
            auto pNativeThreadHandle = GetCurrentThread();
            ThreadID const nativeThreadID = GetThreadId( pNativeThreadHandle );
            return nativeThreadID;
        }

        void SetCurrentThreadName( char const* pName )
        {
            EE_ASSERT( pName != nullptr );
            wchar_t wThreadName[255];
            auto pNativeThreadHandle = GetCurrentThread();
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pName, -1, wThreadName, 255 );
            SetThreadDescription( pNativeThreadHandle, wThreadName );
        }

        //-------------------------------------------------------------------------

        SyncEvent::SyncEvent()
            : m_pNativeHandle( nullptr )
        {
            m_pNativeHandle = CreateEvent( nullptr, TRUE, FALSE, nullptr );
            EE_ASSERT( m_pNativeHandle != nullptr );
        }

        void SyncEvent::Signal()
        {
            EE_ASSERT( m_pNativeHandle != nullptr );
            auto result = SetEvent( m_pNativeHandle );
            EE_ASSERT( result );
        }

        void SyncEvent::Reset()
        {
            EE_ASSERT( m_pNativeHandle != nullptr );
            auto result = ResetEvent( m_pNativeHandle );
            EE_ASSERT( result );
        }

        void SyncEvent::Wait() const
        {
            EE_ASSERT( m_pNativeHandle != nullptr );
            WaitForSingleObject( m_pNativeHandle, INFINITE );
        }

        void SyncEvent::Wait( Milliseconds maxWaitTime ) const
        {
            EE_ASSERT( m_pNativeHandle != nullptr );
            WaitForSingleObject( m_pNativeHandle, (uint32_t) maxWaitTime );
        }
    }
}
#endif