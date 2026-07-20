# AppendBuffer

An `AppendBuffer` lets shaders append elements to a dynamically-sized array on the GPU.

We use it for Debug Draw and instance picking — workloads where the output count is unknown at dispatch time and dropping data on overflow is acceptable.

## Why this exists

The GPU has no memory allocation interface — we cannot allocate or free memory from a shader. 

An append buffer bridges this: the GPU writes, the CPU monitors the counter and grows the buffer on overflow. The tradeoff is `RHI::MaxPendingFrames` of latency — writes silently fail until the CPU catches up. 

For best-effort systems, this is acceptable.

## GPU side

Two shader-side variants, both wrapping an atomic counter buffer and a data buffer:

| Variant           | Counter          | Data buffer             |
| ----------------- | ---------------- | ----------------------- |
| `AppendBuffer<T>` | `RWBuffer<uint>` | `RWStructuredBuffer<T>` |
| `RawAppendBuffer` | `RWBuffer<uint>` | `RWByteAddressBuffer`   |

The only write path is `TryAppend`. It calls `InterlockedAdd` on the counter to claim the next slot, then writes if the index is within capacity.

On overflow we silently drop the write without any fallbacks or error reports. `TryAppend` returns `false` for API consistency, but in practice we ignore it.

A subsequent pass that reads the counter must bounds-check: `m_counterBuffer[0]` counts all attempted writes, including those past capacity. The valid element count is `min( m_counterBuffer[0], m_capacity )`. When the data drives indirect dispatches, `CmdExecuteIndirect` handles this clamping — pass the counter as the counter buffer argument and `m_maxBufferSize` as `maxNumCommands`.

## Handle encoding

We pass the append buffer to the GPU as a single `uint64_t` handle:

| Bits  | Field                 |
| ----- | --------------------- |
| 0–15  | Counter buffer handle |
| 16–31 | Data buffer handle    |
| 32–63 | Data buffer capacity  |

`CreateAppendBuffer<T>()` unpacks the handle on the GPU to bind the counter and data buffers. The raw counter drives indirect dispatch — the Debug Draw resolve pass reads it to determine the command count.

## CPU side

`DeviceAppendBuffer<T>` manages the backing buffers. There is no automatic lifecycle — we call the update and draw methods explicitly. The per-frame pattern splits across two phases:

```cpp
class MyRenderPass
{
    DeviceAppendBuffer<SomeStruct>    m_buffer;

    void Initialize( RenderPassContext const& context )
    {
        m_buffer.Initialize( context.m_pContextRHI, "MyBuffer" );
    }

    void Shutdown( RenderSystem* pRenderSystem )
    {
        m_buffer.Shutdown( pRenderSystem->GetContextRHI() );
    }

    void UpdateDeviceResources( RenderSystem* pRenderSystem, uint32_t frameIndex )
    {
        // Read the counter from MaxPendingFrames back, grow if needed
        m_buffer.UpdateBuffers( pRenderSystem, frameIndex, sizeof( SomeStruct ), RHI::DescriptorTypeFlags::RWBuffer );
    }

    void Draw( DeviceResourceStates& states, RHI::CommandBuffer* pCommandBuffer, uint32_t frameIndex )
    {
        // 1. Zero the counter before appending dispatches
        m_buffer.Clear( states, pCommandBuffer, frameIndex );

        // ... submit dispatches that call TryAppend on the GPU ...

        // 2. Copy results to host readback buffers
        m_buffer.CopyResults( states, pCommandBuffer, frameIndex );

        // 3. Transition host buffers so the CPU can read them next frame
        m_buffer.Barrier( states, pCommandBuffer, frameIndex );
    }
};
```

`UpdateBuffers` reads the counter from `RHI::MaxPendingFrames` frames back. If it exceeded capacity we allocate a larger buffer and queue the old one for deferred deletion. For typed buffers we also copy the readback data into `m_bufferData`. Capacity (`m_maxBufferSize`) is monotonic — it grows to meet peak demand and never shrinks.

`DeviceAppendBuffer<void>` skips the data readback entirely — we use it when only the element count matters.

## Latency tradeoff

Overflow is not detected for `RHI::MaxPendingFrames` — writes keep failing until the CPU reads the counter and allocates a larger buffer. The CPU never stalls.

This is acceptable for Debug Draw and picking, which are best-effort by nature.

## Shader example

Declare the append buffer in a resource table, unpack at the top of the entry point:

```c
ESF_RESOURCE_TABLE struct MyComputeResources
{
    AppendBuffer<MyResult>    m_results;
};

[numthreads( 64, 1, 1 )]
void CS_main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    MyComputeResources resources = CreateMyComputeResources( RootConstants );
    AppendBuffer<MyResult> results = CreateAppendBuffer<MyResult>( resources.m_results );

    // ... compute something ...

    results.TryAppend( result );  // return value ignored — nothing to do on failure
}
```

**Use caution in pixel shaders.** Invocation counts reach millions easily; an unbounded append per pixel will exhaust memory and crash. Prefer compute or mesh shaders.

## Consumers

**Debug Draw** — two depth-test buckets backed by `DeviceAppendBuffer<void>`, filled by mesh shaders. The resolve pass reads the counter for indirect draw arguments.

**Instance picking** — stores per-hit results in `DeviceAppendBuffer<PickingResult>`. The CPU resolves hits from `m_bufferData` from the first available frame.

## When to use

Append buffers are the right tool when the output element count is unknown at dispatch time and data loss on overflow is acceptable. They work best when the output is consumed on the GPU — indirect draw, further compute — and the CPU only needs the count or a best-effort copy.

They are not a good fit when the count is known upfront (use a fixed-size buffer), when data loss is unacceptable, or when the GPU must know the final count within the same dispatch.
