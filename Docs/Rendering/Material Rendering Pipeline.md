# Material Rendering Pipeline

The material rendering pipeline is a fully GPU-driven path for opaque and transparent triangle meshes. Frustum culling, occlusion culling, cluster culling, and indirect draw argument generation all run in compute shaders — there is no per-instance work on the CPU.

The pipeline produces ready-to-use indirect draw argument buffers for each shader and render view.

## Render Views

We create a render view for the main camera, each shadow cascade, and each environment map face. Directional lights create 4 render views each for Cascaded Shadow Mapping.

Each render view maps to one bit of a 64-bit view mask. The culling passes produce one indirect draw argument buffer per shader, per render view and sub-bucket, through a shared draw compaction pass that scatters visible cluster slots into dense per-( view, bucket ) draw buffers.

## Culling Passes

Culling runs in six GPU compute passes:

**Instance culling** runs 128-thread groups ( 1 thread per instance ), tests each instance against every render view, and writes a 64-bit visibility mask for every instance.

**Culling compaction** runs 128-thread groups ( 1 thread per instance ) and, for each visible instance, atomically reserves one work entry per cluster in the culling work buffer, then writes the entries sequentially.

**Culling argument generation** reads the work counter and writes one cluster culling argument plus one draw compaction argument per 65k-group chunk of the work buffer.

**Cluster culling** executes indirectly over the chunk arguments, one thread per work entry. It performs per-cluster frustum and screen-size tests for each view, writes the resulting 64-bit visibility mask back into the work entry, and atomically counts the cluster per view and sub-bucket.

**Draw argument generation** prefix-sums the per-( view, bucket ) counts into per-bucket base offsets and writes one draw argument per non-empty bucket, split into 65k-group draws.

**Draw compaction** executes indirectly over the chunk arguments and scatters the slots of visible clusters into dense per-( view, bucket ) draw cluster buffers, which the mesh shaders index directly.

## Draw Arguments

Three sub-buckets exist per shader per view: opaque, alpha-tested and alpha-blended. The sub-bucket is selected at runtime using the shader flags specified in the material.

Each bucket has its own draw counter and draw argument buffer, those are filled by the culling pipeline and grouped into 65k-group dispatches.

Each render pass executes its draw argument buffers with the per-bucket counters. Shadow passes do a single depth pass; the forward shading pass runs a depth prepass followed by opaque and alpha blending passes.

## Light and Decal Culling

Lights and decals use a spatial hash for world-space culling.

The world is partitioned into a 6-level LOD hierarchy — the hash works with arbitrary spatial positions and any cell coordinate you feed it; it just happens to be camera-relative by default. The coarsest LOD spans the entire scene, finer LODs refine near the origin.

Unlike a dense grid, the hash stores only occupied cells — empty space costs no memory, and the structure naturally conforms to complex world topology without extra cost.

Unlike screen-space tiling, the spatial hash supports arbitrary world-space lookups — reflection passes and ray tracing shaders can query off-screen lights and decals.

A compute shader tests each light against its cell, constrained by parent page ranges, and writes per-cell bitmasks into an open-addressing hash table. The culling pass runs on the async compute queue, overlapped with the depth prepass and the previous frame's post processing.

The pixel shader looks up its cell via `LoadPayloadCell` and iterates the compacted light list with a scalarized bitscan loop.

## Extending the Pipeline

At the moment the renderer is “vertically integrated” — extending it means writing code directly. We are working on a more customizable and data-driven solution, current renderer code is under heavy development.

There is no plugin system or high-level scripting API.

The pipeline has two natural extension points:

1. **Custom mesh rendering passes** inside the material pipeline. Derive from`RenderPass_ForwardShading` or `RenderPass_CascadedShadow` in the engine source. Prefer this when triangle mesh rendering fits into the existing pipeline.
2. **Custom rendering passes** outside the material pipeline. Derive from `RenderPass_PostProcess` in the engine source. Prefer this for image filtering or anything that does not fit the triangle mesh category.

`Renderer_ForwardShading.h/.cpp` implements the clustered forward rendering pipeline. It can be extended or used as a reference for a custom renderer.

Shader authoring is covered in [Shaders](Shaders.md) . Mesh data and compression are in [Meshes](Meshes.md) [.](Meshes.md)