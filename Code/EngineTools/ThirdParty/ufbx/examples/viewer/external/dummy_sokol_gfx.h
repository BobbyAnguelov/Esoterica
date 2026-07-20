#if defined(SOKOL_IMPL) && !defined(SOKOL_GFX_IMPL)
#define SOKOL_GFX_IMPL
#endif
#ifndef SOKOL_GFX_INCLUDED
/*
    sokol_gfx.h -- simple 3D API wrapper

    Project URL: https://github.com/floooh/sokol

    Example code: https://github.com/floooh/sokol-samples

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_GFX_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    In the same place define one of the following to select the rendering
    backend:
        #define SOKOL_GLCORE33
        #define SOKOL_GLES2
        #define SOKOL_GLES3
        #define SOKOL_D3D11
        #define SOKOL_METAL
        #define SOKOL_WGPU
        #define SOKOL_DUMMY_BACKEND

    I.e. for the GL 3.3 Core Profile it should look like this:

    #include ...
    #include ...
    #define SOKOL_IMPL
    #define SOKOL_GLCORE33
    #include "sokol_gfx.h"

    The dummy backend replaces the platform-specific backend code with empty
    stub functions. This is useful for writing tests that need to run on the
    command line.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)             - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)               - your own free function (default: free(p))
    SOKOL_LOG(msg)              - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_GFX_API_DECL          - public function declaration prefix (default: extern)
    SOKOL_API_DECL              - same as SOKOL_GFX_API_DECL
    SOKOL_API_IMPL              - public function implementation prefix (default: -)
    SOKOL_TRACE_HOOKS           - enable trace hook callbacks (search below for TRACE HOOKS)
    SOKOL_EXTERNAL_GL_LOADER    - indicates that you're using your own GL loader, in this case
                                  sokol_gfx.h will not include any platform GL headers and disable
                                  the integrated Win32 GL loader

    If sokol_gfx.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_GFX_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    If you want to compile without deprecated structs and functions,
    define:

    SOKOL_NO_DEPRECATED

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    sokol_gfx DOES NOT:
    ===================
    - create a window or the 3D-API context/device, you must do this
      before sokol_gfx is initialized, and pass any required information
      (like 3D device pointers) to the sokol_gfx initialization call

    - present the rendered frame, how this is done exactly usually depends
      on how the window and 3D-API context/device was created

    - provide a unified shader language, instead 3D-API-specific shader
      source-code or shader-bytecode must be provided (for the "official"
      offline shader cross-compiler, see here:
      https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md)

    STEP BY STEP
    ============
    --- to initialize sokol_gfx, after creating a window and a 3D-API
        context/device, call:

            sg_setup(const sg_desc*)

    --- create resource objects (at least buffers, shaders and pipelines,
        and optionally images and passes):

            sg_buffer sg_make_buffer(const sg_buffer_desc*)
            sg_image sg_make_image(const sg_image_desc*)
            sg_shader sg_make_shader(const sg_shader_desc*)
            sg_pipeline sg_make_pipeline(const sg_pipeline_desc*)
            sg_pass sg_make_pass(const sg_pass_desc*)

    --- start rendering to the default frame buffer with:

            sg_begin_default_pass(const sg_pass_action* action, int width, int height)

        ...or alternatively with:

            sg_begin_default_passf(const sg_pass_action* action, float width, float height)

        ...which takes the framebuffer width and height as float values.

    --- or start rendering to an offscreen framebuffer with:

            sg_begin_pass(sg_pass pass, const sg_pass_action* action)

    --- set the pipeline state for the next draw call with:

            sg_apply_pipeline(sg_pipeline pip)

    --- fill an sg_bindings struct with the resource bindings for the next
        draw call (1..N vertex buffers, 0 or 1 index buffer, 0..N image objects
        to use as textures each on the vertex-shader- and fragment-shader-stage
        and then call

            sg_apply_bindings(const sg_bindings* bindings)

        to update the resource bindings

    --- optionally update shader uniform data with:

            sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data)
            
        Read the section 'UNIFORM DATA LAYOUT' to learn about the expected memory layout
        of the uniform data passed into sg_apply_uniforms().

    --- kick off a draw call with:

            sg_draw(int base_element, int num_elements, int num_instances)

        The sg_draw() function unifies all the different ways to render primitives
        in a single call (indexed vs non-indexed rendering, and instanced vs non-instanced
        rendering). In case of indexed rendering, base_element and num_element specify
        indices in the currently bound index buffer. In case of non-indexed rendering
        base_element and num_elements specify vertices in the currently bound
        vertex-buffer(s). To perform instanced rendering, the rendering pipeline
        must be setup for instancing (see sg_pipeline_desc below), a separate vertex buffer
        containing per-instance data must be bound, and the num_instances parameter
        must be > 1.

    --- finish the current rendering pass with:

            sg_end_pass()

    --- when done with the current frame, call

            sg_commit()

    --- at the end of your program, shutdown sokol_gfx with:

            sg_shutdown()

    --- if you need to destroy resources before sg_shutdown(), call:

            sg_destroy_buffer(sg_buffer buf)
            sg_destroy_image(sg_image img)
            sg_destroy_shader(sg_shader shd)
            sg_destroy_pipeline(sg_pipeline pip)
            sg_destroy_pass(sg_pass pass)

    --- to set a new viewport rectangle, call

            sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left)

        ...or if you want to specifiy the viewport rectangle with float values:

            sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left)

    --- to set a new scissor rect, call:

            sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left)

        ...or with float values:

            sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left)

        Both sg_apply_viewport() and sg_apply_scissor_rect() must be called
        inside a rendering pass

        Note that sg_begin_default_pass() and sg_begin_pass() will reset both the
        viewport and scissor rectangles to cover the entire framebuffer.

    --- to update (overwrite) the content of buffer and image resources, call:

            sg_update_buffer(sg_buffer buf, const sg_range* data)
            sg_update_image(sg_image img, const sg_image_data* data)

        Buffers and images to be updated must have been created with
        SG_USAGE_DYNAMIC or SG_USAGE_STREAM

        Only one update per frame is allowed for buffer and image resources when
        using the sg_update_*() functions. The rationale is to have a simple
        countermeasure to avoid the CPU scribbling over data the GPU is currently
        using, or the CPU having to wait for the GPU

        Buffer and image updates can be partial, as long as a rendering
        operation only references the valid (updated) data in the
        buffer or image.

    --- to append a chunk of data to a buffer resource, call:

            int sg_append_buffer(sg_buffer buf, const sg_range* data)

        The difference to sg_update_buffer() is that sg_append_buffer()
        can be called multiple times per frame to append new data to the
        buffer piece by piece, optionally interleaved with draw calls referencing
        the previously written data.

        sg_append_buffer() returns a byte offset to the start of the
        written data, this offset can be assigned to
        sg_bindings.vertex_buffer_offsets[n] or
        sg_bindings.index_buffer_offset

        Code example:

        for (...) {
            const void* data = ...;
            const int num_bytes = ...;
            int offset = sg_append_buffer(buf, &(sg_range) { .ptr=data, .size=num_bytes });
            bindings.vertex_buffer_offsets[0] = offset;
            sg_apply_pipeline(pip);
            sg_apply_bindings(&bindings);
            sg_apply_uniforms(...);
            sg_draw(...);
        }

        A buffer to be used with sg_append_buffer() must have been created
        with SG_USAGE_DYNAMIC or SG_USAGE_STREAM.

        If the application appends more data to the buffer then fits into
        the buffer, the buffer will go into the "overflow" state for the
        rest of the frame.

        Any draw calls attempting to render an overflown buffer will be
        silently dropped (in debug mode this will also result in a
        validation error).

        You can also check manually if a buffer is in overflow-state by calling

            bool sg_query_buffer_overflow(sg_buffer buf)

        NOTE: Due to restrictions in underlying 3D-APIs, appended chunks of
        data will be 4-byte aligned in the destination buffer. This means
        that there will be gaps in index buffers containing 16-bit indices
        when the number of indices in a call to sg_append_buffer() is
        odd. This isn't a problem when each call to sg_append_buffer()
        is associated with one draw call, but will be problematic when
        a single indexed draw call spans several appended chunks of indices.

    --- to check at runtime for optional features, limits and pixelformat support,
        call:

            sg_features sg_query_features()
            sg_limits sg_query_limits()
            sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt)

    --- if you need to call into the underlying 3D-API directly, you must call:

            sg_reset_state_cache()

        ...before calling sokol_gfx functions again

    --- you can inspect the original sg_desc structure handed to sg_setup()
        by calling sg_query_desc(). This will return an sg_desc struct with
        the default values patched in instead of any zero-initialized values

    --- you can inspect various internal resource attributes via:

            sg_buffer_info sg_query_buffer_info(sg_buffer buf)
            sg_image_info sg_query_image_info(sg_image img)
            sg_shader_info sg_query_shader_info(sg_shader shd)
            sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip)
            sg_pass_info sg_query_pass_info(sg_pass pass)

        ...please note that the returned info-structs are tied quite closely
        to sokol_gfx.h internals, and may change more often than other
        public API functions and structs.

    --- you can ask at runtime what backend sokol_gfx.h has been compiled
        for, or whether the GLES3 backend had to fall back to GLES2 with:

            sg_backend sg_query_backend(void)

    --- you can query the default resource creation parameters through the functions

            sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc)
            sg_image_desc sg_query_image_defaults(const sg_image_desc* desc)
            sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc)
            sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc)
            sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc)

        These functions take a pointer to a desc structure which may contain
        zero-initialized items for default values. These zero-init values
        will be replaced with their concrete values in the returned desc
        struct.

    ON INITIALIZATION:
    ==================
    When calling sg_setup(), a pointer to an sg_desc struct must be provided
    which contains initialization options. These options provide two types
    of information to sokol-gfx:

        (1) upper bounds and limits needed to allocate various internal
            data structures:
                - the max number of resources of each type that can
                  be alive at the same time, this is used for allocating
                  internal pools
                - the max overall size of uniform data that can be
                  updated per frame, including a worst-case alignment
                  per uniform update (this worst-case alignment is 256 bytes)
                - the max size of all dynamic resource updates (sg_update_buffer,
                  sg_append_buffer and sg_update_image) per frame
                - the max number of entries in the texture sampler cache
                  (how many unique texture sampler can exist at the same time)
            Not all of those limit values are used by all backends, but it is
            good practice to provide them none-the-less.

        (2) 3D-API "context information" (sometimes also called "bindings"):
            sokol_gfx.h doesn't create or initialize 3D API objects which are
            closely related to the presentation layer (this includes the "rendering
            device", the swapchain, and any objects which depend on the
            swapchain). These API objects (or callback functions to obtain
            them, if those objects might change between frames), must
            be provided in a nested sg_context_desc struct inside the
            sg_desc struct. If sokol_gfx.h is used together with
            sokol_app.h, have a look at the sokol_glue.h header which provides
            a convenience function to get a sg_context_desc struct filled out
            with context information provided by sokol_app.h

    See the documention block of the sg_desc struct below for more information.
    

    UNIFORM DATA LAYOUT:
    ====================
    NOTE: if you use the sokol-shdc shader compiler tool, you don't need to worry
    about the following details.

    The data that's passed into the sg_apply_uniforms() function must adhere to
    specific layout rules so that the GPU shader finds the uniform block
    items at the right offset.

    For the D3D11 and Metal backends, sokol-gfx only cares about the size of uniform
    blocks, but not about the internal layout. The data will just be copied into
    a uniform/constant buffer in a single operation and it's up you to arrange the
    CPU-side layout so that it matches the GPU side layout. This also means that with
    the D3D11 and Metal backends you are not limited to a 'cross-platform' subset
    of uniform variable types.

    If you ever only use one of the D3D11, Metal *or* WebGPU backend, you can stop reading here.

    For the GL backends, the internal layout of uniform blocks matters though,
    and you are limited to a small number of uniform variable types. This is
    because sokol-gfx must be able to locate the uniform block members in order
    to upload them to the GPU with glUniformXXX() calls.

    To describe the uniform block layout to sokol-gfx, the following information
    must be passed to the sg_make_shader() call in the sg_shader_desc struct:

        - a hint about the used packing rule (either SG_UNIFORMLAYOUT_NATIVE or
          SG_UNIFORMLAYOUT_STD140)
        - a list of the uniform block members types in the correct order they
          appear on the CPU side
        
    For example if the GLSL shader has the following uniform declarations:

        uniform mat4 mvp;
        uniform vec2 offset0;
        uniform vec2 offset1;
        uniform vec2 offset2;

    ...and on the CPU side, there's a similar C struct:

        typedef struct {
            float mvp[16];
            float offset0[2];
            float offset1[2];
            float offset2[2];
        } params_t;

    ...the uniform block description in the sg_shader_desc must look like this:

        sg_shader_desc desc = {
            .vs.uniform_blocks[0] = {
                .size = sizeof(params_t),
                .layout = SG_UNIFORMLAYOUT_NATIVE,  // this is the default and can be omitted
                .uniforms = {
                    // order must be the same as in 'params_t':
                    [0] = { .name = "mvp", .type = SG_UNIFORMTYPE_MAT4 },
                    [1] = { .name = "offset0", .type = SG_UNIFORMTYPE_VEC2 },
                    [2] = { .name = "offset1", .type = SG_UNIFORMTYPE_VEC2 },
                    [3] = { .name = "offset2", .type = SG_UNIFORMTYPE_VEC2 },
                }
            }
        };

    With this information sokol-gfx can now compute the correct offsets of the data items
    within the uniform block struct.

    The SG_UNIFORMLAYOUT_NATIVE packing rule works fine if only the GL backends are used,
    but for proper D3D11/Metal/GL a subset of the std140 layout must be used which is
    described in the next section:


    CROSS-BACKEND COMMON UNIFORM DATA LAYOUT
    ========================================
    For cross-platform / cross-3D-backend code it is important that the same uniform block
    layout on the CPU side can be used for all sokol-gfx backends. To achieve this, 
    a common subset of the std140 layout must be used:

    - The uniform block layout hint in sg_shader_desc must be explicitely set to
      SG_UNIFORMLAYOUT_STD140.
    - Only the following GLSL uniform types can be used (with their associated sokol-gfx enums):
        - float => SG_UNIFORMTYPE_FLOAT
        - vec2  => SG_UNIFORMTYPE_FLOAT2
        - vec3  => SG_UNIFORMTYPE_FLOAT3
        - vec4  => SG_UNIFORMTYPE_FLOAT4
        - int   => SG_UNIFORMTYPE_INT
        - ivec2 => SG_UNIFORMTYPE_INT2
        - ivec3 => SG_UNIFORMTYPE_INT3
        - ivec4 => SG_UNIFORMTYPE_INT4
        - mat4  => SG_UNIFORMTYPE_MAT4
    - Alignment for those types must be as follows (in bytes):
        - float => 4
        - vec2  => 8
        - vec3  => 16
        - vec4  => 16
        - int   => 4
        - ivec2 => 8
        - ivec3 => 16
        - ivec4 => 16
        - mat4  => 16
    - Arrays are only allowed for the following types: vec4, int4, mat4.
          
    Note that the HLSL cbuffer layout rules are slightly different from the
    std140 layout rules, this means that the cbuffer declarations in HLSL code
    must be tweaked so that the layout is compatible with std140.

    The by far easiest way to tacke the common uniform block layout problem is
    to use the sokol-shdc shader cross-compiler tool!


    BACKEND-SPECIFIC TOPICS:
    ========================
    --- The GL backends need to know about the internal structure of uniform
        blocks, and the texture sampler-name and -type. The uniform layout details
        are  described in the UNIFORM DATA LAYOUT section above.
        
            // uniform block structure and texture image definition in sg_shader_desc:
            sg_shader_desc desc = {
                // uniform block description (size and internal structure)
                .vs.uniform_blocks[0] = {
                    ...
                },
                // one texture on the fragment-shader-stage, GLES2/WebGL needs name and image type
                .fs.images[0] = { .name="tex", .type=SG_IMAGETYPE_ARRAY }
                ...
            };

    --- the Metal and D3D11 backends only need to know the size of uniform blocks,
        not their internal member structure, and they only need to know
        the type of a texture sampler, not its name:

            sg_shader_desc desc = {
                .vs.uniform_blocks[0].size = sizeof(params_t),
                .fs.images[0].type = SG_IMAGETYPE_ARRAY,
                ...
            };

    --- when creating a shader object, GLES2/WebGL need to know the vertex
        attribute names as used in the vertex shader:

            sg_shader_desc desc = {
                .attrs = {
                    [0] = { .name="position" },
                    [1] = { .name="color1" }
                }
            };

        The vertex attribute names provided when creating a shader will be
        used later in sg_create_pipeline() for matching the vertex layout
        to vertex shader inputs.

    --- on D3D11 you need to provide a semantic name and semantic index in the
        shader description struct instead (see the D3D11 documentation on
        D3D11_INPUT_ELEMENT_DESC for details):

            sg_shader_desc desc = {
                .attrs = {
                    [0] = { .sem_name="POSITION", .sem_index=0 }
                    [1] = { .sem_name="COLOR", .sem_index=1 }
                }
            };

        The provided semantic information will be used later in sg_create_pipeline()
        to match the vertex layout to vertex shader inputs.

    --- on D3D11, and when passing HLSL source code (instead of byte code) to shader
        creation, you can optionally define the shader model targets on the vertex
        stage:

            sg_shader_Desc desc = {
                .vs = {
                    ...
                    .d3d11_target = "vs_5_0"
                },
                .fs = {
                    ...
                    .d3d11_target = "ps_5_0"
                }
            };

        The default targets are "ps_4_0" and "fs_4_0". Note that those target names
        are only used when compiling shaders from source. They are ignored when
        creating a shader from bytecode.

    --- on Metal, GL 3.3 or GLES3/WebGL2, you don't need to provide an attribute
        name or semantic name, since vertex attributes can be bound by their slot index
        (this is mandatory in Metal, and optional in GL):

            sg_pipeline_desc desc = {
                .layout = {
                    .attrs = {
                        [0] = { .format=SG_VERTEXFORMAT_FLOAT3 },
                        [1] = { .format=SG_VERTEXFORMAT_FLOAT4 }
                    }
                }
            };

    WORKING WITH CONTEXTS
    =====================
    sokol-gfx allows to switch between different rendering contexts and
    associate resource objects with contexts. This is useful to
    create GL applications that render into multiple windows.

    A rendering context keeps track of all resources created while
    the context is active. When the context is destroyed, all resources
    "belonging to the context" are destroyed as well.

    A default context will be created and activated implicitly in
    sg_setup(), and destroyed in sg_shutdown(). So for a typical application
    which *doesn't* use multiple contexts, nothing changes, and calling
    the context functions isn't necessary.

    Three functions have been added to work with contexts:

    --- sg_context sg_setup_context():
        This must be called once after a GL context has been created and
        made active.

    --- void sg_activate_context(sg_context ctx)
        This must be called after making a different GL context active.
        Apart from 3D-API-specific actions, the call to sg_activate_context()
        will internally call sg_reset_state_cache().

    --- void sg_discard_context(sg_context ctx)
        This must be called right before a GL context is destroyed and
        will destroy all resources associated with the context (that
        have been created while the context was active) The GL context must be
        active at the time sg_discard_context(sg_context ctx) is called.

    Also note that resources (buffers, images, shaders and pipelines) must
    only be used or destroyed while the same GL context is active that
    was also active while the resource was created (an exception is
    resource sharing on GL, such resources can be used while
    another context is active, but must still be destroyed under
    the same context that was active during creation).

    For more information, check out the multiwindow-glfw sample:

    https://github.com/floooh/sokol-samples/blob/master/glfw/multiwindow-glfw.c

    TRACE HOOKS:
    ============
    sokol_gfx.h optionally allows to install "trace hook" callbacks for
    each public API functions. When a public API function is called, and
    a trace hook callback has been installed for this function, the
    callback will be invoked with the parameters and result of the function.
    This is useful for things like debugging- and profiling-tools, or
    keeping track of resource creation and destruction.

    To use the trace hook feature:

    --- Define SOKOL_TRACE_HOOKS before including the implementation.

    --- Setup an sg_trace_hooks structure with your callback function
        pointers (keep all function pointers you're not interested
        in zero-initialized), optionally set the user_data member
        in the sg_trace_hooks struct.

    --- Install the trace hooks by calling sg_install_trace_hooks(),
        the return value of this function is another sg_trace_hooks
        struct which contains the previously set of trace hooks.
        You should keep this struct around, and call those previous
        functions pointers from your own trace callbacks for proper
        chaining.

    As an example of how trace hooks are used, have a look at the
    imgui/sokol_gfx_imgui.h header which implements a realtime
    debugging UI for sokol_gfx.h on top of Dear ImGui.

    A NOTE ON PORTABLE PACKED VERTEX FORMATS:
    =========================================
    There are two things to consider when using packed
    vertex formats like UBYTE4, SHORT2, etc which need to work
    across all backends:

    - D3D11 can only convert *normalized* vertex formats to
      floating point during vertex fetch, normalized formats
      have a trailing 'N', and are "normalized" to a range
      -1.0..+1.0 (for the signed formats) or 0.0..1.0 (for the
      unsigned formats):

        - SG_VERTEXFORMAT_BYTE4N
        - SG_VERTEXFORMAT_UBYTE4N
        - SG_VERTEXFORMAT_SHORT2N
        - SG_VERTEXFORMAT_USHORT2N
        - SG_VERTEXFORMAT_SHORT4N
        - SG_VERTEXFORMAT_USHORT4N

      D3D11 will not convert *non-normalized* vertex formats to floating point
      vertex shader inputs, those can only be uses with the *ivecn* vertex shader
      input types when D3D11 is used as backend (GL and Metal can use both formats)

        - SG_VERTEXFORMAT_BYTE4,
        - SG_VERTEXFORMAT_UBYTE4
        - SG_VERTEXFORMAT_SHORT2
        - SG_VERTEXFORMAT_SHORT4

    - WebGL/GLES2 cannot use integer vertex shader inputs (int or ivecn)

    - SG_VERTEXFORMAT_UINT10_N2 is not supported on WebGL/GLES2

    So for a vertex input layout which works on all platforms, only use the following
    vertex formats, and if needed "expand" the normalized vertex shader
    inputs in the vertex shader by multiplying with 127.0, 255.0, 32767.0 or
    65535.0:

        - SG_VERTEXFORMAT_FLOAT,
        - SG_VERTEXFORMAT_FLOAT2,
        - SG_VERTEXFORMAT_FLOAT3,
        - SG_VERTEXFORMAT_FLOAT4,
        - SG_VERTEXFORMAT_BYTE4N,
        - SG_VERTEXFORMAT_UBYTE4N,
        - SG_VERTEXFORMAT_SHORT2N,
        - SG_VERTEXFORMAT_USHORT2N
        - SG_VERTEXFORMAT_SHORT4N,
        - SG_VERTEXFORMAT_USHORT4N

    TODO:
    ====
    - talk about asynchronous resource creation

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_GFX_INCLUDED (1)
#include <stddef.h>     // size_t
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_GFX_API_DECL)
#define SOKOL_GFX_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_GFX_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_GFX_IMPL)
#define SOKOL_GFX_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_GFX_API_DECL __declspec(dllimport)
#else
#define SOKOL_GFX_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    Resource id typedefs:

    sg_buffer:      vertex- and index-buffers
    sg_image:       textures and render targets
    sg_shader:      vertex- and fragment-shaders, uniform blocks
    sg_pipeline:    associated shader and vertex-layouts, and render states
    sg_pass:        a bundle of render targets and actions on them
    sg_context:     a 'context handle' for switching between 3D-API contexts

    Instead of pointers, resource creation functions return a 32-bit
    number which uniquely identifies the resource object.

    The 32-bit resource id is split into a 16-bit pool index in the lower bits,
    and a 16-bit 'unique counter' in the upper bits. The index allows fast
    pool lookups, and combined with the unique-mask it allows to detect
    'dangling accesses' (trying to use an object which no longer exists, and
    its pool slot has been reused for a new object)

    The resource ids are wrapped into a struct so that the compiler
    can complain when the wrong resource type is used.
*/
typedef struct sg_buffer   { uint32_t id; } sg_buffer;
typedef struct sg_image    { uint32_t id; } sg_image;
typedef struct sg_shader   { uint32_t id; } sg_shader;
typedef struct sg_pipeline { uint32_t id; } sg_pipeline;
typedef struct sg_pass     { uint32_t id; } sg_pass;
typedef struct sg_context  { uint32_t id; } sg_context;

/*
    sg_range is a pointer-size-pair struct used to pass memory blobs into
    sokol-gfx. When initialized from a value type (array or struct), you can
    use the SG_RANGE() macro to build an sg_range struct. For functions which
    take either a sg_range pointer, or a (C++) sg_range reference, use the
    SG_RANGE_REF macro as a solution which compiles both in C and C++.
*/
typedef struct sg_range {
    const void* ptr;
    size_t size;
} sg_range;

// disabling this for every includer isn't great, but the warnings are also quite pointless
#if defined(_MSC_VER)
#pragma warning(disable:4221)   /* /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y' */
#pragma warning(disable:4204)   /* VS2015: nonstandard extension used: non-constant aggregate initializer */
#endif
#if defined(__cplusplus)
#define SG_RANGE(x) sg_range{ &x, sizeof(x) }
#define SG_RANGE_REF(x) sg_range{ &x, sizeof(x) }
#else
#define SG_RANGE(x) (sg_range){ &x, sizeof(x) }
#define SG_RANGE_REF(x) &(sg_range){ &x, sizeof(x) }
#endif

//  various compile-time constants
enum {
    SG_INVALID_ID = 0,
    SG_NUM_SHADER_STAGES = 2,
    SG_NUM_INFLIGHT_FRAMES = 2,
    SG_MAX_COLOR_ATTACHMENTS = 4,
    SG_MAX_SHADERSTAGE_BUFFERS = 8,
    SG_MAX_SHADERSTAGE_IMAGES = 12,
    SG_MAX_SHADERSTAGE_UBS = 4,
    SG_MAX_UB_MEMBERS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,      /* NOTE: actual max vertex attrs can be less on GLES2, see sg_limits! */
    SG_MAX_MIPMAPS = 16,
    SG_MAX_TEXTUREARRAY_LAYERS = 128
};

/*
    sg_color

    An RGBA color value.
*/
typedef struct sg_color { float r, g, b, a; } sg_color;

/*
    sg_backend

    The active 3D-API backend, use the function sg_query_backend()
    to get the currently active backend.

    NOTE that SG_BACKEND_GLES2 will be returned if sokol-gfx was
    compiled with SOKOL_GLES3, but the runtime platform doesn't support
    GLES3/WebGL2 and sokol-gfx had to fallback to GLES2/WebGL.
*/
typedef enum sg_backend {
    SG_BACKEND_GLCORE33,
    SG_BACKEND_GLES2,
    SG_BACKEND_GLES3,
    SG_BACKEND_D3D11,
    SG_BACKEND_METAL_IOS,
    SG_BACKEND_METAL_MACOS,
    SG_BACKEND_METAL_SIMULATOR,
    SG_BACKEND_WGPU,
    SG_BACKEND_DUMMY,
} sg_backend;

/*
    sg_pixel_format

    sokol_gfx.h basically uses the same pixel formats as WebGPU, since these
    are supported on most newer GPUs. GLES2 and WebGL only supports a much
    smaller subset of actually available pixel formats. Call
    sg_query_pixelformat() to check at runtime if a pixel format supports the
    desired features.

    A pixelformat name consist of three parts:

        - components (R, RG, RGB or RGBA)
        - bit width per component (8, 16 or 32)
        - component data type:
            - unsigned normalized (no postfix)
            - signed normalized (SN postfix)
            - unsigned integer (UI postfix)
            - signed integer (SI postfix)
            - float (F postfix)

    Not all pixel formats can be used for everything, call sg_query_pixelformat()
    to inspect the capabilities of a given pixelformat. The function returns
    an sg_pixelformat_info struct with the following bool members:

        - sample: the pixelformat can be sampled as texture at least with
                  nearest filtering
        - filter: the pixelformat can be samples as texture with linear
                  filtering
        - render: the pixelformat can be used for render targets
        - blend:  blending is supported when using the pixelformat for
                  render targets
        - msaa:   multisample-antialiasing is supported when using the
                  pixelformat for render targets
        - depth:  the pixelformat can be used for depth-stencil attachments

    When targeting GLES2/WebGL, the only safe formats to use
    as texture are SG_PIXELFORMAT_R8 and SG_PIXELFORMAT_RGBA8. For rendering
    in GLES2/WebGL, only SG_PIXELFORMAT_RGBA8 is safe. All other formats
    must be checked via sg_query_pixelformats().

    The default pixel format for texture images is SG_PIXELFORMAT_RGBA8.

    The default pixel format for render target images is platform-dependent:
        - for Metal and D3D11 it is SG_PIXELFORMAT_BGRA8
        - for GL backends it is SG_PIXELFORMAT_RGBA8

    This is mainly because of the default framebuffer which is setup outside
    of sokol_gfx.h. On some backends, using BGRA for the default frame buffer
    allows more efficient frame flips. For your own offscreen-render-targets,
    use whatever renderable pixel format is convenient for you.
*/
typedef enum sg_pixel_format {
    _SG_PIXELFORMAT_DEFAULT,    /* value 0 reserved for default-init */
    SG_PIXELFORMAT_NONE,

    SG_PIXELFORMAT_R8,
    SG_PIXELFORMAT_R8SN,
    SG_PIXELFORMAT_R8UI,
    SG_PIXELFORMAT_R8SI,

    SG_PIXELFORMAT_R16,
    SG_PIXELFORMAT_R16SN,
    SG_PIXELFORMAT_R16UI,
    SG_PIXELFORMAT_R16SI,
    SG_PIXELFORMAT_R16F,
    SG_PIXELFORMAT_RG8,
    SG_PIXELFORMAT_RG8SN,
    SG_PIXELFORMAT_RG8UI,
    SG_PIXELFORMAT_RG8SI,

    SG_PIXELFORMAT_R32UI,
    SG_PIXELFORMAT_R32SI,
    SG_PIXELFORMAT_R32F,
    SG_PIXELFORMAT_RG16,
    SG_PIXELFORMAT_RG16SN,
    SG_PIXELFORMAT_RG16UI,
    SG_PIXELFORMAT_RG16SI,
    SG_PIXELFORMAT_RG16F,
    SG_PIXELFORMAT_RGBA8,
    SG_PIXELFORMAT_RGBA8SN,
    SG_PIXELFORMAT_RGBA8UI,
    SG_PIXELFORMAT_RGBA8SI,
    SG_PIXELFORMAT_BGRA8,
    SG_PIXELFORMAT_RGB10A2,
    SG_PIXELFORMAT_RG11B10F,

    SG_PIXELFORMAT_RG32UI,
    SG_PIXELFORMAT_RG32SI,
    SG_PIXELFORMAT_RG32F,
    SG_PIXELFORMAT_RGBA16,
    SG_PIXELFORMAT_RGBA16SN,
    SG_PIXELFORMAT_RGBA16UI,
    SG_PIXELFORMAT_RGBA16SI,
    SG_PIXELFORMAT_RGBA16F,

    SG_PIXELFORMAT_RGBA32UI,
    SG_PIXELFORMAT_RGBA32SI,
    SG_PIXELFORMAT_RGBA32F,

    SG_PIXELFORMAT_DEPTH,
    SG_PIXELFORMAT_DEPTH_STENCIL,

    SG_PIXELFORMAT_BC1_RGBA,
    SG_PIXELFORMAT_BC2_RGBA,
    SG_PIXELFORMAT_BC3_RGBA,
    SG_PIXELFORMAT_BC4_R,
    SG_PIXELFORMAT_BC4_RSN,
    SG_PIXELFORMAT_BC5_RG,
    SG_PIXELFORMAT_BC5_RGSN,
    SG_PIXELFORMAT_BC6H_RGBF,
    SG_PIXELFORMAT_BC6H_RGBUF,
    SG_PIXELFORMAT_BC7_RGBA,
    SG_PIXELFORMAT_PVRTC_RGB_2BPP,
    SG_PIXELFORMAT_PVRTC_RGB_4BPP,
    SG_PIXELFORMAT_PVRTC_RGBA_2BPP,
    SG_PIXELFORMAT_PVRTC_RGBA_4BPP,
    SG_PIXELFORMAT_ETC2_RGB8,
    SG_PIXELFORMAT_ETC2_RGB8A1,
    SG_PIXELFORMAT_ETC2_RGBA8,
    SG_PIXELFORMAT_ETC2_RG11,
    SG_PIXELFORMAT_ETC2_RG11SN,

    _SG_PIXELFORMAT_NUM,
    _SG_PIXELFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_pixel_format;

/*
    Runtime information about a pixel format, returned
    by sg_query_pixelformat().
*/
typedef struct sg_pixelformat_info {
    bool sample;        // pixel format can be sampled in shaders
    bool filter;        // pixel format can be sampled with filtering
    bool render;        // pixel format can be used as render target
    bool blend;         // alpha-blending is supported
    bool msaa;          // pixel format can be used as MSAA render target
    bool depth;         // pixel format is a depth format
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[3];
    #endif
} sg_pixelformat_info;

/*
    Runtime information about available optional features,
    returned by sg_query_features()
*/
typedef struct sg_features {
    bool instancing;                    // hardware instancing supported
    bool origin_top_left;               // framebuffer and texture origin is in top left corner
    bool multiple_render_targets;       // offscreen render passes can have multiple render targets attached
    bool msaa_render_targets;           // offscreen render passes support MSAA antialiasing
    bool imagetype_3d;                  // creation of SG_IMAGETYPE_3D images is supported
    bool imagetype_array;               // creation of SG_IMAGETYPE_ARRAY images is supported
    bool image_clamp_to_border;         // border color and clamp-to-border UV-wrap mode is supported
    bool mrt_independent_blend_state;   // multiple-render-target rendering can use per-render-target blend state
    bool mrt_independent_write_mask;    // multiple-render-target rendering can use per-render-target color write masks
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[3];
    #endif
} sg_features;

/*
    Runtime information about resource limits, returned by sg_query_limit()
*/
typedef struct sg_limits {
    int max_image_size_2d;          // max width/height of SG_IMAGETYPE_2D images
    int max_image_size_cube;        // max width/height of SG_IMAGETYPE_CUBE images
    int max_image_size_3d;          // max width/height/depth of SG_IMAGETYPE_3D images
    int max_image_size_array;       // max width/height of SG_IMAGETYPE_ARRAY images
    int max_image_array_layers;     // max number of layers in SG_IMAGETYPE_ARRAY images
    int max_vertex_attrs;           // <= SG_MAX_VERTEX_ATTRIBUTES or less (on some GLES2 impls)
    int gl_max_vertex_uniform_vectors;  // <= GL_MAX_VERTEX_UNIFORM_VECTORS (only on GL backends)
} sg_limits;

/*
    sg_resource_state

    The current state of a resource in its resource pool.
    Resources start in the INITIAL state, which means the
    pool slot is unoccupied and can be allocated. When a resource is
    created, first an id is allocated, and the resource pool slot
    is set to state ALLOC. After allocation, the resource is
    initialized, which may result in the VALID or FAILED state. The
    reason why allocation and initialization are separate is because
    some resource types (e.g. buffers and images) might be asynchronously
    initialized by the user application. If a resource which is not
    in the VALID state is attempted to be used for rendering, rendering
    operations will silently be dropped.

    The special INVALID state is returned in sg_query_xxx_state() if no
    resource object exists for the provided resource id.
*/
typedef enum sg_resource_state {
    SG_RESOURCESTATE_INITIAL,
    SG_RESOURCESTATE_ALLOC,
    SG_RESOURCESTATE_VALID,
    SG_RESOURCESTATE_FAILED,
    SG_RESOURCESTATE_INVALID,
    _SG_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} sg_resource_state;

/*
    sg_usage

    A resource usage hint describing the update strategy of
    buffers and images. This is used in the sg_buffer_desc.usage
    and sg_image_desc.usage members when creating buffers
    and images:

    SG_USAGE_IMMUTABLE:     the resource will never be updated with
                            new data, instead the content of the
                            resource must be provided on creation
    SG_USAGE_DYNAMIC:       the resource will be updated infrequently
                            with new data (this could range from "once
                            after creation", to "quite often but not
                            every frame")
    SG_USAGE_STREAM:        the resource will be updated each frame
                            with new content

    The rendering backends use this hint to prevent that the
    CPU needs to wait for the GPU when attempting to update
    a resource that might be currently accessed by the GPU.

    Resource content is updated with the functions sg_update_buffer() or
    sg_append_buffer() for buffer objects, and sg_update_image() for image
    objects. For the sg_update_*() functions, only one update is allowed per
    frame and resource object, while sg_append_buffer() can be called
    multiple times per frame on the same buffer. The application must update
    all data required for rendering (this means that the update data can be
    smaller than the resource size, if only a part of the overall resource
    size is used for rendering, you only need to make sure that the data that
    *is* used is valid).

    The default usage is SG_USAGE_IMMUTABLE.
*/
typedef enum sg_usage {
    _SG_USAGE_DEFAULT,      /* value 0 reserved for default-init */
    SG_USAGE_IMMUTABLE,
    SG_USAGE_DYNAMIC,
    SG_USAGE_STREAM,
    _SG_USAGE_NUM,
    _SG_USAGE_FORCE_U32 = 0x7FFFFFFF
} sg_usage;

/*
    sg_buffer_type

    This indicates whether a buffer contains vertex- or index-data,
    used in the sg_buffer_desc.type member when creating a buffer.

    The default value is SG_BUFFERTYPE_VERTEXBUFFER.
*/
typedef enum sg_buffer_type {
    _SG_BUFFERTYPE_DEFAULT,         /* value 0 reserved for default-init */
    SG_BUFFERTYPE_VERTEXBUFFER,
    SG_BUFFERTYPE_INDEXBUFFER,
    _SG_BUFFERTYPE_NUM,
    _SG_BUFFERTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_buffer_type;

/*
    sg_index_type

    Indicates whether indexed rendering (fetching vertex-indices from an
    index buffer) is used, and if yes, the index data type (16- or 32-bits).
    This is used in the sg_pipeline_desc.index_type member when creating a
    pipeline object.

    The default index type is SG_INDEXTYPE_NONE.
*/
typedef enum sg_index_type {
    _SG_INDEXTYPE_DEFAULT,   /* value 0 reserved for default-init */
    SG_INDEXTYPE_NONE,
    SG_INDEXTYPE_UINT16,
    SG_INDEXTYPE_UINT32,
    _SG_INDEXTYPE_NUM,
    _SG_INDEXTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_index_type;

/*
    sg_image_type

    Indicates the basic type of an image object (2D-texture, cubemap,
    3D-texture or 2D-array-texture). 3D- and array-textures are not supported
    on the GLES2/WebGL backend (use sg_query_features().imagetype_3d and
    sg_query_features().imagetype_array to check for support). The image type
    is used in the sg_image_desc.type member when creating an image, and
    in sg_shader_image_desc when describing a shader's texture sampler binding.

    The default image type when creating an image is SG_IMAGETYPE_2D.
*/
typedef enum sg_image_type {
    _SG_IMAGETYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_IMAGETYPE_2D,
    SG_IMAGETYPE_CUBE,
    SG_IMAGETYPE_3D,
    SG_IMAGETYPE_ARRAY,
    _SG_IMAGETYPE_NUM,
    _SG_IMAGETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_type;

/*
    sg_sampler_type

    Indicates the basic data type of a shader's texture sampler which
    can be float , unsigned integer or signed integer. The sampler
    type is used in the sg_shader_image_desc to describe the
    sampler type of a shader's texture sampler binding.

    The default sampler type is SG_SAMPLERTYPE_FLOAT.
*/
typedef enum sg_sampler_type {
    _SG_SAMPLERTYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_SAMPLERTYPE_FLOAT,
    SG_SAMPLERTYPE_SINT,
    SG_SAMPLERTYPE_UINT,
} sg_sampler_type;

/*
    sg_cube_face

    The cubemap faces. Use these as indices in the sg_image_desc.content
    array.
*/
typedef enum sg_cube_face {
    SG_CUBEFACE_POS_X,
    SG_CUBEFACE_NEG_X,
    SG_CUBEFACE_POS_Y,
    SG_CUBEFACE_NEG_Y,
    SG_CUBEFACE_POS_Z,
    SG_CUBEFACE_NEG_Z,
    SG_CUBEFACE_NUM,
    _SG_CUBEFACE_FORCE_U32 = 0x7FFFFFFF
} sg_cube_face;

/*
    sg_shader_stage

    There are 2 shader stages: vertex- and fragment-shader-stage.
    Each shader stage consists of:

    - one slot for a shader function (provided as source- or byte-code)
    - SG_MAX_SHADERSTAGE_UBS slots for uniform blocks
    - SG_MAX_SHADERSTAGE_IMAGES slots for images used as textures by
      the shader function
*/
typedef enum sg_shader_stage {
    SG_SHADERSTAGE_VS,
    SG_SHADERSTAGE_FS,
    _SG_SHADERSTAGE_FORCE_U32 = 0x7FFFFFFF
} sg_shader_stage;

/*
    sg_primitive_type

    This is the common subset of 3D primitive types supported across all 3D
    APIs. This is used in the sg_pipeline_desc.primitive_type member when
    creating a pipeline object.

    The default primitive type is SG_PRIMITIVETYPE_TRIANGLES.
*/
typedef enum sg_primitive_type {
    _SG_PRIMITIVETYPE_DEFAULT,  /* value 0 reserved for default-init */
    SG_PRIMITIVETYPE_POINTS,
    SG_PRIMITIVETYPE_LINES,
    SG_PRIMITIVETYPE_LINE_STRIP,
    SG_PRIMITIVETYPE_TRIANGLES,
    SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    _SG_PRIMITIVETYPE_NUM,
    _SG_PRIMITIVETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_primitive_type;

/*
    sg_filter

    The filtering mode when sampling a texture image. This is
    used in the sg_image_desc.min_filter and sg_image_desc.mag_filter
    members when creating an image object.

    The default filter mode is SG_FILTER_NEAREST.
*/
typedef enum sg_filter {
    _SG_FILTER_DEFAULT, /* value 0 reserved for default-init */
    SG_FILTER_NEAREST,
    SG_FILTER_LINEAR,
    SG_FILTER_NEAREST_MIPMAP_NEAREST,
    SG_FILTER_NEAREST_MIPMAP_LINEAR,
    SG_FILTER_LINEAR_MIPMAP_NEAREST,
    SG_FILTER_LINEAR_MIPMAP_LINEAR,
    _SG_FILTER_NUM,
    _SG_FILTER_FORCE_U32 = 0x7FFFFFFF
} sg_filter;

/*
    sg_wrap

    The texture coordinates wrapping mode when sampling a texture
    image. This is used in the sg_image_desc.wrap_u, .wrap_v
    and .wrap_w members when creating an image.

    The default wrap mode is SG_WRAP_REPEAT.

    NOTE: SG_WRAP_CLAMP_TO_BORDER is not supported on all backends
    and platforms. To check for support, call sg_query_features()
    and check the "clamp_to_border" boolean in the returned
    sg_features struct.

    Platforms which don't support SG_WRAP_CLAMP_TO_BORDER will silently fall back
    to SG_WRAP_CLAMP_TO_EDGE without a validation error.

    Platforms which support clamp-to-border are:

        - all desktop GL platforms
        - Metal on macOS
        - D3D11

    Platforms which do not support clamp-to-border:

        - GLES2/3 and WebGL/WebGL2
        - Metal on iOS
*/
typedef enum sg_wrap {
    _SG_WRAP_DEFAULT,   /* value 0 reserved for default-init */
    SG_WRAP_REPEAT,
    SG_WRAP_CLAMP_TO_EDGE,
    SG_WRAP_CLAMP_TO_BORDER,
    SG_WRAP_MIRRORED_REPEAT,
    _SG_WRAP_NUM,
    _SG_WRAP_FORCE_U32 = 0x7FFFFFFF
} sg_wrap;

/*
    sg_border_color

    The border color to use when sampling a texture, and the UV wrap
    mode is SG_WRAP_CLAMP_TO_BORDER.

    The default border color is SG_BORDERCOLOR_OPAQUE_BLACK
*/
typedef enum sg_border_color {
    _SG_BORDERCOLOR_DEFAULT,    /* value 0 reserved for default-init */
    SG_BORDERCOLOR_TRANSPARENT_BLACK,
    SG_BORDERCOLOR_OPAQUE_BLACK,
    SG_BORDERCOLOR_OPAQUE_WHITE,
    _SG_BORDERCOLOR_NUM,
    _SG_BORDERCOLOR_FORCE_U32 = 0x7FFFFFFF
} sg_border_color;

/*
    sg_vertex_format

    The data type of a vertex component. This is used to describe
    the layout of vertex data when creating a pipeline object.
*/
typedef enum sg_vertex_format {
    SG_VERTEXFORMAT_INVALID,
    SG_VERTEXFORMAT_FLOAT,
    SG_VERTEXFORMAT_FLOAT2,
    SG_VERTEXFORMAT_FLOAT3,
    SG_VERTEXFORMAT_FLOAT4,
    SG_VERTEXFORMAT_BYTE4,
    SG_VERTEXFORMAT_BYTE4N,
    SG_VERTEXFORMAT_UBYTE4,
    SG_VERTEXFORMAT_UBYTE4N,
    SG_VERTEXFORMAT_SHORT2,
    SG_VERTEXFORMAT_SHORT2N,
    SG_VERTEXFORMAT_USHORT2N,
    SG_VERTEXFORMAT_SHORT4,
    SG_VERTEXFORMAT_SHORT4N,
    SG_VERTEXFORMAT_USHORT4N,
    SG_VERTEXFORMAT_UINT10_N2,
    _SG_VERTEXFORMAT_NUM,
    _SG_VERTEXFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_vertex_format;

/*
    sg_vertex_step

    Defines whether the input pointer of a vertex input stream is advanced
    'per vertex' or 'per instance'. The default step-func is
    SG_VERTEXSTEP_PER_VERTEX. SG_VERTEXSTEP_PER_INSTANCE is used with
    instanced-rendering.

    The vertex-step is part of the vertex-layout definition
    when creating pipeline objects.
*/
typedef enum sg_vertex_step {
    _SG_VERTEXSTEP_DEFAULT,     /* value 0 reserved for default-init */
    SG_VERTEXSTEP_PER_VERTEX,
    SG_VERTEXSTEP_PER_INSTANCE,
    _SG_VERTEXSTEP_NUM,
    _SG_VERTEXSTEP_FORCE_U32 = 0x7FFFFFFF
} sg_vertex_step;

/*
    sg_uniform_type

    The data type of a uniform block member. This is used to
    describe the internal layout of uniform blocks when creating
    a shader object.
*/
typedef enum sg_uniform_type {
    SG_UNIFORMTYPE_INVALID,
    SG_UNIFORMTYPE_FLOAT,
    SG_UNIFORMTYPE_FLOAT2,
    SG_UNIFORMTYPE_FLOAT3,
    SG_UNIFORMTYPE_FLOAT4,
    SG_UNIFORMTYPE_INT,
    SG_UNIFORMTYPE_INT2,
    SG_UNIFORMTYPE_INT3,
    SG_UNIFORMTYPE_INT4,
    SG_UNIFORMTYPE_MAT4,
    _SG_UNIFORMTYPE_NUM,
    _SG_UNIFORMTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_type;

/*
    sg_uniform_layout
    
    A hint for the interior memory layout of uniform blocks. This is
    only really relevant for the GL backend where the internal layout
    of uniform blocks must be known to sokol-gfx. For all other backends the
    internal memory layout of uniform blocks doesn't matter, sokol-gfx
    will just pass uniform data as a single memory blob to the
    3D backend.
    
    SG_UNIFORMLAYOUT_NATIVE (default)
        Native layout means that a 'backend-native' memory layout
        is used. For the GL backend this means that uniforms
        are packed tightly in memory (e.g. there are no padding
        bytes).
        
    SG_UNIFORMLAYOUT_STD140
        The memory layout is a subset of std140. Arrays are only
        allowed for the FLOAT4, INT4 and MAT4. Alignment is as
        is as follows:
        
            FLOAT, INT:         4 byte alignment
            FLOAT2, INT2:       8 byte alignment
            FLOAT3, INT3:       16 byte alignment(!)
            FLOAT4, INT4:       16 byte alignment
            MAT4:               16 byte alignment
            FLOAT4[], INT4[]:   16 byte alignment
            
        The overall size of the uniform block must be a multiple
        of 16.
        
    For more information search for 'UNIFORM DATA LAYOUT' in the documentation block
    at the start of the header.
*/
typedef enum sg_uniform_layout {
    _SG_UNIFORMLAYOUT_DEFAULT,     /* value 0 reserved for default-init */
    SG_UNIFORMLAYOUT_NATIVE,       /* default: layout depends on currently active backend */
    SG_UNIFORMLAYOUT_STD140,       /* std140: memory layout according to std140 */
    _SG_UNIFORMLAYOUT_NUM,
    _SG_UNIFORMLAYOUT_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_layout;
 
/*
    sg_cull_mode

    The face-culling mode, this is used in the
    sg_pipeline_desc.cull_mode member when creating a
    pipeline object.

    The default cull mode is SG_CULLMODE_NONE
*/
typedef enum sg_cull_mode {
    _SG_CULLMODE_DEFAULT,   /* value 0 reserved for default-init */
    SG_CULLMODE_NONE,
    SG_CULLMODE_FRONT,
    SG_CULLMODE_BACK,
    _SG_CULLMODE_NUM,
    _SG_CULLMODE_FORCE_U32 = 0x7FFFFFFF
} sg_cull_mode;

/*
    sg_face_winding

    The vertex-winding rule that determines a front-facing primitive. This
    is used in the member sg_pipeline_desc.face_winding
    when creating a pipeline object.

    The default winding is SG_FACEWINDING_CW (clockwise)
*/
typedef enum sg_face_winding {
    _SG_FACEWINDING_DEFAULT,    /* value 0 reserved for default-init */
    SG_FACEWINDING_CCW,
    SG_FACEWINDING_CW,
    _SG_FACEWINDING_NUM,
    _SG_FACEWINDING_FORCE_U32 = 0x7FFFFFFF
} sg_face_winding;

/*
    sg_compare_func

    The compare-function for depth- and stencil-ref tests.
    This is used when creating pipeline objects in the members:

    sg_pipeline_desc
        .depth
            .compare
        .stencil
            .front.compare
            .back.compar

    The default compare func for depth- and stencil-tests is
    SG_COMPAREFUNC_ALWAYS.
*/
typedef enum sg_compare_func {
    _SG_COMPAREFUNC_DEFAULT,    /* value 0 reserved for default-init */
    SG_COMPAREFUNC_NEVER,
    SG_COMPAREFUNC_LESS,
    SG_COMPAREFUNC_EQUAL,
    SG_COMPAREFUNC_LESS_EQUAL,
    SG_COMPAREFUNC_GREATER,
    SG_COMPAREFUNC_NOT_EQUAL,
    SG_COMPAREFUNC_GREATER_EQUAL,
    SG_COMPAREFUNC_ALWAYS,
    _SG_COMPAREFUNC_NUM,
    _SG_COMPAREFUNC_FORCE_U32 = 0x7FFFFFFF
} sg_compare_func;

/*
    sg_stencil_op

    The operation performed on a currently stored stencil-value when a
    comparison test passes or fails. This is used when creating a pipeline
    object in the members:

    sg_pipeline_desc
        .stencil
            .front
                .fail_op
                .depth_fail_op
                .pass_op
            .back
                .fail_op
                .depth_fail_op
                .pass_op

    The default value is SG_STENCILOP_KEEP.
*/
typedef enum sg_stencil_op {
    _SG_STENCILOP_DEFAULT,      /* value 0 reserved for default-init */
    SG_STENCILOP_KEEP,
    SG_STENCILOP_ZERO,
    SG_STENCILOP_REPLACE,
    SG_STENCILOP_INCR_CLAMP,
    SG_STENCILOP_DECR_CLAMP,
    SG_STENCILOP_INVERT,
    SG_STENCILOP_INCR_WRAP,
    SG_STENCILOP_DECR_WRAP,
    _SG_STENCILOP_NUM,
    _SG_STENCILOP_FORCE_U32 = 0x7FFFFFFF
} sg_stencil_op;

/*
    sg_blend_factor

    The source and destination factors in blending operations.
    This is used in the following members when creating a pipeline object:

    sg_pipeline_desc
        .colors[i]
            .blend
                .src_factor_rgb
                .dst_factor_rgb
                .src_factor_alpha
                .dst_factor_alpha

    The default value is SG_BLENDFACTOR_ONE for source
    factors, and SG_BLENDFACTOR_ZERO for destination factors.
*/
typedef enum sg_blend_factor {
    _SG_BLENDFACTOR_DEFAULT,    /* value 0 reserved for default-init */
    SG_BLENDFACTOR_ZERO,
    SG_BLENDFACTOR_ONE,
    SG_BLENDFACTOR_SRC_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
    SG_BLENDFACTOR_SRC_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    SG_BLENDFACTOR_DST_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_DST_COLOR,
    SG_BLENDFACTOR_DST_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
    SG_BLENDFACTOR_SRC_ALPHA_SATURATED,
    SG_BLENDFACTOR_BLEND_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR,
    SG_BLENDFACTOR_BLEND_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA,
    _SG_BLENDFACTOR_NUM,
    _SG_BLENDFACTOR_FORCE_U32 = 0x7FFFFFFF
} sg_blend_factor;

/*
    sg_blend_op

    Describes how the source and destination values are combined in the
    fragment blending operation. It is used in the following members when
    creating a pipeline object:

    sg_pipeline_desc
        .colors[i]
            .blend
                .op_rgb
                .op_alpha

    The default value is SG_BLENDOP_ADD.
*/
typedef enum sg_blend_op {
    _SG_BLENDOP_DEFAULT,    /* value 0 reserved for default-init */
    SG_BLENDOP_ADD,
    SG_BLENDOP_SUBTRACT,
    SG_BLENDOP_REVERSE_SUBTRACT,
    _SG_BLENDOP_NUM,
    _SG_BLENDOP_FORCE_U32 = 0x7FFFFFFF
} sg_blend_op;

/*
    sg_color_mask

    Selects the active color channels when writing a fragment color to the
    framebuffer. This is used in the members
    sg_pipeline_desc.colors[i].write_mask when creating a pipeline object.

    The default colormask is SG_COLORMASK_RGBA (write all colors channels)

    NOTE: since the color mask value 0 is reserved for the default value
    (SG_COLORMASK_RGBA), use SG_COLORMASK_NONE if all color channels
    should be disabled.
*/
typedef enum sg_color_mask {
    _SG_COLORMASK_DEFAULT = 0,    /* value 0 reserved for default-init */
    SG_COLORMASK_NONE   = 0x10,   /* special value for 'all channels disabled */
    SG_COLORMASK_R      = 0x1,
    SG_COLORMASK_G      = 0x2,
    SG_COLORMASK_RG     = 0x3,
    SG_COLORMASK_B      = 0x4,
    SG_COLORMASK_RB     = 0x5,
    SG_COLORMASK_GB     = 0x6,
    SG_COLORMASK_RGB    = 0x7,
    SG_COLORMASK_A      = 0x8,
    SG_COLORMASK_RA     = 0x9,
    SG_COLORMASK_GA     = 0xA,
    SG_COLORMASK_RGA    = 0xB,
    SG_COLORMASK_BA     = 0xC,
    SG_COLORMASK_RBA    = 0xD,
    SG_COLORMASK_GBA    = 0xE,
    SG_COLORMASK_RGBA   = 0xF,
    _SG_COLORMASK_FORCE_U32 = 0x7FFFFFFF
} sg_color_mask;

/*
    sg_action

    Defines what action should be performed at the start of a render pass:

    SG_ACTION_CLEAR:    clear the render target image
    SG_ACTION_LOAD:     load the previous content of the render target image
    SG_ACTION_DONTCARE: leave the render target image content undefined

    This is used in the sg_pass_action structure.

    The default action for all pass attachments is SG_ACTION_CLEAR, with the
    clear color rgba = {0.5f, 0.5f, 0.5f, 1.0f], depth=1.0 and stencil=0.

    If you want to override the default behaviour, it is important to not
    only set the clear color, but the 'action' field as well (as long as this
    is in its _SG_ACTION_DEFAULT, the value fields will be ignored).
*/
typedef enum sg_action {
    _SG_ACTION_DEFAULT,
    SG_ACTION_CLEAR,
    SG_ACTION_LOAD,
    SG_ACTION_DONTCARE,
    _SG_ACTION_NUM,
    _SG_ACTION_FORCE_U32 = 0x7FFFFFFF
} sg_action;

/*
    sg_pass_action

    The sg_pass_action struct defines the actions to be performed
    at the start of a rendering pass in the functions sg_begin_pass()
    and sg_begin_default_pass().

    A separate action and clear values can be defined for each
    color attachment, and for the depth-stencil attachment.

    The default clear values are defined by the macros:

    - SG_DEFAULT_CLEAR_RED:     0.5f
    - SG_DEFAULT_CLEAR_GREEN:   0.5f
    - SG_DEFAULT_CLEAR_BLUE:    0.5f
    - SG_DEFAULT_CLEAR_ALPHA:   1.0f
    - SG_DEFAULT_CLEAR_DEPTH:   1.0f
    - SG_DEFAULT_CLEAR_STENCIL: 0
*/
typedef struct sg_color_attachment_action {
    sg_action action;
    sg_color value;
} sg_color_attachment_action;

typedef struct sg_depth_attachment_action {
    sg_action action;
    float value;
} sg_depth_attachment_action;

typedef struct sg_stencil_attachment_action {
    sg_action action;
    uint8_t value;
} sg_stencil_attachment_action;

typedef struct sg_pass_action {
    uint32_t _start_canary;
    sg_color_attachment_action colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_depth_attachment_action depth;
    sg_stencil_attachment_action stencil;
    uint32_t _end_canary;
} sg_pass_action;

/*
    sg_bindings

    The sg_bindings structure defines the resource binding slots
    of the sokol_gfx render pipeline, used as argument to the
    sg_apply_bindings() function.

    A resource binding struct contains:

    - 1..N vertex buffers
    - 0..N vertex buffer offsets
    - 0..1 index buffers
    - 0..1 index buffer offsets
    - 0..N vertex shader stage images
    - 0..N fragment shader stage images

    The max number of vertex buffer and shader stage images
    are defined by the SG_MAX_SHADERSTAGE_BUFFERS and
    SG_MAX_SHADERSTAGE_IMAGES configuration constants.

    The optional buffer offsets can be used to put different unrelated
    chunks of vertex- and/or index-data into the same buffer objects.
*/
typedef struct sg_bindings {
    uint32_t _start_canary;
    sg_buffer vertex_buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    int vertex_buffer_offsets[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_buffer index_buffer;
    int index_buffer_offset;
    sg_image vs_images[SG_MAX_SHADERSTAGE_IMAGES];
    sg_image fs_images[SG_MAX_SHADERSTAGE_IMAGES];
    uint32_t _end_canary;
} sg_bindings;

/*
    sg_buffer_desc

    Creation parameters for sg_buffer objects, used in the
    sg_make_buffer() call.

    The default configuration is:

    .size:      0       (*must* be >0 for buffers without data)
    .type:      SG_BUFFERTYPE_VERTEXBUFFER
    .usage:     SG_USAGE_IMMUTABLE
    .data.ptr   0       (*must* be valid for immutable buffers)
    .data.size  0       (*must* be > 0 for immutable buffers)
    .label      0       (optional string label for trace hooks)

    The label will be ignored by sokol_gfx.h, it is only useful
    when hooking into sg_make_buffer() or sg_init_buffer() via
    the sg_install_trace_hooks() function.

    For immutable buffers which are initialized with initial data,
    keep the .size item zero-initialized, and set the size together with the
    pointer to the initial data in the .data item.

    For mutable buffers without initial data, keep the .data item
    zero-initialized, and set the buffer size in the .size item instead.

    You can also set both size values, but currently both size values must
    be identical (this may change in the future when the dynamic resource
    management may become more flexible).

    ADVANCED TOPIC: Injecting native 3D-API buffers:

    The following struct members allow to inject your own GL, Metal
    or D3D11 buffers into sokol_gfx:

    .gl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .mtl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_buffer

    You must still provide all other struct items except the .data item, and
    these must match the creation parameters of the native buffers you
    provide. For SG_USAGE_IMMUTABLE, only provide a single native 3D-API
    buffer, otherwise you need to provide SG_NUM_INFLIGHT_FRAMES buffers
    (only for GL and Metal, not D3D11). Providing multiple buffers for GL and
    Metal is necessary because sokol_gfx will rotate through them when
    calling sg_update_buffer() to prevent lock-stalls.

    Note that it is expected that immutable injected buffer have already been
    initialized with content, and the .content member must be 0!

    Also you need to call sg_reset_state_cache() after calling native 3D-API
    functions, and before calling any sokol_gfx function.
*/
typedef struct sg_buffer_desc {
    uint32_t _start_canary;
    size_t size;
    sg_buffer_type type;
    sg_usage usage;
    sg_range data;
    const char* label;
    /* GL specific */
    uint32_t gl_buffers[SG_NUM_INFLIGHT_FRAMES];
    /* Metal specific */
    const void* mtl_buffers[SG_NUM_INFLIGHT_FRAMES];
    /* D3D11 specific */
    const void* d3d11_buffer;
    /* WebGPU specific */
    const void* wgpu_buffer;
    uint32_t _end_canary;
} sg_buffer_desc;

/*
    sg_image_data

    Defines the content of an image through a 2D array of sg_range structs.
    The first array dimension is the cubemap face, and the second array
    dimension the mipmap level.
*/
typedef struct sg_image_data {
    sg_range subimage[SG_CUBEFACE_NUM][SG_MAX_MIPMAPS];
} sg_image_data;

/*
    sg_image_desc

    Creation parameters for sg_image objects, used in the sg_make_image()
    call.

    The default configuration is:

    .type:              SG_IMAGETYPE_2D
    .render_target:     false
    .width              0 (must be set to >0)
    .height             0 (must be set to >0)
    .num_slices         1 (3D textures: depth; array textures: number of layers)
    .num_mipmaps:       1
    .usage:             SG_USAGE_IMMUTABLE
    .pixel_format:      SG_PIXELFORMAT_RGBA8 for textures, or sg_desc.context.color_format for render targets
    .sample_count:      1 for textures, or sg_desc.context.sample_count for render targets
    .min_filter:        SG_FILTER_NEAREST
    .mag_filter:        SG_FILTER_NEAREST
    .wrap_u:            SG_WRAP_REPEAT
    .wrap_v:            SG_WRAP_REPEAT
    .wrap_w:            SG_WRAP_REPEAT (only SG_IMAGETYPE_3D)
    .border_color       SG_BORDERCOLOR_OPAQUE_BLACK
    .max_anisotropy     1 (must be 1..16)
    .min_lod            0.0f
    .max_lod            FLT_MAX
    .data               an sg_image_data struct to define the initial content
    .label              0       (optional string label for trace hooks)

    Q: Why is the default sample_count for render targets identical with the
    "default sample count" from sg_desc.context.sample_count?

    A: So that it matches the default sample count in pipeline objects. Even
    though it is a bit strange/confusing that offscreen render targets by default
    get the same sample count as the default framebuffer, but it's better that
    an offscreen render target created with default parameters matches
    a pipeline object created with default parameters.

    NOTE:

    SG_IMAGETYPE_ARRAY and SG_IMAGETYPE_3D are not supported on WebGL/GLES2,
    use sg_query_features().imagetype_array and
    sg_query_features().imagetype_3d at runtime to check if array- and
    3D-textures are supported.

    Images with usage SG_USAGE_IMMUTABLE must be fully initialized by
    providing a valid .data member which points to initialization data.

    ADVANCED TOPIC: Injecting native 3D-API textures:

    The following struct members allow to inject your own GL, Metal or D3D11
    textures into sokol_gfx:

    .gl_textures[SG_NUM_INFLIGHT_FRAMES]
    .mtl_textures[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_texture
    .d3d11_shader_resource_view

    For GL, you can also specify the texture target or leave it empty to use
    the default texture target for the image type (GL_TEXTURE_2D for
    SG_IMAGETYPE_2D etc)

    For D3D11, you can provide either a D3D11 texture, or a
    shader-resource-view, or both. If only a texture is provided, a matching
    shader-resource-view will be created. If only a shader-resource-view is
    provided, the texture will be looked up from the shader-resource-view.

    The same rules apply as for injecting native buffers (see sg_buffer_desc
    documentation for more details).
*/
typedef struct sg_image_desc {
    uint32_t _start_canary;
    sg_image_type type;
    bool render_target;
    int width;
    int height;
    int num_slices;
    int num_mipmaps;
    sg_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    sg_border_color border_color;
    uint32_t max_anisotropy;
    float min_lod;
    float max_lod;
    sg_image_data data;
    const char* label;
    /* GL specific */
    uint32_t gl_textures[SG_NUM_INFLIGHT_FRAMES];
    uint32_t gl_texture_target;
    /* Metal specific */
    const void* mtl_textures[SG_NUM_INFLIGHT_FRAMES];
    /* D3D11 specific */
    const void* d3d11_texture;
    const void* d3d11_shader_resource_view;
    /* WebGPU specific */
    const void* wgpu_texture;
    uint32_t _end_canary;
} sg_image_desc;

/*
    sg_shader_desc

    The structure sg_shader_desc defines all creation parameters for shader
    programs, used as input to the sg_make_shader() function:

    - reflection information for vertex attributes (vertex shader inputs):
        - vertex attribute name (required for GLES2, optional for GLES3 and GL)
        - a semantic name and index (required for D3D11)
    - for each shader-stage (vertex and fragment):
        - the shader source or bytecode
        - an optional entry function name
        - an optional compile target (only for D3D11 when source is provided,
          defaults are "vs_4_0" and "ps_4_0")
        - reflection info for each uniform block used by the shader stage:
            - the size of the uniform block in bytes
            - a memory layout hint (native vs std140, only required for GL backends)
            - reflection info for each uniform block member (only required for GL backends):
                - member name
                - member type (SG_UNIFORMTYPE_xxx)
                - if the member is an array, the number of array items
        - reflection info for the texture images used by the shader stage:
            - the image type (SG_IMAGETYPE_xxx)
            - the sampler type (SG_SAMPLERTYPE_xxx, default is SG_SAMPLERTYPE_FLOAT)
            - the name of the texture sampler (required for GLES2, optional everywhere else)

    For all GL backends, shader source-code must be provided. For D3D11 and Metal,
    either shader source-code or byte-code can be provided.

    For D3D11, if source code is provided, the d3dcompiler_47.dll will be loaded
    on demand. If this fails, shader creation will fail. When compiling HLSL
    source code, you can provide an optional target string via
    sg_shader_stage_desc.d3d11_target, the default target is "vs_4_0" for the
    vertex shader stage and "ps_4_0" for the pixel shader stage.
*/
typedef struct sg_shader_attr_desc {
    const char* name;           // GLSL vertex attribute name (only strictly required for GLES2)
    const char* sem_name;       // HLSL semantic name
    int sem_index;              // HLSL semantic index
} sg_shader_attr_desc;

typedef struct sg_shader_uniform_desc {
    const char* name;
    sg_uniform_type type;
    int array_count;
} sg_shader_uniform_desc;

typedef struct sg_shader_uniform_block_desc {
    size_t size;
    sg_uniform_layout layout;
    sg_shader_uniform_desc uniforms[SG_MAX_UB_MEMBERS];
} sg_shader_uniform_block_desc;

typedef struct sg_shader_image_desc {
    const char* name;
    sg_image_type image_type;
    sg_sampler_type sampler_type;
} sg_shader_image_desc;

typedef struct sg_shader_stage_desc {
    const char* source;
    sg_range bytecode;
    const char* entry;
    const char* d3d11_target;
    sg_shader_uniform_block_desc uniform_blocks[SG_MAX_SHADERSTAGE_UBS];
    sg_shader_image_desc images[SG_MAX_SHADERSTAGE_IMAGES];
} sg_shader_stage_desc;

typedef struct sg_shader_desc {
    uint32_t _start_canary;
    sg_shader_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_stage_desc vs;
    sg_shader_stage_desc fs;
    const char* label;
    uint32_t _end_canary;
} sg_shader_desc;

/*
    sg_pipeline_desc

    The sg_pipeline_desc struct defines all creation parameters for an
    sg_pipeline object, used as argument to the sg_make_pipeline() function:

    - the vertex layout for all input vertex buffers
    - a shader object
    - the 3D primitive type (points, lines, triangles, ...)
    - the index type (none, 16- or 32-bit)
    - all the fixed-function-pipeline state (depth-, stencil-, blend-state, etc...)

    If the vertex data has no gaps between vertex components, you can omit
    the .layout.buffers[].stride and layout.attrs[].offset items (leave them
    default-initialized to 0), sokol-gfx will then compute the offsets and
    strides from the vertex component formats (.layout.attrs[].format).
    Please note that ALL vertex attribute offsets must be 0 in order for the
    automatic offset computation to kick in.

    The default configuration is as follows:

    .shader:            0 (must be initialized with a valid sg_shader id!)
    .layout:
        .buffers[]:         vertex buffer layouts
            .stride:        0 (if no stride is given it will be computed)
            .step_func      SG_VERTEXSTEP_PER_VERTEX
            .step_rate      1
        .attrs[]:           vertex attribute declarations
            .buffer_index   0 the vertex buffer bind slot
            .offset         0 (offsets can be omitted if the vertex layout has no gaps)
            .format         SG_VERTEXFORMAT_INVALID (must be initialized!)
    .depth:
        .pixel_format:      sg_desc.context.depth_format
        .compare:           SG_COMPAREFUNC_ALWAYS
        .write_enabled:     false
        .bias:              0.0f
        .bias_slope_scale:  0.0f
        .bias_clamp:        0.0f
    .stencil:
        .enabled:           false
        .front/back:
            .compare:       SG_COMPAREFUNC_ALWAYS
            .depth_fail_op: SG_STENCILOP_KEEP
            .pass_op:       SG_STENCILOP_KEEP
            .compare:       SG_COMPAREFUNC_ALWAYS
        .read_mask:         0
        .write_mask:        0
        .ref:               0
    .color_count            1
    .colors[0..color_count]
        .pixel_format       sg_desc.context.color_format
        .write_mask:        SG_COLORMASK_RGBA
        .blend:
            .enabled:           false
            .src_factor_rgb:    SG_BLENDFACTOR_ONE
            .dst_factor_rgb:    SG_BLENDFACTOR_ZERO
            .op_rgb:            SG_BLENDOP_ADD
            .src_factor_alpha:  SG_BLENDFACTOR_ONE
            .dst_factor_alpha:  SG_BLENDFACTOR_ZERO
            .op_alpha:          SG_BLENDOP_ADD
    .primitive_type:            SG_PRIMITIVETYPE_TRIANGLES
    .index_type:                SG_INDEXTYPE_NONE
    .cull_mode:                 SG_CULLMODE_NONE
    .face_winding:              SG_FACEWINDING_CW
    .sample_count:              sg_desc.context.sample_count
    .blend_color:               (sg_color) { 0.0f, 0.0f, 0.0f, 0.0f }
    .alpha_to_coverage_enabled: false
    .label  0       (optional string label for trace hooks)
*/
typedef struct sg_buffer_layout_desc {
    int stride;
    sg_vertex_step step_func;
    int step_rate;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[2];
    #endif
} sg_buffer_layout_desc;

typedef struct sg_vertex_attr_desc {
    int buffer_index;
    int offset;
    sg_vertex_format format;
    #if defined(SOKOL_ZIG_BINDINGS)
    uint32_t __pad[2];
    #endif
} sg_vertex_attr_desc;

typedef struct sg_layout_desc {
    sg_buffer_layout_desc buffers[SG_MAX_SHADERSTAGE_BUFFERS];
    sg_vertex_attr_desc attrs[SG_MAX_VERTEX_ATTRIBUTES];
} sg_layout_desc;

typedef struct sg_stencil_face_state {
    sg_compare_func compare;
    sg_stencil_op fail_op;
    sg_stencil_op depth_fail_op;
    sg_stencil_op pass_op;
} sg_stencil_face_state;

typedef struct sg_stencil_state {
    bool enabled;
    sg_stencil_face_state front;
    sg_stencil_face_state back;
    uint8_t read_mask;
    uint8_t write_mask;
    uint8_t ref;
} sg_stencil_state;

typedef struct sg_depth_state {
    sg_pixel_format pixel_format;
    sg_compare_func compare;
    bool write_enabled;
    float bias;
    float bias_slope_scale;
    float bias_clamp;
} sg_depth_state;

typedef struct sg_blend_state {
    bool enabled;
    sg_blend_factor src_factor_rgb;
    sg_blend_factor dst_factor_rgb;
    sg_blend_op op_rgb;
    sg_blend_factor src_factor_alpha;
    sg_blend_factor dst_factor_alpha;
    sg_blend_op op_alpha;
} sg_blend_state;

typedef struct sg_color_state {
    sg_pixel_format pixel_format;
    sg_color_mask write_mask;
    sg_blend_state blend;
} sg_color_state;

typedef struct sg_pipeline_desc {
    uint32_t _start_canary;
    sg_shader shader;
    sg_layout_desc layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;
    sg_color_state colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    int sample_count;
    sg_color blend_color;
    bool alpha_to_coverage_enabled;
    const char* label;
    uint32_t _end_canary;
} sg_pipeline_desc;

/*
    sg_pass_desc

    Creation parameters for an sg_pass object, used as argument
    to the sg_make_pass() function.

    A pass object contains 1..4 color-attachments and none, or one,
    depth-stencil-attachment. Each attachment consists of
    an image, and two additional indices describing
    which subimage the pass will render to: one mipmap index, and
    if the image is a cubemap, array-texture or 3D-texture, the
    face-index, array-layer or depth-slice.

    Pass images must fulfill the following requirements:

    All images must have:
    - been created as render target (sg_image_desc.render_target = true)
    - the same size
    - the same sample count

    In addition, all color-attachment images must have the same pixel format.
*/
typedef struct sg_pass_attachment_desc {
    sg_image image;
    int mip_level;
    int slice;      /* cube texture: face; array texture: layer; 3D texture: slice */
} sg_pass_attachment_desc;

typedef struct sg_pass_desc {
    uint32_t _start_canary;
    sg_pass_attachment_desc color_attachments[SG_MAX_COLOR_ATTACHMENTS];
    sg_pass_attachment_desc depth_stencil_attachment;
    const char* label;
    uint32_t _end_canary;
} sg_pass_desc;

/*
    sg_trace_hooks

    Installable callback functions to keep track of the sokol-gfx calls,
    this is useful for debugging, or keeping track of resource creation
    and destruction.

    Trace hooks are installed with sg_install_trace_hooks(), this returns
    another sg_trace_hooks struct with the previous set of
    trace hook function pointers. These should be invoked by the
    new trace hooks to form a proper call chain.
*/
typedef struct sg_trace_hooks {
    void* user_data;
    void (*reset_state_cache)(void* user_data);
    void (*make_buffer)(const sg_buffer_desc* desc, sg_buffer result, void* user_data);
    void (*make_image)(const sg_image_desc* desc, sg_image result, void* user_data);
    void (*make_shader)(const sg_shader_desc* desc, sg_shader result, void* user_data);
    void (*make_pipeline)(const sg_pipeline_desc* desc, sg_pipeline result, void* user_data);
    void (*make_pass)(const sg_pass_desc* desc, sg_pass result, void* user_data);
    void (*destroy_buffer)(sg_buffer buf, void* user_data);
    void (*destroy_image)(sg_image img, void* user_data);
    void (*destroy_shader)(sg_shader shd, void* user_data);
    void (*destroy_pipeline)(sg_pipeline pip, void* user_data);
    void (*destroy_pass)(sg_pass pass, void* user_data);
    void (*update_buffer)(sg_buffer buf, const sg_range* data, void* user_data);
    void (*update_image)(sg_image img, const sg_image_data* data, void* user_data);
    void (*append_buffer)(sg_buffer buf, const sg_range* data, int result, void* user_data);
    void (*begin_default_pass)(const sg_pass_action* pass_action, int width, int height, void* user_data);
    void (*begin_pass)(sg_pass pass, const sg_pass_action* pass_action, void* user_data);
    void (*apply_viewport)(int x, int y, int width, int height, bool origin_top_left, void* user_data);
    void (*apply_scissor_rect)(int x, int y, int width, int height, bool origin_top_left, void* user_data);
    void (*apply_pipeline)(sg_pipeline pip, void* user_data);
    void (*apply_bindings)(const sg_bindings* bindings, void* user_data);
    void (*apply_uniforms)(sg_shader_stage stage, int ub_index, const sg_range* data, void* user_data);
    void (*draw)(int base_element, int num_elements, int num_instances, void* user_data);
    void (*end_pass)(void* user_data);
    void (*commit)(void* user_data);
    void (*alloc_buffer)(sg_buffer result, void* user_data);
    void (*alloc_image)(sg_image result, void* user_data);
    void (*alloc_shader)(sg_shader result, void* user_data);
    void (*alloc_pipeline)(sg_pipeline result, void* user_data);
    void (*alloc_pass)(sg_pass result, void* user_data);
    void (*dealloc_buffer)(sg_buffer buf_id, void* user_data);
    void (*dealloc_image)(sg_image img_id, void* user_data);
    void (*dealloc_shader)(sg_shader shd_id, void* user_data);
    void (*dealloc_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*dealloc_pass)(sg_pass pass_id, void* user_data);
    void (*init_buffer)(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data);
    void (*init_image)(sg_image img_id, const sg_image_desc* desc, void* user_data);
    void (*init_shader)(sg_shader shd_id, const sg_shader_desc* desc, void* user_data);
    void (*init_pipeline)(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data);
    void (*init_pass)(sg_pass pass_id, const sg_pass_desc* desc, void* user_data);
    void (*uninit_buffer)(sg_buffer buf_id, void* user_data);
    void (*uninit_image)(sg_image img_id, void* user_data);
    void (*uninit_shader)(sg_shader shd_id, void* user_data);
    void (*uninit_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*uninit_pass)(sg_pass pass_id, void* user_data);
    void (*fail_buffer)(sg_buffer buf_id, void* user_data);
    void (*fail_image)(sg_image img_id, void* user_data);
    void (*fail_shader)(sg_shader shd_id, void* user_data);
    void (*fail_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*fail_pass)(sg_pass pass_id, void* user_data);
    void (*push_debug_group)(const char* name, void* user_data);
    void (*pop_debug_group)(void* user_data);
    void (*err_buffer_pool_exhausted)(void* user_data);
    void (*err_image_pool_exhausted)(void* user_data);
    void (*err_shader_pool_exhausted)(void* user_data);
    void (*err_pipeline_pool_exhausted)(void* user_data);
    void (*err_pass_pool_exhausted)(void* user_data);
    void (*err_context_mismatch)(void* user_data);
    void (*err_pass_invalid)(void* user_data);
    void (*err_draw_invalid)(void* user_data);
    void (*err_bindings_invalid)(void* user_data);
} sg_trace_hooks;

/*
    sg_buffer_info
    sg_image_info
    sg_shader_info
    sg_pipeline_info
    sg_pass_info

    These structs contain various internal resource attributes which
    might be useful for debug-inspection. Please don't rely on the
    actual content of those structs too much, as they are quite closely
    tied to sokol_gfx.h internals and may change more frequently than
    the other public API elements.

    The *_info structs are used as the return values of the following functions:

    sg_query_buffer_info()
    sg_query_image_info()
    sg_query_shader_info()
    sg_query_pipeline_info()
    sg_query_pass_info()
*/
typedef struct sg_slot_info {
    sg_resource_state state;    /* the current state of this resource slot */
    uint32_t res_id;        /* type-neutral resource if (e.g. sg_buffer.id) */
    uint32_t ctx_id;        /* the context this resource belongs to */
} sg_slot_info;

typedef struct sg_buffer_info {
    sg_slot_info slot;              /* resource pool slot info */
    uint32_t update_frame_index;    /* frame index of last sg_update_buffer() */
    uint32_t append_frame_index;    /* frame index of last sg_append_buffer() */
    int append_pos;                 /* current position in buffer for sg_append_buffer() */
    bool append_overflow;           /* is buffer in overflow state (due to sg_append_buffer) */
    int num_slots;                  /* number of renaming-slots for dynamically updated buffers */
    int active_slot;                /* currently active write-slot for dynamically updated buffers */
} sg_buffer_info;

typedef struct sg_image_info {
    sg_slot_info slot;              /* resource pool slot info */
    uint32_t upd_frame_index;       /* frame index of last sg_update_image() */
    int num_slots;                  /* number of renaming-slots for dynamically updated images */
    int active_slot;                /* currently active write-slot for dynamically updated images */
    int width;                      /* image width */
    int height;                     /* image height */
} sg_image_info;

typedef struct sg_shader_info {
    sg_slot_info slot;              /* resoure pool slot info */
} sg_shader_info;

typedef struct sg_pipeline_info {
    sg_slot_info slot;              /* resource pool slot info */
} sg_pipeline_info;

typedef struct sg_pass_info {
    sg_slot_info slot;              /* resource pool slot info */
} sg_pass_info;

/*
    sg_desc

    The sg_desc struct contains configuration values for sokol_gfx,
    it is used as parameter to the sg_setup() call.

    NOTE that all callback function pointers come in two versions, one without
    a userdata pointer, and one with a userdata pointer. You would
    either initialize one or the other depending on whether you pass data
    to your callbacks.

    FIXME: explain the various configuration options

    The default configuration is:

    .buffer_pool_size       128
    .image_pool_size        128
    .shader_pool_size       32
    .pipeline_pool_size     64
    .pass_pool_size         16
    .context_pool_size      16
    .sampler_cache_size     64
    .uniform_buffer_size    4 MB (4*1024*1024)
    .staging_buffer_size    8 MB (8*1024*1024)

    .context.color_format: default value depends on selected backend:
        all GL backends:    SG_PIXELFORMAT_RGBA8
        Metal and D3D11:    SG_PIXELFORMAT_BGRA8
        WGPU:               *no default* (must be queried from WGPU swapchain)
    .context.depth_format   SG_PIXELFORMAT_DEPTH_STENCIL
    .context.sample_count   1

    GL specific:
        .context.gl.force_gles2
            if this is true the GL backend will act in "GLES2 fallback mode" even
            when compiled with SOKOL_GLES3, this is useful to fall back
            to traditional WebGL if a browser doesn't support a WebGL2 context

    Metal specific:
        (NOTE: All Objective-C object references are transferred through
        a bridged (const void*) to sokol_gfx, which will use a unretained
        bridged cast (__bridged id<xxx>) to retrieve the Objective-C
        references back. Since the bridge cast is unretained, the caller
        must hold a strong reference to the Objective-C object for the
        duration of the sokol_gfx call!

        .context.metal.device
            a pointer to the MTLDevice object
        .context.metal.renderpass_descriptor_cb
        .context.metal_renderpass_descriptor_userdata_cb
            A C callback function to obtain the MTLRenderPassDescriptor for the
            current frame when rendering to the default framebuffer, will be called
            in sg_begin_default_pass().
        .context.metal.drawable_cb
        .context.metal.drawable_userdata_cb
            a C callback function to obtain a MTLDrawable for the current
            frame when rendering to the default framebuffer, will be called in
            sg_end_pass() of the default pass
        .context.metal.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    D3D11 specific:
        .context.d3d11.device
            a pointer to the ID3D11Device object, this must have been created
            before sg_setup() is called
        .context.d3d11.device_context
            a pointer to the ID3D11DeviceContext object
        .context.d3d11.render_target_view_cb
        .context.d3d11.render_target_view_userdata_cb
            a C callback function to obtain a pointer to the current
            ID3D11RenderTargetView object of the default framebuffer,
            this function will be called in sg_begin_pass() when rendering
            to the default framebuffer
        .context.d3d11.depth_stencil_view_cb
        .context.d3d11.depth_stencil_view_userdata_cb
            a C callback function to obtain a pointer to the current
            ID3D11DepthStencilView object of the default framebuffer,
            this function will be called in sg_begin_pass() when rendering
            to the default framebuffer
        .context.metal.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    WebGPU specific:
        .context.wgpu.device
            a WGPUDevice handle
        .context.wgpu.render_format
            WGPUTextureFormat of the swap chain surface
        .context.wgpu.render_view_cb
        .context.wgpu.render_view_userdata_cb
            callback to get the current WGPUTextureView of the swapchain's
            rendering attachment (may be an MSAA surface)
        .context.wgpu.resolve_view_cb
        .context.wgpu.resolve_view_userdata_cb
            callback to get the current WGPUTextureView of the swapchain's
            MSAA-resolve-target surface, must return 0 if not MSAA rendering
        .context.wgpu.depth_stencil_view_cb
        .context.wgpu.depth_stencil_view_userdata_cb
            callback to get current default-pass depth-stencil-surface WGPUTextureView
            the pixel format of the default WGPUTextureView must be WGPUTextureFormat_Depth24Plus8
        .context.metal.user_data
            optional user data pointer passed to the userdata versions of
            callback functions

    When using sokol_gfx.h and sokol_app.h together, consider using the
    helper function sapp_sgcontext() in the sokol_glue.h header to
    initialize the sg_desc.context nested struct. sapp_sgcontext() returns
    a completely initialized sg_context_desc struct with information
    provided by sokol_app.h.
*/
typedef struct sg_gl_context_desc {
    bool force_gles2;
} sg_gl_context_desc;

typedef struct sg_metal_context_desc {
    const void* device;
    const void* (*renderpass_descriptor_cb)(void);
    const void* (*renderpass_descriptor_userdata_cb)(void*);
    const void* (*drawable_cb)(void);
    const void* (*drawable_userdata_cb)(void*);
    void* user_data;
} sg_metal_context_desc;

typedef struct sg_d3d11_context_desc {
    const void* device;
    const void* device_context;
    const void* (*render_target_view_cb)(void);
    const void* (*render_target_view_userdata_cb)(void*);
    const void* (*depth_stencil_view_cb)(void);
    const void* (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
} sg_d3d11_context_desc;

typedef struct sg_wgpu_context_desc {
    const void* device;                    /* WGPUDevice */
    const void* (*render_view_cb)(void);   /* returns WGPUTextureView */
    const void* (*render_view_userdata_cb)(void*);
    const void* (*resolve_view_cb)(void);  /* returns WGPUTextureView */
    const void* (*resolve_view_userdata_cb)(void*);
    const void* (*depth_stencil_view_cb)(void);    /* returns WGPUTextureView, must be WGPUTextureFormat_Depth24Plus8 */
    const void* (*depth_stencil_view_userdata_cb)(void*);
    void* user_data;
} sg_wgpu_context_desc;

typedef struct sg_context_desc {
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    sg_gl_context_desc gl;
    sg_metal_context_desc metal;
    sg_d3d11_context_desc d3d11;
    sg_wgpu_context_desc wgpu;
} sg_context_desc;

typedef struct sg_desc {
    uint32_t _start_canary;
    int buffer_pool_size;
    int image_pool_size;
    int shader_pool_size;
    int pipeline_pool_size;
    int pass_pool_size;
    int context_pool_size;
    int uniform_buffer_size;
    int staging_buffer_size;
    int sampler_cache_size;
    sg_context_desc context;
    uint32_t _end_canary;
} sg_desc;

/* setup and misc functions */
SOKOL_GFX_API_DECL void sg_setup(const sg_desc* desc);
SOKOL_GFX_API_DECL void sg_shutdown(void);
SOKOL_GFX_API_DECL bool sg_isvalid(void);
SOKOL_GFX_API_DECL void sg_reset_state_cache(void);
SOKOL_GFX_API_DECL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks);
SOKOL_GFX_API_DECL void sg_push_debug_group(const char* name);
SOKOL_GFX_API_DECL void sg_pop_debug_group(void);

/* resource creation, destruction and updating */
SOKOL_GFX_API_DECL sg_buffer sg_make_buffer(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image sg_make_image(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_shader sg_make_shader(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_pass sg_make_pass(const sg_pass_desc* desc);
SOKOL_GFX_API_DECL void sg_destroy_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_destroy_image(sg_image img);
SOKOL_GFX_API_DECL void sg_destroy_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_destroy_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_destroy_pass(sg_pass pass);
SOKOL_GFX_API_DECL void sg_update_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL void sg_update_image(sg_image img, const sg_image_data* data);
SOKOL_GFX_API_DECL int sg_append_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL bool sg_query_buffer_overflow(sg_buffer buf);

/* rendering functions */
SOKOL_GFX_API_DECL void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height);
SOKOL_GFX_API_DECL void sg_begin_default_passf(const sg_pass_action* pass_action, float width, float height);
SOKOL_GFX_API_DECL void sg_begin_pass(sg_pass pass, const sg_pass_action* pass_action);
SOKOL_GFX_API_DECL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_apply_bindings(const sg_bindings* bindings);
SOKOL_GFX_API_DECL void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data);
SOKOL_GFX_API_DECL void sg_draw(int base_element, int num_elements, int num_instances);
SOKOL_GFX_API_DECL void sg_end_pass(void);
SOKOL_GFX_API_DECL void sg_commit(void);

/* getting information */
SOKOL_GFX_API_DECL sg_desc sg_query_desc(void);
SOKOL_GFX_API_DECL sg_backend sg_query_backend(void);
SOKOL_GFX_API_DECL sg_features sg_query_features(void);
SOKOL_GFX_API_DECL sg_limits sg_query_limits(void);
SOKOL_GFX_API_DECL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt);
/* get current state of a resource (INITIAL, ALLOC, VALID, FAILED, INVALID) */
SOKOL_GFX_API_DECL sg_resource_state sg_query_buffer_state(sg_buffer buf);
SOKOL_GFX_API_DECL sg_resource_state sg_query_image_state(sg_image img);
SOKOL_GFX_API_DECL sg_resource_state sg_query_shader_state(sg_shader shd);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pipeline_state(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pass_state(sg_pass pass);
/* get runtime information about a resource */
SOKOL_GFX_API_DECL sg_buffer_info sg_query_buffer_info(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_info sg_query_image_info(sg_image img);
SOKOL_GFX_API_DECL sg_shader_info sg_query_shader_info(sg_shader shd);
SOKOL_GFX_API_DECL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_pass_info sg_query_pass_info(sg_pass pass);
/* get resource creation desc struct with their default values replaced */
SOKOL_GFX_API_DECL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc);

/* separate resource allocation and initialization (for async setup) */
SOKOL_GFX_API_DECL sg_buffer sg_alloc_buffer(void);
SOKOL_GFX_API_DECL sg_image sg_alloc_image(void);
SOKOL_GFX_API_DECL sg_shader sg_alloc_shader(void);
SOKOL_GFX_API_DECL sg_pipeline sg_alloc_pipeline(void);
SOKOL_GFX_API_DECL sg_pass sg_alloc_pass(void);
SOKOL_GFX_API_DECL void sg_dealloc_buffer(sg_buffer buf_id);
SOKOL_GFX_API_DECL void sg_dealloc_image(sg_image img_id);
SOKOL_GFX_API_DECL void sg_dealloc_shader(sg_shader shd_id);
SOKOL_GFX_API_DECL void sg_dealloc_pipeline(sg_pipeline pip_id);
SOKOL_GFX_API_DECL void sg_dealloc_pass(sg_pass pass_id);
SOKOL_GFX_API_DECL void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL void sg_init_image(sg_image img_id, const sg_image_desc* desc);
SOKOL_GFX_API_DECL void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc);
SOKOL_GFX_API_DECL bool sg_uninit_buffer(sg_buffer buf_id);
SOKOL_GFX_API_DECL bool sg_uninit_image(sg_image img_id);
SOKOL_GFX_API_DECL bool sg_uninit_shader(sg_shader shd_id);
SOKOL_GFX_API_DECL bool sg_uninit_pipeline(sg_pipeline pip_id);
SOKOL_GFX_API_DECL bool sg_uninit_pass(sg_pass pass_id);
SOKOL_GFX_API_DECL void sg_fail_buffer(sg_buffer buf_id);
SOKOL_GFX_API_DECL void sg_fail_image(sg_image img_id);
SOKOL_GFX_API_DECL void sg_fail_shader(sg_shader shd_id);
SOKOL_GFX_API_DECL void sg_fail_pipeline(sg_pipeline pip_id);
SOKOL_GFX_API_DECL void sg_fail_pass(sg_pass pass_id);

/* rendering contexts (optional) */
SOKOL_GFX_API_DECL sg_context sg_setup_context(void);
SOKOL_GFX_API_DECL void sg_activate_context(sg_context ctx_id);
SOKOL_GFX_API_DECL void sg_discard_context(sg_context ctx_id);

/* Backend-specific helper functions, these may come in handy for mixing
   sokol-gfx rendering with 'native backend' rendering functions.

   This group of functions will be expanded as needed.
*/

/* D3D11: return ID3D11Device */
SOKOL_GFX_API_DECL const void* sg_d3d11_device(void);

/* Metal: return __bridge-casted MTLDevice */
SOKOL_GFX_API_DECL const void* sg_mtl_device(void);

/* Metal: return __bridge-casted MTLRenderCommandEncoder in current pass (or zero if outside pass) */
SOKOL_GFX_API_DECL const void* sg_mtl_render_command_encoder(void);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for c++ */
inline void sg_setup(const sg_desc& desc) { return sg_setup(&desc); }

inline sg_buffer sg_make_buffer(const sg_buffer_desc& desc) { return sg_make_buffer(&desc); }
inline sg_image sg_make_image(const sg_image_desc& desc) { return sg_make_image(&desc); }
inline sg_shader sg_make_shader(const sg_shader_desc& desc) { return sg_make_shader(&desc); }
inline sg_pipeline sg_make_pipeline(const sg_pipeline_desc& desc) { return sg_make_pipeline(&desc); }
inline sg_pass sg_make_pass(const sg_pass_desc& desc) { return sg_make_pass(&desc); }
inline void sg_update_image(sg_image img, const sg_image_data& data) { return sg_update_image(img, &data); }

inline void sg_begin_default_pass(const sg_pass_action& pass_action, int width, int height) { return sg_begin_default_pass(&pass_action, width, height); }
inline void sg_begin_default_passf(const sg_pass_action& pass_action, float width, float height) { return sg_begin_default_passf(&pass_action, width, height); }
inline void sg_begin_pass(sg_pass pass, const sg_pass_action& pass_action) { return sg_begin_pass(pass, &pass_action); }
inline void sg_apply_bindings(const sg_bindings& bindings) { return sg_apply_bindings(&bindings); }
inline void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range& data) { return sg_apply_uniforms(stage, ub_index, &data); }

inline sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc& desc) { return sg_query_buffer_defaults(&desc); }
inline sg_image_desc sg_query_image_defaults(const sg_image_desc& desc) { return sg_query_image_defaults(&desc); }
inline sg_shader_desc sg_query_shader_defaults(const sg_shader_desc& desc) { return sg_query_shader_defaults(&desc); }
inline sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc& desc) { return sg_query_pipeline_defaults(&desc); }
inline sg_pass_desc sg_query_pass_defaults(const sg_pass_desc& desc) { return sg_query_pass_defaults(&desc); }

inline void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc& desc) { return sg_init_buffer(buf_id, &desc); }
inline void sg_init_image(sg_image img_id, const sg_image_desc& desc) { return sg_init_image(img_id, &desc); }
inline void sg_init_shader(sg_shader shd_id, const sg_shader_desc& desc) { return sg_init_shader(shd_id, &desc); }
inline void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc& desc) { return sg_init_pipeline(pip_id, &desc); }
inline void sg_init_pass(sg_pass pass_id, const sg_pass_desc& desc) { return sg_init_pass(pass_id, &desc); }

inline void sg_update_buffer(sg_buffer buf_id, const sg_range& data) { return sg_update_buffer(buf_id, &data); }
inline int sg_append_buffer(sg_buffer buf_id, const sg_range& data) { return sg_append_buffer(buf_id, &data); }
#endif
#endif // SOKOL_GFX_INCLUDED

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_GFX_IMPL
#define SOKOL_GFX_IMPL_INCLUDED (1)

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif

typedef struct {
    bool valid;
    sg_desc desc;
    uint32_t buffer_id;
    uint32_t image_id;
    uint32_t shader_id;
    uint32_t pipeline_id;
    uint32_t pass_id;
    uint32_t ctx_id;
} _sg_state;

_sg_state _sg;

/* setup and misc functions */
SOKOL_API_IMPL void sg_setup(const sg_desc* desc)
{
    _sg.valid = true;
    _sg.desc = *desc;
}

SOKOL_API_IMPL void sg_shutdown(void)
{
    _sg.valid = false;
}

SOKOL_API_IMPL bool sg_isvalid(void) { return _sg.valid; }
SOKOL_API_IMPL void sg_reset_state_cache(void) { }
SOKOL_API_IMPL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks) { }
SOKOL_API_IMPL void sg_push_debug_group(const char* name) { }
SOKOL_API_IMPL void sg_pop_debug_group(void) { }

/* resource creation, destruction and updating */
SOKOL_API_IMPL sg_buffer sg_make_buffer(const sg_buffer_desc* desc) { sg_buffer res = { ++_sg.buffer_id }; return res; }
SOKOL_API_IMPL sg_image sg_make_image(const sg_image_desc* desc) { sg_image res = { ++_sg.image_id }; return res; }
SOKOL_API_IMPL sg_shader sg_make_shader(const sg_shader_desc* desc) { sg_shader res = { ++_sg.shader_id }; return res; }
SOKOL_API_IMPL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc) { sg_pipeline res = { ++_sg.pipeline_id }; return res; }
SOKOL_API_IMPL sg_pass sg_make_pass(const sg_pass_desc* desc) { sg_pass res = { ++_sg.pass_id }; return res; }
SOKOL_API_IMPL void sg_destroy_buffer(sg_buffer buf) { }
SOKOL_API_IMPL void sg_destroy_image(sg_image img) { }
SOKOL_API_IMPL void sg_destroy_shader(sg_shader shd) { }
SOKOL_API_IMPL void sg_destroy_pipeline(sg_pipeline pip) {}
SOKOL_API_IMPL void sg_destroy_pass(sg_pass pass) {}
SOKOL_API_IMPL void sg_update_buffer(sg_buffer buf, const sg_range* data) { }
SOKOL_API_IMPL void sg_update_image(sg_image img, const sg_image_data* data) { }
SOKOL_API_IMPL int sg_append_buffer(sg_buffer buf, const sg_range* data) { return 0; }
SOKOL_API_IMPL bool sg_query_buffer_overflow(sg_buffer buf) { return false; }

/* rendering functions */
SOKOL_API_IMPL void sg_begin_default_pass(const sg_pass_action* pass_action, int width, int height) { }
SOKOL_API_IMPL void sg_begin_default_passf(const sg_pass_action* pass_action, float width, float height) { }
SOKOL_API_IMPL void sg_begin_pass(sg_pass pass, const sg_pass_action* pass_action) { }
SOKOL_API_IMPL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) { }
SOKOL_API_IMPL void sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left) { }
SOKOL_API_IMPL void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) { }
SOKOL_API_IMPL void sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left) { }
SOKOL_API_IMPL void sg_apply_pipeline(sg_pipeline pip) { }
SOKOL_API_IMPL void sg_apply_bindings(const sg_bindings* bindings) { }
SOKOL_API_IMPL void sg_apply_uniforms(sg_shader_stage stage, int ub_index, const sg_range* data) { }
SOKOL_API_IMPL void sg_draw(int base_element, int num_elements, int num_instances) { }
SOKOL_API_IMPL void sg_end_pass(void) { }
SOKOL_API_IMPL void sg_commit(void) { }

/* getting information */
SOKOL_API_IMPL sg_desc sg_query_desc(void) { return _sg.desc; }
SOKOL_API_IMPL sg_backend sg_query_backend(void) { return SG_BACKEND_DUMMY; }
SOKOL_API_IMPL sg_features sg_query_features(void) {
    sg_features features = { 0 };
    features.instancing = true;
    features.origin_top_left = true;
    features.multiple_render_targets = true;
    features.msaa_render_targets = true;
    features.imagetype_3d = true;
    features.imagetype_array = true;
    features.image_clamp_to_border = true;
    features.mrt_independent_blend_state = true;
    features.mrt_independent_write_mask = true;
    return features;
}
SOKOL_API_IMPL sg_limits sg_query_limits(void) {
    sg_limits limits = { 0 };
    limits.max_image_size_2d = 65536;
    limits.max_image_size_cube = 65536;
    limits.max_image_size_3d = 65536;
    limits.max_image_size_array = 65536;
    limits.max_image_array_layers = 65536;
    limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;
    limits.gl_max_vertex_uniform_vectors = 65536;
    return limits;
}
SOKOL_API_IMPL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt) {
    sg_pixelformat_info info = { 0 };
    info.sample = true;
    info.filter = true;
    info.render = true;
    info.blend = true;
    info.msaa = true;
    if (fmt == SG_PIXELFORMAT_DEPTH || fmt == SG_PIXELFORMAT_DEPTH_STENCIL) {
        info.depth = true;
    }
    return info;
}
/* get current state of a resource (INITIAL, ALLOC, VALID, FAILED, INVALID) */
SOKOL_API_IMPL sg_resource_state sg_query_buffer_state(sg_buffer buf) { return SG_RESOURCESTATE_VALID; }
SOKOL_API_IMPL sg_resource_state sg_query_image_state(sg_image img) { return SG_RESOURCESTATE_VALID; }
SOKOL_API_IMPL sg_resource_state sg_query_shader_state(sg_shader shd) { return SG_RESOURCESTATE_VALID; }
SOKOL_API_IMPL sg_resource_state sg_query_pipeline_state(sg_pipeline pip) { return SG_RESOURCESTATE_VALID; }
SOKOL_API_IMPL sg_resource_state sg_query_pass_state(sg_pass pass) { return SG_RESOURCESTATE_VALID; }
/* get runtime information about a resource */
SOKOL_API_IMPL sg_buffer_info sg_query_buffer_info(sg_buffer buf) { sg_buffer_info info = { 0 }; return info; }
SOKOL_API_IMPL sg_image_info sg_query_image_info(sg_image img) { sg_image_info info = { 0 }; return info; }
SOKOL_API_IMPL sg_shader_info sg_query_shader_info(sg_shader shd) { sg_shader_info info = { 0 }; return info; }
SOKOL_API_IMPL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip) { sg_pipeline_info info = { 0 }; return info; }
SOKOL_API_IMPL sg_pass_info sg_query_pass_info(sg_pass pass) { sg_pass_info info = { 0 }; return info; }
/* get resource creation desc struct with their default values replaced */
SOKOL_API_IMPL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc) { return *desc; }
SOKOL_API_IMPL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc) { return *desc; }
SOKOL_API_IMPL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc) { return *desc; }
SOKOL_API_IMPL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc) { return *desc; }
SOKOL_API_IMPL sg_pass_desc sg_query_pass_defaults(const sg_pass_desc* desc) { return *desc; }

/* separate resource allocation and initialization (for async setup) */
SOKOL_API_IMPL sg_buffer sg_alloc_buffer(void) { return sg_make_buffer(NULL); }
SOKOL_API_IMPL sg_image sg_alloc_image(void) { return sg_make_image(NULL); }
SOKOL_API_IMPL sg_shader sg_alloc_shader(void) { return sg_make_shader(NULL); }
SOKOL_API_IMPL sg_pipeline sg_alloc_pipeline(void) { return sg_make_pipeline(NULL); }
SOKOL_API_IMPL sg_pass sg_alloc_pass(void) { return sg_make_pass(NULL); }
SOKOL_API_IMPL void sg_dealloc_buffer(sg_buffer buf_id) { }
SOKOL_API_IMPL void sg_dealloc_image(sg_image img_id) { }
SOKOL_API_IMPL void sg_dealloc_shader(sg_shader shd_id) { }
SOKOL_API_IMPL void sg_dealloc_pipeline(sg_pipeline pip_id) { }
SOKOL_API_IMPL void sg_dealloc_pass(sg_pass pass_id) { }
SOKOL_API_IMPL void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc) { }
SOKOL_API_IMPL void sg_init_image(sg_image img_id, const sg_image_desc* desc) { }
SOKOL_API_IMPL void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) { }
SOKOL_API_IMPL void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc) { }
SOKOL_API_IMPL void sg_init_pass(sg_pass pass_id, const sg_pass_desc* desc) { }
SOKOL_API_IMPL bool sg_uninit_buffer(sg_buffer buf_id) { }
SOKOL_API_IMPL bool sg_uninit_image(sg_image img_id) { }
SOKOL_API_IMPL bool sg_uninit_shader(sg_shader shd_id) { }
SOKOL_API_IMPL bool sg_uninit_pipeline(sg_pipeline pip_id) { }
SOKOL_API_IMPL bool sg_uninit_pass(sg_pass pass_id) { }
SOKOL_API_IMPL void sg_fail_buffer(sg_buffer buf_id) { }
SOKOL_API_IMPL void sg_fail_image(sg_image img_id) { }
SOKOL_API_IMPL void sg_fail_shader(sg_shader shd_id) { }
SOKOL_API_IMPL void sg_fail_pipeline(sg_pipeline pip_id) { }
SOKOL_API_IMPL void sg_fail_pass(sg_pass pass_id) { }

/* rendering contexts (optional) */
SOKOL_API_IMPL sg_context sg_setup_context(void) { sg_context ctx = { ++_sg.ctx_id }; return ctx; }
SOKOL_API_IMPL void sg_activate_context(sg_context ctx_id) { }
SOKOL_API_IMPL void sg_discard_context(sg_context ctx_id) { }

/* Backend-specific helper functions, these may come in handy for mixing
   sokol-gfx rendering with 'native backend' rendering functions.

   This group of functions will be expanded as needed.
*/

/* D3D11: return ID3D11Device */
SOKOL_API_IMPL const void* sg_d3d11_device(void) { return NULL; }

/* Metal: return __bridge-casted MTLDevice */
SOKOL_API_IMPL const void* sg_mtl_device(void) { return NULL; }

/* Metal: return __bridge-casted MTLRenderCommandEncoder in current pass (or zero if outside pass) */
SOKOL_API_IMPL const void* sg_mtl_render_command_encoder(void) { return NULL; }

#endif /* SOKOL_GFX_IMPL */
