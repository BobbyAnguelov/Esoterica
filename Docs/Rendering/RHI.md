# RHI

The RHI is a thin, opinionated abstraction layer over modern graphics APIs (Direct3D 12 and Vulkan).

Its core job is to provide a simple and portable interface.

"Portable" means the abstractions are implementable on other graphics APIs without requiring changes to engine code. There is no `#ifdef` chains or per-platform rendering paths anywhere.

It targets the hardware that users actually run today — not 15 years old legacy GPUs, not the highest-end enthusiast stuff. Only the features that match that practical hardware range are exposed.

The Vulkan backend is not implemented yet.

## Design Philosophy

The RHI was inspired by [The-Forge](https://github.com/ConfettiFX/The-Forge) but adopts different trade-offs that match the engine's goals. We're slowly moving towards [no graphics api](https://www.sebastianaaltonen.com/blog/no-graphics-api) direction where possible.

The key principles:

### HLSL only, always

All shaders are written in HLSL 2021 regardless of backend. A shared header `RHI.esh` provides macros and utilities so that shader code stays backend-agnostic.

### No legacy API support

There is no D3D11, OpenGL, or any pre-D3D12/Vulkan path. The minimum D3D feature level is 12_0.

This lets bindless resources be used everywhere and keeps backend implementations simple.

### No geometry or tessellation shaders

Mesh shaders replace geometry shaders entirely. If you need tessellation, do it in a compute shader.

The available pipeline combinations are vertex + pixel, mesh + pixel, and compute.

### No built-in shader compiler

There is no built-in shader compiler, pre-compiled binary bytecode blobs are consumed directly. Actual shader compilation happens on the engine side.

Shader reflection data is extracted from the bytecode and surfaced to the engine through `ShaderReflection` and `PipelineReflection`.

### Bindless by default, SM 6.6 minimum

Shader Model 6.6 is required. Every shader has access to all resources in the application through integer handles rather than descriptor sets.

Per-draw descriptor tables are never managed on the CPU. Instead, integer handles are written into buffers or root constants. 

Shaders read those handles with macros from `RHI.esh`.

### Externally synchronized

There are no internal mutexes or shared global state, all synchronization is the caller's responsibility

This keeps overhead near zero and gives the engine full control over its threading model.

### Sane defaults in parameter structs

Every `XXXParameters` struct (e.g., `TextureParameters`, `BufferParameters`) initializes all fields to sensible values. 

For example, `TextureParameters` sets `m_width`, `m_height`, `m_depth`, and`m_arrayLayers` to 1 by default. If you need a 2D texture, you only override width and height.

### Dangerous operations are exposed, not hidden

Functions like texture-to-buffer copies are part of the public API. Callers are trusted to know what they're doing.

Log warnings are emitted for a small subset of operations. These are not fatal errors, but they indicate minor correctness or performance issues.

We don't want to get in the way of debugging, but we do expect all warnings to be fixed before code is submitted.

We highly recommend enabling both host and device validation in debug builds. These catch a wide range of usage errors and provide actionable diagnostics.

### Resource loader lives in the engine, not the RHI

The RHI provides Create/Destroy functions for buffers and textures but does not include an asynchronous resource loading system.

The engine layer has more context about resource dependencies and pipeline stalls, so the loader is implemented there.

Similarly, there is no way to specify initial data for textures or buffers — the caller has to allocate staging memory, copy data to it, and issue a GPU copy command.

## Architecture Overview

The RHI is organized around a few central types. The relationships are:

```
Context
 ├── Queue (Graphics / Compute / Transfer)
 │    ├── CommandPool
 │    │    └── CommandBuffer
 │    └── Swapchain
 ├── Buffer
 ├── Texture
 ├── Sampler
 ├── Shader
 ├── RootSignature
 ├── Pipeline
 ├── PipelineCache
 ├── CommandSignature
 └── QueryPool
```

Nearly every Create function takes a `Context*` — the context owns the device, its capabilities are queried from `Context::m_deviceCapabilities`.

The RHI follows Vulkan terminology and concepts. If you are familiar with Vulkan, most of it transfers directly.

Use your Vulkan tutorial of choice as supplementary reading.

## Object Lifecycle

Every resource type follows the same create/destroy convention.

Destroy functions take the resource pointer by rvalue reference. They null the pointer after destruction, so the caller's pointer is invalidated.

`Context` must outlive all resources created from it: create it first, destroy it last.

## Context and Device Management

`Context` represents a logical GPU device. It is created from `ContextParameters`.

`ContextParameters` selects the shader model, the device mode (`Single`, `Linked`, or `Unlinked`), and which GPU to use. It also carries validation and debugging flags.

See [Multi-GPU and Linked Adapters](#multi-gpu-and-linked-adapters) for details on multi-GPU support.

After creation, `Context::m_deviceCapabilities` (`DeviceCapabilities`) contains full information about the physical device. This includes video memory, wave lane count, supported shader rates, format support, and more.

Caps are checked at runtime — nothing is hardcoded to a specific feature level.

`ContextParameters` has fields for validation, debugging, and tool integration — see [Debugging and Validation](#debugging-and-validation)  section for details.

## Bindless Resource System

The RHI uses a global bindless resource model.

Command buffers automatically bind both global descriptor pools when recording starts.

Instead of binding descriptor sets for each draw, shaders reference resources through integer handles (aliased as `GenericResourceHandle = uint16_t`). See [Resource Handles](#resource-handles) below.

There are no vertex buffer bindings or input layouts. Vertex data is passed through raw or structured buffers. Vertex offsets are passed as integer parameters to root constants or buffers.

Index buffers still exist. The fixed-function primitive assembler uses them to deduplicate vertex shader invocations when mesh shaders are not used; this is still important on modern GPUs.

### Resource Handles

The RHI exposes three resource types: Sampler, Buffer, and Texture.

Internally, each handle is an index into a global descriptor heap owned by the `Context`.

Shaders index this heap directly using the handle value (`SamplerDescriptorHeap` for samplers, `ResourceDescriptorHeap` for buffers and textures in HLSL).

There is no concept of a "resource view" like in D3D12. Handles are obtained directly from the resource object, not from a separate view object. The handle type determines how the resource is accessed in the shader.

Handles share the lifetime of the resource — they are allocated when the resource is created and freed when it is destroyed.

The shader receives handles via root constants or buffers. Macros from `RHI.esh` turn those integer handles into the corresponding HLSL resource types.

#### Handle-to-HLSL mapping

When a resource is created, it allocates `N` handles. For each handle a corresponding resource view is created — on D3D12 this means a `ShaderResourceView`, `UnorderedAccessView`, or similar, depending on the requested `DescriptorTypeFlags` and other parameters.

The table below shows which HLSL type results from each flag combination:

| Resource | `DescriptorTypeFlags` | HLSL type                                                                        |
| -------- | --------------------- | -------------------------------------------------------------------------------- |
| Sampler  | `Sampler`             | `SamplerState`                                                                   |
| Texture  | `Texture`             | `Texture2D<T>`, `Texture3D<T>`, etc. (and multisample variants)                  |
| Texture  | `TextureCube`         | `TextureCube<T>`, `TextureCubeArray<T>`                                          |
| Texture  | `RWTexture`           | `RWTexture2D<T>`, `RWTexture3D<T>`, etc.                                         |
| Buffer   | `Buffer`              | `Buffer<T>` if `m_format` is set, `StructuredBuffer<T>` if `m_stride` is set     |
| Buffer   | `ConstantBuffer`      | `ConstantBuffer<T>`                                                              |
| Buffer   | `RWBuffer`            | `RWBuffer<T>` if `m_format` is set, `RWStructuredBuffer<T>` if `m_stride` is set |
| Buffer   | `Buffer` + `Raw`      | `ByteAddressBuffer`                                                              |
| Buffer   | `RWBuffer` + `Raw`    | `RWByteAddressBuffer`                                                            |

`IndexBuffer`, `IndirectArgumentBuffer`, `RootConstant` and `RenderTarget` are also valid descriptor types, but they are not directly shader-visible — the table above covers only the types a shader sees in HLSL.

Sampler handles always map to `SamplerState` — there is only one sampler type.

For buffers, the HLSL type is determined by `BufferParameters`:

- If `m_format` is set — typed `Buffer<T>` where `T` is a built-in HLSL type (`int`, `uint`, `float3`, etc., not a struct).
- If `m_stride` is set — `StructuredBuffer<T>` where `T` is a user-defined struct.
- If the `Raw` flag is set — `ByteAddressBuffer` (overrides both `m_format` and `m_stride`).

For textures, the HLSL type is determined by `DescriptorTypeFlags` and the texture dimensions in `TextureParameters`:

- `Texture` is for regular textures and array textures. The dimension is chosen from `TextureParameters`: `Texture1D<T>` when `m_height == 1`, `Texture2D<T>`when `m_depth == 1`, `Texture3D<T>` otherwise. Multisample variants are supported.
- `TextureCube` is specifically for cubemaps and cubemap arrays — to create a cubemap, set `m_arrayLayers` to 6. The HLSL type is `TextureCube<T>` or `TextureCubeArray<T>`.
- `RWTexture` maps to `RWTexture1D<T>`, `RWTexture2D<T>`, or `RWTexture3D<T>`using the same dimension logic as `Texture`. It also requires a separate view for each mip level because the underlying API does not allow shaders to write
to arbitrary mip levels using an index. Pass the desired mip to `rwTextureMipLevel` in the corresponding `GetTextureHandle` call to obtain the handle for that level.
- `RWTextureCube` does not exist in HLSL — a cubemap with `RWTexture` maps to `RWTexture2DArray<T>` instead.

There is no compile-time validation that a handle matches its expected HLSL type. If a shader uses the wrong handle the result is undefined — enable both host and device validation to catch these mismatches at runtime.

## Buffer Sub-Allocation

Buffers created with the `SubAllocations` flag support carving out regions at runtime through `BufferSubAllocate` and `BufferSubDeallocate`. This is primarily used for staging-buffer management — the engine owns one large upload buffer and sub-allocates transient regions for per-frame resource updates.

Buffer sub-allocation uses TLSF under the hood, this is the right trade-off for staging buffers because offset has no impact on GPU throughput. See [HandleAllocator](HandleAllocator.md) for more details and tradeoffs

### API

```cpp
struct BufferSubAllocation
{
    uint64_t m_offset  = ~0ULL;  // Byte offset into the parent buffer
    uint64_t m_internal = ~0ULL; // Opaque backend handle, do not use directly

    bool IsValid() const;
};

BufferSubAllocation BufferSubAllocate( Buffer* pBuffer, uint64_t size, uint64_t alignment );
void                BufferSubDeallocate( Buffer* pBuffer, BufferSubAllocation&& subAllocation );
```

`BufferSubAllocate` returns a valid-but-zero-sized allocation on failure (out of memory). The caller must check `IsValid()` before using the result.

`BufferSubDeallocate` takes the allocation by rvalue reference — the caller's `BufferSubAllocation` is invalidated after the call.

The sub-allocated offset is relative to the parent buffer's base. It can be passed directly to`CmdSetRootParameter` as a buffer offset, or used as the source/destination offset in copy commands.

## Synchronization

### Timeline semaphores

Synchronization uses timeline semaphores — monotonically increasing `uint64_t` values, one per queue.

There is no separate semaphore or fence object. Every queue owns an internal timeline that advances with each submission.

`QueueSubmit` returns the semaphore value this submission will signal when all its command buffers finish executing on the GPU. The caller stores that value and uses it later to wait.

`QueueHostWait` blocks the calling CPU thread until the queue reaches the given value. Use it when the CPU needs a GPU result before continuing — reading back a query, mapping a buffer, or waiting for pending GPU submission.

`QueueDeviceWait` inserts a GPU-side wait: one queue stalls until another queue reaches the given semaphore value. This is the primary tool for cross-queue dependencies — a compute queue waiting on a graphics queue depth pass, or a graphics queue waiting on a compute queue post-process.

`QueueGetCurrentSemaphore` returns the next value the queue will signal — useful for predicting the value of an upcoming submission before calling `QueueSubmit` .

`QueueGetCompletedSemaphore`returns the value that is completed on the GPU at the time of the call.

### Frame pacing

The `MaxPendingFrames` limit controls how many frames can be in flight. It is set to 2 by default (double buffering). To switch to triple buffering, change this to 3.

Double buffering is the default because it minimizes input latency. A value of 3 is possible but not officially supported — we do not test this configuration.

### Barriers

The barrier API follows the D3D12 Enhanced Barriers model, which is also compatible with Vulkan pipeline barriers. 

Three enums define a barrier: `PipelineStage` (which stage before and after), `ResourceAccess` (read/write access flags), and `TextureState` (the resource state being transitioned to or from).

`TextureBarrierRegion` can optionally specify the subresource range; setting `m_numMipLevels` or `m_numArraySlices` to 0 means "the entire texture."

## Debugging and Validation

Several layers of debugging support are available, controlled at context creation time:

| Context parameter           | What it does                                                                            |
| --------------------------- | --------------------------------------------------------------------------------------- |
| `m_enableHostValidation`    | Validates RHI function parameters on the CPU                                            |
| `m_enableDeviceValidation`  | Enables GPU-based validation (GBV) in D3D12, GPU Assisted Validation (GPU-AV) in Vulkan |
| `m_enableDeviceBreadcrumbs` | Enables DRED on D3D12 and tries to enable similar vendor-specific extensions on Vulkan  |
| `m_enableRenderDoc`         | Hooks into RenderDoc for in-application capture control                                 |
| `m_enablePix`               | Does the same as RenderDoc, but for PIX                                                 |

`BeginFrameCapture` and `EndFrameCapture` trigger GPU capture frames. When both RenderDoc and PIX hooks are disabled, these functions do nothing. When either is enabled, they trigger the corresponding capture.

`SetDebugName` overloads exist for every resource type to change resource debug name after creation.

`ReportDeviceMemoryLeaks()` must be called after `Context` is destroyed. It reports leaked GPU memory and backend API objects — not general CPU memory leaks.

### Debug names are mandatory

Every resource (buffers, textures, pipelines, command buffers, etc.) requires a debug name.

The RHI will assert if one is not provided. This ensures GPU capture tools (RenderDoc, PIX) always show meaningful names.

## Multi-GPU and Linked Adapters

Multiple GPU configurations are supported through `DeviceMode`:

- `Single` — single GPU (default)
- `Linked` — linked adapters (SLI/CrossFire, where GPUs appear as one device with multiple nodes)
- `Unlinked` — separate physical adapters, each with its own context

In linked mode, `Context::m_numLinkedNodes` reports the number of GPU nodes.

Resource creation parameters include `m_nodeIndex` and `m_sharedNodeIndices` to control placement and sharing.

Multi-GPU is not used by the engine at the moment — consider it untested, here be dragons.

## Quirks and gotchas

- Shader bytecode is stored compressed, not as raw DXIL. The RHI decompresses it at load time — the doc says "consumed directly" in the sense that no compilation step runs.
- `ComputeTextureMipLevels` returns the number of mips down to a 4×4 minimum, not 1×1. The lowest mip is always at least 4×4.
- The engine currently does not use per-queue texture states.
- Texture states can optionally be **per-queue** — `GraphicsQueueShaderResource`, `ComputeQueueShaderResource`, and so on — but this is not a hard requirement.
- Some pipeline stages and access flags are restricted to specific queue types — using the wrong stage or access flag on a given queue is an error.
- If a texture has mipmaps, the size of the lowest possible mip level is 4x4

## Backend Implementation

Implementation lives in backend-specific files not exposed in the public header.

Each backend (D3D12, Vulkan) implements the same set of public functions declared in `RHI.h`. A backend is selected at compile time and linked into the final binary.

The public header intentionally exposes no backend-specific types. All differences between D3D12 and Vulkan are hidden behind the opaque resource structs and the function implementations.

## Ray Tracing

The RHI includes a ray tracing interface, it is a placeholder — the implementation is not functional at this time and should not be relied on.
