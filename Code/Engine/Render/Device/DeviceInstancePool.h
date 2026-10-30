#pragma once

#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Base/Render/HandleAllocator.h"
#include "Base/Render/PageAllocator.h"
#include "Base/Render/RHI.h"
#include "Base/Memory/Memory.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Profiling.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    template<typename T>
    class CopyInstanceBufferDataTask final : public ITaskSet
    {
    public:

        //-------------------------------------------------------------------------

        inline CopyInstanceBufferDataTask() = default;

        inline CopyInstanceBufferDataTask( CopyInstanceBufferDataTask const& ) = delete;
        inline CopyInstanceBufferDataTask& operator=( CopyInstanceBufferDataTask const& ) = delete;

        inline CopyInstanceBufferDataTask( CopyInstanceBufferDataTask&& other ) noexcept
            : m_pSrcMemory( other.m_pSrcMemory )
            , m_pDstMemory_WriteCombined( other.m_pDstMemory_WriteCombined )
        {
            other.m_pSrcMemory = nullptr;
            other.m_pDstMemory_WriteCombined = nullptr;
        }

        inline CopyInstanceBufferDataTask& operator=( CopyInstanceBufferDataTask&& other ) noexcept
        {
            m_pSrcMemory = other.m_pSrcMemory;
            m_pDstMemory_WriteCombined = other.m_pDstMemory_WriteCombined;

            other.m_pSrcMemory = nullptr;
            other.m_pDstMemory_WriteCombined = nullptr;

            return *this;
        }

    private:

        //-------------------------------------------------------------------------

        virtual void ExecuteRange( TaskSetPartition range, uint32_t threadIndex ) override
        {
            EE_PROFILE_SCOPE_RENDER( "CopyInstanceBufferDataTask" );

            Memory::CopyToWriteCombined
            (
                m_pDstMemory_WriteCombined + range.start,
                m_pSrcMemory + range.start,
                ( range.end - range.start ) * sizeof( T )
            );
        }

    public:

        //-------------------------------------------------------------------------

        T const*                                            m_pSrcMemory = nullptr;
        T*                                                  m_pDstMemory_WriteCombined = nullptr;
    };

    //-------------------------------------------------------------------------

    template <typename T>
    struct InstanceUpdateCommandsPool final
    {
    public:

        inline InstanceUpdateCommandsPool() = default;

        inline InstanceUpdateCommandsPool( InstanceUpdateCommandsPool const& ) = delete;
        inline InstanceUpdateCommandsPool& operator=( InstanceUpdateCommandsPool const& ) = delete;

        inline InstanceUpdateCommandsPool( InstanceUpdateCommandsPool&& other ) noexcept
            : m_counter( other.m_counter.load() )
            , m_sequence( other.m_sequence )
            , m_memoryPool( eastl::move( other.m_memoryPool ) )
            , m_numUpdateCommands( other.m_numUpdateCommands )
        {}

        inline InstanceUpdateCommandsPool& operator=( InstanceUpdateCommandsPool&& other ) noexcept
        {
            m_counter.store( other.m_counter.load() );
            m_sequence = other.m_sequence;
            m_memoryPool = eastl::move( other.m_memoryPool );
            m_numUpdateCommands = other.m_numUpdateCommands;
            return *this;
        }

        inline void Initialize()
        {
            m_memoryPool.Initialize( 1 );
        }

        inline void Shutdown()
        {
            m_memoryPool.Shutdown();
        }

        inline void Update()
        {
            EE_ASSERT( m_numUpdateCommands == 0 );

            m_numUpdateCommands = m_counter.exchange( 0 );
            m_sequence++;
        }

        inline void Submit()
        {
            m_numUpdateCommands = 0;
        }

        //-------------------------------------------------------------------------

    public:

        eastl::atomic<uint32_t>                             m_counter = 0;
        uint64_t                                            m_sequence = 0;
        PageMemoryPool<T>                                   m_memoryPool = {};
        uint32_t                                            m_numUpdateCommands = 0;
    };

    //-------------------------------------------------------------------------

    template <typename TInitializeCommand>
    struct DeviceInstanceInitializeQueue final
    {
    public:

        using InitializeCommand = TInitializeCommand;

        inline void QueueCommand( TInitializeCommand const& command )
        {
            m_commands.emplace_back( command );
        }

        inline void Clear()
        {
            m_commands.clear();
        }

    public:

        //-------------------------------------------------------------------------

        // TODO: Get rid of intermediate buffers and write directly to mapped buffers
        TAlignedVector<TInitializeCommand>                  m_commands;
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_buffers = {};
        CopyInstanceBufferDataTask<TInitializeCommand>      m_copyCommands;
    };

    //-------------------------------------------------------------------------

    template <typename TInstance, typename TAllocator = HandleAllocator<uint32_t>>
    struct DeviceInstanceStorage final
    {
    public:

        using Handle = typename TAllocator::Handle;

        template <typename TCount>
        inline Handle Allocate( TCount count )
        {
            return m_allocator.Allocate( count );
        }

        inline void Deallocate( Handle&& handle )
        {
            m_allocator.Deallocate( eastl::move( handle ) );
        }

        inline uint32_t GetCapacityInPages() const { return m_allocator.GetCapacityInPages(); }
        inline uint32_t GetCapacity() const { return GetCapacityInPages() * 64; }
        inline size_t GetInstanceBufferSize() const { return size_t( GetCapacity() ) * sizeof( TInstance ); }

        inline void UpdatePageBuffer( uint32_t frameIndex )
        {
            Memory::CopyToWriteCombined
            (
                m_pageBuffers[frameIndex].m_pBuffer->m_pMappedAddress_WriteCombined,
                m_allocator.GetPageData(),
                GetCapacityInPages() * sizeof( uint64_t )
            );
        }

        //-------------------------------------------------------------------------

    public:

        TAllocator                                          m_allocator = {};
        DeviceResizeBuffer                                  m_instanceBuffer = {};

        // TODO: Use QueueBufferUpdate API instead of triple buffering it, should save a lot of memory
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>   m_pageBuffers = {};
    };

    //-------------------------------------------------------------------------

    template <typename TUpdateCommand>
    struct DeviceInstanceUpdateCommands final
    {
    public:

        using UpdateCommand = TUpdateCommand;

        inline void Initialize() { m_updatePool.Initialize(); }
        inline void Shutdown() { m_updatePool.Shutdown(); }
        inline void Update() { m_updatePool.Update(); }
        inline void Submit() { m_updatePool.Submit(); }

        //-------------------------------------------------------------------------

    public:

        InstanceUpdateCommandsPool<TUpdateCommand>              m_updatePool = {};

        // TODO: Need some kind of scratch GPU memory allocator to avoid tracking all these buffers
        TArray<DeviceResizeBuffer, RHI::MaxPendingFrames>       m_updateBuffers = {};
        CopyInstanceBufferDataTask<TUpdateCommand>              m_copyUpdateCommands;
    };

    //-------------------------------------------------------------------------

    template <typename TInstance, typename TUpdateCommand, typename TAllocator = HandleAllocator<uint32_t>>
    struct DeviceInstancePool final
    {
    public:

        template <typename TCount>
        inline typename TAllocator::Handle Allocate( TCount count )
        {
            typename TAllocator::Handle handle = m_storage.Allocate( count );

            m_updateCommands.m_updatePool.m_memoryPool.Commit( m_storage.GetCapacity() );

            return handle;
        }

        //-------------------------------------------------------------------------

    public:

        DeviceInstanceStorage<TInstance, TAllocator>            m_storage = {};
        DeviceInstanceUpdateCommands<TUpdateCommand>            m_updateCommands = {};
    };
}
