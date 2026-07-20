#if defined(SOKOL_IMPL) && !defined(SOKOL_APP_IMPL)
#define SOKOL_APP_IMPL
#endif
#ifndef SOKOL_APP_INCLUDED
/*
    sokol_app.h -- cross-platform application wrapper

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_APP_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    In the same place define one of the following to select the 3D-API
    which should be initialized by sokol_app.h (this must also match
    the backend selected for sokol_gfx.h if both are used in the same
    project):

        #define SOKOL_GLCORE33
        #define SOKOL_GLES2
        #define SOKOL_GLES3
        #define SOKOL_D3D11
        #define SOKOL_METAL
        #define SOKOL_WGPU

    Optionally provide the following defines with your own implementations:

        SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
        SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
        SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))
        SOKOL_ABORT()       - called after an unrecoverable error (default: abort())
        SOKOL_WIN32_FORCE_MAIN  - define this on Win32 to use a main() entry point instead of WinMain
        SOKOL_NO_ENTRY      - define this if sokol_app.h shouldn't "hijack" the main() function
        SOKOL_APP_API_DECL  - public function declaration prefix (default: extern)
        SOKOL_API_DECL      - same as SOKOL_APP_API_DECL
        SOKOL_API_IMPL      - public function implementation prefix (default: -)
        SOKOL_CALLOC        - your own calloc function (default: calloc(n, s))
        SOKOL_FREE          - your own free function (default: free(p))

    Optionally define the following to force debug checks and validations
    even in release mode:

        SOKOL_DEBUG         - by default this is defined if _DEBUG is defined

    If sokol_app.h is compiled as a DLL, define the following before
    including the declaration or implementation:

        SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_APP_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    For example code, see https://github.com/floooh/sokol-samples/tree/master/sapp

    Portions of the Windows and Linux GL initialization, event-, icon- etc... code
    have been taken from GLFW (http://www.glfw.org/)

    iOS onscreen keyboard support 'inspired' by libgdx.

    Link with the following system libraries:

    - on macOS with Metal: Cocoa, QuartzCore, Metal, MetalKit
    - on macOS with GL: Cocoa, QuartzCore, OpenGL
    - on iOS with Metal: Foundation, UIKit, Metal, MetalKit
    - on iOS with GL: Foundation, UIKit, OpenGLES, GLKit
    - on Linux: X11, Xi, Xcursor, GL, dl, pthread, m(?)
    - on Android: GLESv3, EGL, log, android
    - on Windows with the MSVC or Clang toolchains: no action needed, libs are defined in-source via pragma-comment-lib
    - on Windows with MINGW/MSYS2 gcc: compile with '-mwin32' so that _WIN32 is defined
        - link with the following libs: -lkernel32 -luser32 -lshell32
        - additionally with the GL backend: -lgdi32
        - additionally with the D3D11 backend: -ld3d11 -ldxgi

    On Linux, you also need to use the -pthread compiler and linker option, otherwise weird
    things will happen, see here for details: https://github.com/floooh/sokol/issues/376

    Building for UWP requires a recent Visual Studio toolchain and Windows SDK
    (at least VS2019 and Windows SDK 10.0.19041.0). When the UWP backend is
    selected, the sokol_app.h implementation must be compiled as C++17.

    On macOS and iOS, the implementation must be compiled as Objective-C.

    FEATURE OVERVIEW
    ================
    sokol_app.h provides a minimalistic cross-platform API which
    implements the 'application-wrapper' parts of a 3D application:

    - a common application entry function
    - creates a window and 3D-API context/device with a 'default framebuffer'
    - makes the rendered frame visible
    - provides keyboard-, mouse- and low-level touch-events
    - platforms: MacOS, iOS, HTML5, Win32, Linux, Android (TODO: RaspberryPi)
    - 3D-APIs: Metal, D3D11, GL3.2, GLES2, GLES3, WebGL, WebGL2

    FEATURE/PLATFORM MATRIX
    =======================
                        | Windows | macOS | Linux |  iOS  | Android | UWP  | Raspi | HTML5
    --------------------+---------+-------+-------+-------+---------+------+-------+-------
    gl 3.x              | YES     | YES   | YES   | ---   | ---     | ---  | ---   | ---
    gles2/webgl         | ---     | ---   | ---   | YES   | YES     | ---  | TODO  | YES
    gles3/webgl2        | ---     | ---   | ---   | YES   | YES     | ---  | ---   | YES
    metal               | ---     | YES   | ---   | YES   | ---     | ---  | ---   | ---
    d3d11               | YES     | ---   | ---   | ---   | ---     | YES  | ---   | ---
    KEY_DOWN            | YES     | YES   | YES   | SOME  | TODO    | YES  | TODO  | YES
    KEY_UP              | YES     | YES   | YES   | SOME  | TODO    | YES  | TODO  | YES
    CHAR                | YES     | YES   | YES   | YES   | TODO    | YES  | TODO  | YES
    MOUSE_DOWN          | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    MOUSE_UP            | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    MOUSE_SCROLL        | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    MOUSE_MOVE          | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    MOUSE_ENTER         | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    MOUSE_LEAVE         | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    TOUCHES_BEGAN       | ---     | ---   | ---   | YES   | YES     | TODO | ---   | YES
    TOUCHES_MOVED       | ---     | ---   | ---   | YES   | YES     | TODO | ---   | YES
    TOUCHES_ENDED       | ---     | ---   | ---   | YES   | YES     | TODO | ---   | YES
    TOUCHES_CANCELLED   | ---     | ---   | ---   | YES   | YES     | TODO | ---   | YES
    RESIZED             | YES     | YES   | YES   | YES   | YES     | YES  | ---   | YES
    ICONIFIED           | YES     | YES   | YES   | ---   | ---     | YES  | ---   | ---
    RESTORED            | YES     | YES   | YES   | ---   | ---     | YES  | ---   | ---
    FOCUSED             | YES     | YES   | YES   | ---   | ---     | ---  | ---   | YES
    UNFOCUSED           | YES     | YES   | YES   | ---   | ---     | ---  | ---   | YES
    SUSPENDED           | ---     | ---   | ---   | YES   | YES     | YES  | ---   | TODO
    RESUMED             | ---     | ---   | ---   | YES   | YES     | YES  | ---   | TODO
    QUIT_REQUESTED      | YES     | YES   | YES   | ---   | ---     | ---  | TODO  | YES
    UPDATE_CURSOR       | YES     | YES   | TODO  | ---   | ---     | TODO | ---   | TODO
    IME                 | TODO    | TODO? | TODO  | ???   | TODO    | ---  | ???   | ???
    key repeat flag     | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    windowed            | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | YES
    fullscreen          | YES     | YES   | YES   | YES   | YES     | YES  | TODO  | ---
    mouse hide          | YES     | YES   | YES   | ---   | ---     | YES  | TODO  | TODO
    mouse lock          | YES     | YES   | YES   | ---   | ---     | TODO | TODO  | YES
    screen keyboard     | ---     | ---   | ---   | YES   | TODO    | TODO | ---   | YES
    swap interval       | YES     | YES   | YES   | YES   | TODO    | ---  | TODO  | YES
    high-dpi            | YES     | YES   | TODO  | YES   | YES     | YES  | TODO  | YES
    clipboard           | YES     | YES   | TODO  | ---   | ---     | TODO | ---   | YES
    MSAA                | YES     | YES   | YES   | YES   | YES     | TODO | TODO  | YES
    drag'n'drop         | YES     | YES   | YES   | ---   | ---     | TODO | TODO  | YES
    window icon         | YES     | YES(1)| YES   | ---   | ---     | TODO | TODO  | YES

    (1) macOS has no regular window icons, instead the dock icon is changed

    STEP BY STEP
    ============
    --- Add a sokol_main() function to your code which returns a sapp_desc structure
        with initialization parameters and callback function pointers. This
        function is called very early, usually at the start of the
        platform's entry function (e.g. main or WinMain). You should do as
        little as possible here, since the rest of your code might be called
        from another thread (this depends on the platform):

            sapp_desc sokol_main(int argc, char* argv[]) {
                return (sapp_desc) {
                    .width = 640,
                    .height = 480,
                    .init_cb = my_init_func,
                    .frame_cb = my_frame_func,
                    .cleanup_cb = my_cleanup_func,
                    .event_cb = my_event_func,
                    ...
                };
            }

        There are many more setup parameters, but these are the most important.
        For a complete list search for the sapp_desc structure declaration
        below.

        DO NOT call any sokol-app function from inside sokol_main(), since
        sokol-app will not be initialized at this point.

        The .width and .height parameters are the preferred size of the 3D
        rendering canvas. The actual size may differ from this depending on
        platform and other circumstances. Also the canvas size may change at
        any time (for instance when the user resizes the application window,
        or rotates the mobile device).

        All provided function callbacks will be called from the same thread,
        but this may be different from the thread where sokol_main() was called.

        .init_cb (void (*)(void))
            This function is called once after the application window,
            3D rendering context and swap chain have been created. The
            function takes no arguments and has no return value.
        .frame_cb (void (*)(void))
            This is the per-frame callback, which is usually called 60
            times per second. This is where your application would update
            most of its state and perform all rendering.
        .cleanup_cb (void (*)(void))
            The cleanup callback is called once right before the application
            quits.
        .event_cb (void (*)(const sapp_event* event))
            The event callback is mainly for input handling, but is also
            used to communicate other types of events to the application. Keep the
            event_cb struct member zero-initialized if your application doesn't require
            event handling.
        .fail_cb (void (*)(const char* msg))
            The fail callback is called when a fatal error is encountered
            during start which doesn't allow the program to continue.
            Providing a callback here gives you a chance to show an error message
            to the user. The default behaviour is SOKOL_LOG(msg)

        As you can see, those 'standard callbacks' don't have a user_data
        argument, so any data that needs to be preserved between callbacks
        must live in global variables. If keeping state in global variables
        is not an option, there's an alternative set of callbacks with
        an additional user_data pointer argument:

        .user_data (void*)
            The user-data argument for the callbacks below
        .init_userdata_cb (void (*)(void* user_data))
        .frame_userdata_cb (void (*)(void* user_data))
        .cleanup_userdata_cb (void (*)(void* user_data))
        .event_userdata_cb (void(*)(const sapp_event* event, void* user_data))
        .fail_userdata_cb (void(*)(const char* msg, void* user_data))
            These are the user-data versions of the callback functions. You
            can mix those with the standard callbacks that don't have the
            user_data argument.

        The function sapp_userdata() can be used to query the user_data
        pointer provided in the sapp_desc struct.

        You can also call sapp_query_desc() to get a copy of the
        original sapp_desc structure.

        NOTE that there's also an alternative compile mode where sokol_app.h
        doesn't "hijack" the main() function. Search below for SOKOL_NO_ENTRY.

    --- Implement the initialization callback function (init_cb), this is called
        once after the rendering surface, 3D API and swap chain have been
        initialized by sokol_app. All sokol-app functions can be called
        from inside the initialization callback, the most useful functions
        at this point are:

        int sapp_width(void)
        int sapp_height(void)
            Returns the current width and height of the default framebuffer in pixels,
            this may change from one frame to the next, and it may be different
            from the initial size provided in the sapp_desc struct.

        float sapp_widthf(void)
        float sapp_heightf(void)
            These are alternatives to sapp_width() and sapp_height() which return
            the default framebuffer size as float values instead of integer. This
            may help to prevent casting back and forth between int and float
            in more strongly typed languages than C and C++.

        int sapp_color_format(void)
        int sapp_depth_format(void)
            The color and depth-stencil pixelformats of the default framebuffer,
            as integer values which are compatible with sokol-gfx's
            sg_pixel_format enum (so that they can be plugged directly in places
            where sg_pixel_format is expected). Possible values are:

                23 == SG_PIXELFORMAT_RGBA8
                27 == SG_PIXELFORMAT_BGRA8
                41 == SG_PIXELFORMAT_DEPTH
                42 == SG_PIXELFORMAT_DEPTH_STENCIL

        int sapp_sample_count(void)
            Return the MSAA sample count of the default framebuffer.

        bool sapp_gles2(void)
            Returns true if a GLES2 or WebGL context has been created. This
            is useful when a GLES3/WebGL2 context was requested but is not
            available so that sokol_app.h had to fallback to GLES2/WebGL.

        const void* sapp_metal_get_device(void)
        const void* sapp_metal_get_renderpass_descriptor(void)
        const void* sapp_metal_get_drawable(void)
            If the Metal backend has been selected, these functions return pointers
            to various Metal API objects required for rendering, otherwise
            they return a null pointer. These void pointers are actually
            Objective-C ids converted with a (ARC) __bridge cast so that
            the ids can be tunnel through C code. Also note that the returned
            pointers to the renderpass-descriptor and drawable may change from one
            frame to the next, only the Metal device object is guaranteed to
            stay the same.

        const void* sapp_macos_get_window(void)
            On macOS, get the NSWindow object pointer, otherwise a null pointer.
            Before being used as Objective-C object, the void* must be converted
            back with a (ARC) __bridge cast.

        const void* sapp_ios_get_window(void)
            On iOS, get the UIWindow object pointer, otherwise a null pointer.
            Before being used as Objective-C object, the void* must be converted
            back with a (ARC) __bridge cast.

        const void* sapp_win32_get_hwnd(void)
            On Windows, get the window's HWND, otherwise a null pointer. The
            HWND has been cast to a void pointer in order to be tunneled
            through code which doesn't include Windows.h.

        const void* sapp_d3d11_get_device(void)
        const void* sapp_d3d11_get_device_context(void)
        const void* sapp_d3d11_get_render_target_view(void)
        const void* sapp_d3d11_get_depth_stencil_view(void)
            Similar to the sapp_metal_* functions, the sapp_d3d11_* functions
            return pointers to D3D11 API objects required for rendering,
            only if the D3D11 backend has been selected. Otherwise they
            return a null pointer. Note that the returned pointers to the
            render-target-view and depth-stencil-view may change from one
            frame to the next!

        const void* sapp_wgpu_get_device(void)
        const void* sapp_wgpu_get_render_view(void)
        const void* sapp_wgpu_get_resolve_view(void)
        const void* sapp_wgpu_get_depth_stencil_view(void)
            These are the WebGPU-specific functions to get the WebGPU
            objects and values required for rendering. If sokol_app.h
            is not compiled with SOKOL_WGPU, these functions return null.

        const void* sapp_android_get_native_activity(void);
            On Android, get the native activity ANativeActivity pointer, otherwise
            a null pointer.

    --- Implement the frame-callback function, this function will be called
        on the same thread as the init callback, but might be on a different
        thread than the sokol_main() function. Note that the size of
        the rendering framebuffer might have changed since the frame callback
        was called last. Call the functions sapp_width() and sapp_height()
        each frame to get the current size.

    --- Optionally implement the event-callback to handle input events.
        sokol-app provides the following type of input events:
            - a 'virtual key' was pressed down or released
            - a single text character was entered (provided as UTF-32 code point)
            - a mouse button was pressed down or released (left, right, middle)
            - mouse-wheel or 2D scrolling events
            - the mouse was moved
            - the mouse has entered or left the application window boundaries
            - low-level, portable multi-touch events (began, moved, ended, cancelled)
            - the application window was resized, iconified or restored
            - the application was suspended or restored (on mobile platforms)
            - the user or application code has asked to quit the application
            - a string was pasted to the system clipboard
            - one or more files have been dropped onto the application window

        To explicitly 'consume' an event and prevent that the event is
        forwarded for further handling to the operating system, call
        sapp_consume_event() from inside the event handler (NOTE that
        this behaviour is currently only implemented for some HTML5
        events, support for other platforms and event types will
        be added as needed, please open a github ticket and/or provide
        a PR if needed).

        NOTE: Do *not* call any 3D API rendering functions in the event
        callback function, since the 3D API context may not be active when the
        event callback is called (it may work on some platforms and 3D APIs,
        but not others, and the exact behaviour may change between
        sokol-app versions).

    --- Implement the cleanup-callback function, this is called once
        after the user quits the application (see the section
        "APPLICATION QUIT" for detailed information on quitting
        behaviour, and how to intercept a pending quit - for instance to show a
        "Really Quit?" dialog box). Note that the cleanup-callback isn't
        guaranteed to be called on the web and mobile platforms.

    MOUSE LOCK (AKA POINTER LOCK, AKA MOUSE CAPTURE)
    ================================================
    In normal mouse mode, no mouse movement events are reported when the
    mouse leaves the windows client area or hits the screen border (whether
    it's one or the other depends on the platform), and the mouse move events
    (SAPP_EVENTTYPE_MOUSE_MOVE) contain absolute mouse positions in
    framebuffer pixels in the sapp_event items mouse_x and mouse_y, and
    relative movement in framebuffer pixels in the sapp_event items mouse_dx
    and mouse_dy.

    To get continuous mouse movement (also when the mouse leaves the window
    client area or hits the screen border), activate mouse-lock mode
    by calling:

        sapp_lock_mouse(true)

    When mouse lock is activated, the mouse pointer is hidden, the
    reported absolute mouse position (sapp_event.mouse_x/y) appears
    frozen, and the relative mouse movement in sapp_event.mouse_dx/dy
    no longer has a direct relation to framebuffer pixels but instead
    uses "raw mouse input" (what "raw mouse input" exactly means also
    differs by platform).

    To deactivate mouse lock and return to normal mouse mode, call

        sapp_lock_mouse(false)

    And finally, to check if mouse lock is currently active, call

        if (sapp_mouse_locked()) { ... }

    On native platforms, the sapp_lock_mouse() and sapp_mouse_locked()
    functions work as expected (mouse lock is activated or deactivated
    immediately when sapp_lock_mouse() is called, and sapp_mouse_locked()
    also immediately returns the new state after sapp_lock_mouse()
    is called.

    On the web platform, sapp_lock_mouse() and sapp_mouse_locked() behave
    differently, as dictated by the limitations of the HTML5 Pointer Lock API:

        - sapp_lock_mouse(true) can be called at any time, but it will
          only take effect in a 'short-lived input event handler of a specific
          type', meaning when one of the following events happens:
            - SAPP_EVENTTYPE_MOUSE_DOWN
            - SAPP_EVENTTYPE_MOUSE_UP
            - SAPP_EVENTTYPE_MOUSE_SCROLL
            - SAPP_EVENTYTPE_KEY_UP
            - SAPP_EVENTTYPE_KEY_DOWN
        - The mouse lock/unlock action on the web platform is asynchronous,
          this means that sapp_mouse_locked() won't immediately return
          the new status after calling sapp_lock_mouse(), instead the
          reported status will only change when the pointer lock has actually
          been activated or deactivated in the browser.
        - On the web, mouse lock can be deactivated by the user at any time
          by pressing the Esc key. When this happens, sokol_app.h behaves
          the same as if sapp_lock_mouse(false) is called.

    For things like camera manipulation it's most straightforward to lock
    and unlock the mouse right from the sokol_app.h event handler, for
    instance the following code enters and leaves mouse lock when the
    left mouse button is pressed and released, and then uses the relative
    movement information to manipulate a camera (taken from the
    cgltf-sapp.c sample in the sokol-samples repository
    at https://github.com/floooh/sokol-samples):

        static void input(const sapp_event* ev) {
            switch (ev->type) {
                case SAPP_EVENTTYPE_MOUSE_DOWN:
                    if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
                        sapp_lock_mouse(true);
                    }
                    break;

                case SAPP_EVENTTYPE_MOUSE_UP:
                    if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
                        sapp_lock_mouse(false);
                    }
                    break;

                case SAPP_EVENTTYPE_MOUSE_MOVE:
                    if (sapp_mouse_locked()) {
                        cam_orbit(&state.camera, ev->mouse_dx * 0.25f, ev->mouse_dy * 0.25f);
                    }
                    break;

                default:
                    break;
            }
        }

    CLIPBOARD SUPPORT
    =================
    Applications can send and receive UTF-8 encoded text data from and to the
    system clipboard. By default, clipboard support is disabled and
    must be enabled at startup via the following sapp_desc struct
    members:

        sapp_desc.enable_clipboard  - set to true to enable clipboard support
        sapp_desc.clipboard_size    - size of the internal clipboard buffer in bytes

    Enabling the clipboard will dynamically allocate a clipboard buffer
    for UTF-8 encoded text data of the requested size in bytes, the default
    size is 8 KBytes. Strings that don't fit into the clipboard buffer
    (including the terminating zero) will be silently clipped, so it's
    important that you provide a big enough clipboard size for your
    use case.

    To send data to the clipboard, call sapp_set_clipboard_string() with
    a pointer to an UTF-8 encoded, null-terminated C-string.

    NOTE that on the HTML5 platform, sapp_set_clipboard_string() must be
    called from inside a 'short-lived event handler', and there are a few
    other HTML5-specific caveats to workaround. You'll basically have to
    tinker until it works in all browsers :/ (maybe the situation will
    improve when all browsers agree on and implement the new
    HTML5 navigator.clipboard API).

    To get data from the clipboard, check for the SAPP_EVENTTYPE_CLIPBOARD_PASTED
    event in your event handler function, and then call sapp_get_clipboard_string()
    to obtain the pasted UTF-8 encoded text.

    NOTE that behaviour of sapp_get_clipboard_string() is slightly different
    depending on platform:

        - on the HTML5 platform, the internal clipboard buffer will only be updated
          right before the SAPP_EVENTTYPE_CLIPBOARD_PASTED event is sent,
          and sapp_get_clipboard_string() will simply return the current content
          of the clipboard buffer
        - on 'native' platforms, the call to sapp_get_clipboard_string() will
          update the internal clipboard buffer with the most recent data
          from the system clipboard

    Portable code should check for the SAPP_EVENTTYPE_CLIPBOARD_PASTED event,
    and then call sapp_get_clipboard_string() right in the event handler.

    The SAPP_EVENTTYPE_CLIPBOARD_PASTED event will be generated by sokol-app
    as follows:

        - on macOS: when the Cmd+V key is pressed down
        - on HTML5: when the browser sends a 'paste' event to the global 'window' object
        - on all other platforms: when the Ctrl+V key is pressed down

    DRAG AND DROP SUPPORT
    =====================
    PLEASE NOTE: the drag'n'drop feature works differently on WASM/HTML5
    and on the native desktop platforms (Win32, Linux and macOS) because
    of security-related restrictions in the HTML5 drag'n'drop API. The
    WASM/HTML5 specifics are described at the end of this documentation
    section:

    Like clipboard support, drag'n'drop support must be explicitly enabled
    at startup in the sapp_desc struct.

        sapp_desc sokol_main() {
            return (sapp_desc) {
                .enable_dragndrop = true,   // default is false
                ...
            };
        }

    You can also adjust the maximum number of files that are accepted
    in a drop operation, and the maximum path length in bytes if needed:

        sapp_desc sokol_main() {
            return (sapp_desc) {
                .enable_dragndrop = true,               // default is false
                .max_dropped_files = 8,                 // default is 1
                .max_dropped_file_path_length = 8192,   // in bytes, default is 2048
                ...
            };
        }

    When drag'n'drop is enabled, the event callback will be invoked with an
    event of type SAPP_EVENTTYPE_FILES_DROPPED whenever the user drops files on
    the application window.

    After the SAPP_EVENTTYPE_FILES_DROPPED is received, you can query the
    number of dropped files, and their absolute paths by calling separate
    functions:

        void on_event(const sapp_event* ev) {
            if (ev->type == SAPP_EVENTTYPE_FILES_DROPPED) {

                // the mouse position where the drop happened
                float x = ev->mouse_x;
                float y = ev->mouse_y;

                // get the number of files and their paths like this:
                const int num_dropped_files = sapp_get_num_dropped_files();
                for (int i = 0; i < num_dropped_files; i++) {
                    const char* path = sapp_get_dropped_file_path(i);
                    ...
                }
            }
        }

    The returned file paths are UTF-8 encoded strings.

    You can call sapp_get_num_dropped_files() and sapp_get_dropped_file_path()
    anywhere, also outside the event handler callback, but be aware that the
    file path strings will be overwritten with the next drop operation.

    In any case, sapp_get_dropped_file_path() will never return a null pointer,
    instead an empty string "" will be returned if the drag'n'drop feature
    hasn't been enabled, the last drop-operation failed, or the file path index
    is out of range.

    Drag'n'drop caveats:

        - if more files are dropped in a single drop-action
          than sapp_desc.max_dropped_files, the additional
          files will be silently ignored
        - if any of the file paths is longer than
          sapp_desc.max_dropped_file_path_length (in number of bytes, after UTF-8
          encoding) the entire drop operation will be silently ignored (this
          needs some sort of error feedback in the future)
        - no mouse positions are reported while the drag is in
          process, this may change in the future

    Drag'n'drop on HTML5/WASM:

    The HTML5 drag'n'drop API doesn't return file paths, but instead
    black-box 'file objects' which must be used to load the content
    of dropped files. This is the reason why sokol_app.h adds two
    HTML5-specific functions to the drag'n'drop API:

        uint32_t sapp_html5_get_dropped_file_size(int index)
            Returns the size in bytes of a dropped file.

        void sapp_html5_fetch_dropped_file(const sapp_html5_fetch_request* request)
            Asynchronously loads the content of a dropped file into a
            provided memory buffer (which must be big enough to hold
            the file content)

    To start loading the first dropped file after an SAPP_EVENTTYPE_FILES_DROPPED
    event is received:

        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fetch_cb
            .buffer_ptr = buf,
            .buffer_size = buf_size,
            .user_data = ...
        });

    Make sure that the memory pointed to by 'buf' stays valid until the
    callback function is called!

    As result of the asynchronous loading operation (no matter if succeeded or
    failed) the 'fetch_cb' function will be called:

        void fetch_cb(const sapp_html5_fetch_response* response) {
            // IMPORTANT: check if the loading operation actually succeeded:
            if (response->succeeded) {
                // the size of the loaded file:
                const uint32_t num_bytes = response->fetched_size;
                // and the pointer to the data (same as 'buf' in the fetch-call):
                const void* ptr = response->buffer_ptr;
            }
            else {
                // on error check the error code:
                switch (response->error_code) {
                    case SAPP_HTML5_FETCH_ERROR_BUFFER_TOO_SMALL:
                        ...
                        break;
                    case SAPP_HTML5_FETCH_ERROR_OTHER:
                        ...
                        break;
                }
            }
        }

    Check the droptest-sapp example for a real-world example which works
    both on native platforms and the web:

    https://github.com/floooh/sokol-samples/blob/master/sapp/droptest-sapp.c

    HIGH-DPI RENDERING
    ==================
    You can set the sapp_desc.high_dpi flag during initialization to request
    a full-resolution framebuffer on HighDPI displays. The default behaviour
    is sapp_desc.high_dpi=false, this means that the application will
    render to a lower-resolution framebuffer on HighDPI displays and the
    rendered content will be upscaled by the window system composer.

    In a HighDPI scenario, you still request the same window size during
    sokol_main(), but the framebuffer sizes returned by sapp_width()
    and sapp_height() will be scaled up according to the DPI scaling
    ratio. You can also get a DPI scaling factor with the function
    sapp_dpi_scale().

    Here's an example on a Mac with Retina display:

    sapp_desc sokol_main() {
        return (sapp_desc) {
            .width = 640,
            .height = 480,
            .high_dpi = true,
            ...
        };
    }

    The functions sapp_width(), sapp_height() and sapp_dpi_scale() will
    return the following values:

    sapp_width      -> 1280
    sapp_height     -> 960
    sapp_dpi_scale  -> 2.0

    If the high_dpi flag is false, or you're not running on a Retina display,
    the values would be:

    sapp_width      -> 640
    sapp_height     -> 480
    sapp_dpi_scale  -> 1.0

    APPLICATION QUIT
    ================
    Without special quit handling, a sokol_app.h application will quit
    'gracefully' when the user clicks the window close-button unless a
    platform's application model prevents this (e.g. on web or mobile).
    'Graceful exit' means that the application-provided cleanup callback will
    be called before the application quits.

    On native desktop platforms sokol_app.h provides more control over the
    application-quit-process. It's possible to initiate a 'programmatic quit'
    from the application code, and a quit initiated by the application user can
    be intercepted (for instance to show a custom dialog box).

    This 'programmatic quit protocol' is implemented through 3 functions
    and 1 event:

        - sapp_quit(): This function simply quits the application without
          giving the user a chance to intervene. Usually this might
          be called when the user clicks the 'Ok' button in a 'Really Quit?'
          dialog box
        - sapp_request_quit(): Calling sapp_request_quit() will send the
          event SAPP_EVENTTYPE_QUIT_REQUESTED to the applications event handler
          callback, giving the user code a chance to intervene and cancel the
          pending quit process (for instance to show a 'Really Quit?' dialog
          box). If the event handler callback does nothing, the application
          will be quit as usual. To prevent this, call the function
          sapp_cancel_quit() from inside the event handler.
        - sapp_cancel_quit(): Cancels a pending quit request, either initiated
          by the user clicking the window close button, or programmatically
          by calling sapp_request_quit(). The only place where calling this
          function makes sense is from inside the event handler callback when
          the SAPP_EVENTTYPE_QUIT_REQUESTED event has been received.
        - SAPP_EVENTTYPE_QUIT_REQUESTED: this event is sent when the user
          clicks the window's close button or application code calls the
          sapp_request_quit() function. The event handler callback code can handle
          this event by calling sapp_cancel_quit() to cancel the quit.
          If the event is ignored, the application will quit as usual.

    On the web platform, the quit behaviour differs from native platforms,
    because of web-specific restrictions:

    A `programmatic quit` initiated by calling sapp_quit() or
    sapp_request_quit() will work as described above: the cleanup callback is
    called, platform-specific cleanup is performed (on the web
    this means that JS event handlers are unregisters), and then
    the request-animation-loop will be exited. However that's all. The
    web page itself will continue to exist (e.g. it's not possible to
    programmatically close the browser tab).

    On the web it's also not possible to run custom code when the user
    closes a brower tab, so it's not possible to prevent this with a
    fancy custom dialog box.

    Instead the standard "Leave Site?" dialog box can be activated (or
    deactivated) with the following function:

        sapp_html5_ask_leave_site(bool ask);

    The initial state of the associated internal flag can be provided
    at startup via sapp_desc.html5_ask_leave_site.

    This feature should only be used sparingly in critical situations - for
    instance when the user would loose data - since popping up modal dialog
    boxes is considered quite rude in the web world. Note that there's no way
    to customize the content of this dialog box or run any code as a result
    of the user's decision. Also note that the user must have interacted with
    the site before the dialog box will appear. These are all security measures
    to prevent fishing.

    The Dear ImGui HighDPI sample contains example code of how to
    implement a 'Really Quit?' dialog box with Dear ImGui (native desktop
    platforms only), and for showing the hardwired "Leave Site?" dialog box
    when running on the web platform:

        https://floooh.github.io/sokol-html5/wasm/imgui-highdpi-sapp.html

    FULLSCREEN
    ==========
    If the sapp_desc.fullscreen flag is true, sokol-app will try to create
    a fullscreen window on platforms with a 'proper' window system
    (mobile devices will always use fullscreen). The implementation details
    depend on the target platform, in general sokol-app will use a
    'soft approach' which doesn't interfere too much with the platform's
    window system (for instance borderless fullscreen window instead of
    a 'real' fullscreen mode). Such details might change over time
    as sokol-app is adapted for different needs.

    The most important effect of fullscreen mode to keep in mind is that
    the requested canvas width and height will be ignored for the initial
    window size, calling sapp_width() and sapp_height() will instead return
    the resolution of the fullscreen canvas (however the provided size
    might still be used for the non-fullscreen window, in case the user can
    switch back from fullscreen- to windowed-mode).

    To toggle fullscreen mode programmatically, call sapp_toggle_fullscreen().

    To check if the application window is currently in fullscreen mode,
    call sapp_is_fullscreen().

    WINDOW ICON SUPPORT
    ===================
    Some sokol_app.h backends allow to change the window icon programmatically:

        - on Win32: the small icon in the window's title bar, and the
          bigger icon in the task bar
        - on Linux: highly dependent on the used window manager, but usually
          the window's title bar icon and/or the task bar icon
        - on HTML5: the favicon shown in the page's browser tab

    NOTE that it is not possible to set the actual application icon which is
    displayed by the operating system on the desktop or 'home screen'. Those
    icons must be provided 'traditionally' through operating-system-specific
    resources which are associated with the application (sokol_app.h might
    later support setting the window icon from platform specific resource data
    though).

    There are two ways to set the window icon:

        - at application start in the sokol_main() function by initializing
          the sapp_desc.icon nested struct
        - or later by calling the function sapp_set_icon()

    As a convenient shortcut, sokol_app.h comes with a builtin default-icon
    (a rainbow-colored 'S', which at least looks a bit better than the Windows
    default icon for applications), which can be activated like this:

    At startup in sokol_main():

        sapp_desc sokol_main(...) {
            return (sapp_desc){
                ...
                icon.sokol_default = true
            };
        }

    Or later by calling:

        sapp_set_icon(&(sapp_icon_desc){ .sokol_default = true });

    NOTE that a completely zero-initialized sapp_icon_desc struct will not
    update the window icon in any way. This is an 'escape hatch' so that you
    can handle the window icon update yourself (or if you do this already,
    sokol_app.h won't get in your way, in this case just leave the
    sapp_desc.icon struct zero-initialized).

    Providing your own icon images works exactly like in GLFW (down to the
    data format):

    You provide one or more 'candidate images' in different sizes, and the
    sokol_app.h platform backends pick the best match for the specific backend
    and icon type.

    For each candidate image, you need to provide:

        - the width in pixels
        - the height in pixels
        - and the actual pixel data in RGBA8 pixel format (e.g. 0xFFCC8844
          on a little-endian CPU means: alpha=0xFF, blue=0xCC, green=0x88, red=0x44)

    For instance, if you have 3 candidate images (small, medium, big) of
    sizes 16x16, 32x32 and 64x64 the corresponding sapp_icon_desc struct is setup
    like this:

        // the actual pixel data (RGBA8, origin top-left)
        const uint32_t small[16][16]  = { ... };
        const uint32_t medium[32][32] = { ... };
        const uint32_t big[64][64]    = { ... };

        const sapp_icon_desc icon_desc = {
            .images = {
                { .width = 16, .height = 16, .pixels = SAPP_RANGE(small) },
                { .width = 32, .height = 32, .pixels = SAPP_RANGE(medium) },
                // ...or without the SAPP_RANGE helper macro:
                { .width = 64, .height = 64, .pixels = { .ptr=big, .size=sizeof(big) } }
            }
        };

    An sapp_icon_desc struct initialized like this can then either be applied
    at application start in sokol_main:

        sapp_desc sokol_main(...) {
            return (sapp_desc){
                ...
                icon = icon_desc
            };
        }

    ...or later by calling sapp_set_icon():

        sapp_set_icon(&icon_desc);

    Some window icon caveats:

        - once the window icon has been updated, there's no way to go back to
          the platform's default icon, this is because some platforms (Linux
          and HTML5) don't switch the icon visual back to the default even if
          the custom icon is deleted or removed
        - on HTML5, if the sokol_app.h icon doesn't show up in the browser
          tab, check that there's no traditional favicon 'link' element
          is defined in the page's index.html, sokol_app.h will only
          append a new favicon link element, but not delete any manually
          defined favicon in the page

    For an example and test of the window icon feature, check out the the
    'icon-sapp' sample on the sokol-samples git repository.

    ONSCREEN KEYBOARD
    =================
    On some platforms which don't provide a physical keyboard, sokol-app
    can display the platform's integrated onscreen keyboard for text
    input. To request that the onscreen keyboard is shown, call

        sapp_show_keyboard(true);

    Likewise, to hide the keyboard call:

        sapp_show_keyboard(false);

    Note that on the web platform, the keyboard can only be shown from
    inside an input handler. On such platforms, sapp_show_keyboard()
    will only work as expected when it is called from inside the
    sokol-app event callback function. When called from other places,
    an internal flag will be set, and the onscreen keyboard will be
    called at the next 'legal' opportunity (when the next input event
    is handled).

    OPTIONAL: DON'T HIJACK main() (#define SOKOL_NO_ENTRY)
    ======================================================
    In its default configuration, sokol_app.h "hijacks" the platform's
    standard main() function. This was done because different platforms
    have different main functions which are not compatible with
    C's main() (for instance WinMain on Windows has completely different
    arguments). However, this "main hijacking" posed a problem for
    usage scenarios like integrating sokol_app.h with other languages than
    C or C++, so an alternative SOKOL_NO_ENTRY mode has been added
    in which the user code provides the platform's main function:

    - define SOKOL_NO_ENTRY before including the sokol_app.h implementation
    - do *not* provide a sokol_main() function
    - instead provide the standard main() function of the platform
    - from the main function, call the function ```sapp_run()``` which
      takes a pointer to an ```sapp_desc``` structure.
    - ```sapp_run()``` takes over control and calls the provided init-, frame-,
      shutdown- and event-callbacks just like in the default model, it
      will only return when the application quits (or not at all on some
      platforms, like emscripten)

    NOTE: SOKOL_NO_ENTRY is currently not supported on Android.

    WINDOWS CONSOLE OUTPUT
    ======================
    On Windows, regular windowed applications don't show any stdout/stderr text
    output, which can be a bit of a hassle for printf() debugging or generally
    logging text to the console. Also, console output by default uses a local
    codepage setting and thus international UTF-8 encoded text is printed
    as garbage.

    To help with these issues, sokol_app.h can be configured at startup
    via the following Windows-specific sapp_desc flags:

        sapp_desc.win32_console_utf8 (default: false)
            When set to true, the output console codepage will be switched
            to UTF-8 (and restored to the original codepage on exit)

        sapp_desc.win32_console_attach (default: false)
            When set to true, stdout and stderr will be attached to the
            console of the parent process (if the parent process actually
            has a console). This means that if the application was started
            in a command line window, stdout and stderr output will be printed
            to the terminal, just like a regular command line program. But if
            the application is started via double-click, it will behave like
            a regular UI application, and stdout/stderr will not be visible.

        sapp_desc.win32_console_create (default: false)
            When set to true, a new console window will be created and
            stdout/stderr will be redirected to that console window. It
            doesn't matter if the application is started from the command
            line or via double-click.

    TEMP NOTE DUMP
    ==============
    - onscreen keyboard support on Android requires Java :(, should we even bother?
    - sapp_desc needs a bool whether to initialize depth-stencil surface
    - GL context initialization needs more control (at least what GL version to initialize)
    - application icon
    - the UPDATE_CURSOR event currently behaves differently between Win32 and OSX
      (Win32 sends the event each frame when the mouse moves and is inside the window
      client area, OSX sends it only once when the mouse enters the client area)
    - the Android implementation calls cleanup_cb() and destroys the egl context in onDestroy
      at the latest but should do it earlier, in onStop, as an app is "killable" after onStop
      on Android Honeycomb and later (it can't be done at the moment as the app may be started
      again after onStop and the sokol lifecycle does not yet handle context teardown/bringup)


    LICENSE
    =======
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
#define SOKOL_APP_INCLUDED (1)
#include <stddef.h> // size_t
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_APP_API_DECL)
#define SOKOL_APP_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_APP_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_APP_IMPL)
#define SOKOL_APP_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_APP_API_DECL __declspec(dllimport)
#else
#define SOKOL_APP_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* misc constants */
enum {
    SAPP_MAX_TOUCHPOINTS = 8,
    SAPP_MAX_MOUSEBUTTONS = 3,
    SAPP_MAX_KEYCODES = 512,
    SAPP_MAX_ICONIMAGES = 8,
};

/*
    sapp_event_type

    The type of event that's passed to the event handler callback
    in the sapp_event.type field. These are not just "traditional"
    input events, but also notify the application about state changes
    or other user-invoked actions.
*/
typedef enum sapp_event_type {
    SAPP_EVENTTYPE_INVALID,
    SAPP_EVENTTYPE_KEY_DOWN,
    SAPP_EVENTTYPE_KEY_UP,
    SAPP_EVENTTYPE_CHAR,
    SAPP_EVENTTYPE_MOUSE_DOWN,
    SAPP_EVENTTYPE_MOUSE_UP,
    SAPP_EVENTTYPE_MOUSE_SCROLL,
    SAPP_EVENTTYPE_MOUSE_MOVE,
    SAPP_EVENTTYPE_MOUSE_ENTER,
    SAPP_EVENTTYPE_MOUSE_LEAVE,
    SAPP_EVENTTYPE_TOUCHES_BEGAN,
    SAPP_EVENTTYPE_TOUCHES_MOVED,
    SAPP_EVENTTYPE_TOUCHES_ENDED,
    SAPP_EVENTTYPE_TOUCHES_CANCELLED,
    SAPP_EVENTTYPE_RESIZED,
    SAPP_EVENTTYPE_ICONIFIED,
    SAPP_EVENTTYPE_RESTORED,
    SAPP_EVENTTYPE_FOCUSED,
    SAPP_EVENTTYPE_UNFOCUSED,
    SAPP_EVENTTYPE_SUSPENDED,
    SAPP_EVENTTYPE_RESUMED,
    SAPP_EVENTTYPE_UPDATE_CURSOR,
    SAPP_EVENTTYPE_QUIT_REQUESTED,
    SAPP_EVENTTYPE_CLIPBOARD_PASTED,
    SAPP_EVENTTYPE_FILES_DROPPED,
    _SAPP_EVENTTYPE_NUM,
    _SAPP_EVENTTYPE_FORCE_U32 = 0x7FFFFFFF
} sapp_event_type;

/*
    sapp_keycode

    The 'virtual keycode' of a KEY_DOWN or KEY_UP event in the
    struct field sapp_event.key_code.

    Note that the keycode values are identical with GLFW.
*/
typedef enum sapp_keycode {
    SAPP_KEYCODE_INVALID          = 0,
    SAPP_KEYCODE_SPACE            = 32,
    SAPP_KEYCODE_APOSTROPHE       = 39,  /* ' */
    SAPP_KEYCODE_COMMA            = 44,  /* , */
    SAPP_KEYCODE_MINUS            = 45,  /* - */
    SAPP_KEYCODE_PERIOD           = 46,  /* . */
    SAPP_KEYCODE_SLASH            = 47,  /* / */
    SAPP_KEYCODE_0                = 48,
    SAPP_KEYCODE_1                = 49,
    SAPP_KEYCODE_2                = 50,
    SAPP_KEYCODE_3                = 51,
    SAPP_KEYCODE_4                = 52,
    SAPP_KEYCODE_5                = 53,
    SAPP_KEYCODE_6                = 54,
    SAPP_KEYCODE_7                = 55,
    SAPP_KEYCODE_8                = 56,
    SAPP_KEYCODE_9                = 57,
    SAPP_KEYCODE_SEMICOLON        = 59,  /* ; */
    SAPP_KEYCODE_EQUAL            = 61,  /* = */
    SAPP_KEYCODE_A                = 65,
    SAPP_KEYCODE_B                = 66,
    SAPP_KEYCODE_C                = 67,
    SAPP_KEYCODE_D                = 68,
    SAPP_KEYCODE_E                = 69,
    SAPP_KEYCODE_F                = 70,
    SAPP_KEYCODE_G                = 71,
    SAPP_KEYCODE_H                = 72,
    SAPP_KEYCODE_I                = 73,
    SAPP_KEYCODE_J                = 74,
    SAPP_KEYCODE_K                = 75,
    SAPP_KEYCODE_L                = 76,
    SAPP_KEYCODE_M                = 77,
    SAPP_KEYCODE_N                = 78,
    SAPP_KEYCODE_O                = 79,
    SAPP_KEYCODE_P                = 80,
    SAPP_KEYCODE_Q                = 81,
    SAPP_KEYCODE_R                = 82,
    SAPP_KEYCODE_S                = 83,
    SAPP_KEYCODE_T                = 84,
    SAPP_KEYCODE_U                = 85,
    SAPP_KEYCODE_V                = 86,
    SAPP_KEYCODE_W                = 87,
    SAPP_KEYCODE_X                = 88,
    SAPP_KEYCODE_Y                = 89,
    SAPP_KEYCODE_Z                = 90,
    SAPP_KEYCODE_LEFT_BRACKET     = 91,  /* [ */
    SAPP_KEYCODE_BACKSLASH        = 92,  /* \ */
    SAPP_KEYCODE_RIGHT_BRACKET    = 93,  /* ] */
    SAPP_KEYCODE_GRAVE_ACCENT     = 96,  /* ` */
    SAPP_KEYCODE_WORLD_1          = 161, /* non-US #1 */
    SAPP_KEYCODE_WORLD_2          = 162, /* non-US #2 */
    SAPP_KEYCODE_ESCAPE           = 256,
    SAPP_KEYCODE_ENTER            = 257,
    SAPP_KEYCODE_TAB              = 258,
    SAPP_KEYCODE_BACKSPACE        = 259,
    SAPP_KEYCODE_INSERT           = 260,
    SAPP_KEYCODE_DELETE           = 261,
    SAPP_KEYCODE_RIGHT            = 262,
    SAPP_KEYCODE_LEFT             = 263,
    SAPP_KEYCODE_DOWN             = 264,
    SAPP_KEYCODE_UP               = 265,
    SAPP_KEYCODE_PAGE_UP          = 266,
    SAPP_KEYCODE_PAGE_DOWN        = 267,
    SAPP_KEYCODE_HOME             = 268,
    SAPP_KEYCODE_END              = 269,
    SAPP_KEYCODE_CAPS_LOCK        = 280,
    SAPP_KEYCODE_SCROLL_LOCK      = 281,
    SAPP_KEYCODE_NUM_LOCK         = 282,
    SAPP_KEYCODE_PRINT_SCREEN     = 283,
    SAPP_KEYCODE_PAUSE            = 284,
    SAPP_KEYCODE_F1               = 290,
    SAPP_KEYCODE_F2               = 291,
    SAPP_KEYCODE_F3               = 292,
    SAPP_KEYCODE_F4               = 293,
    SAPP_KEYCODE_F5               = 294,
    SAPP_KEYCODE_F6               = 295,
    SAPP_KEYCODE_F7               = 296,
    SAPP_KEYCODE_F8               = 297,
    SAPP_KEYCODE_F9               = 298,
    SAPP_KEYCODE_F10              = 299,
    SAPP_KEYCODE_F11              = 300,
    SAPP_KEYCODE_F12              = 301,
    SAPP_KEYCODE_F13              = 302,
    SAPP_KEYCODE_F14              = 303,
    SAPP_KEYCODE_F15              = 304,
    SAPP_KEYCODE_F16              = 305,
    SAPP_KEYCODE_F17              = 306,
    SAPP_KEYCODE_F18              = 307,
    SAPP_KEYCODE_F19              = 308,
    SAPP_KEYCODE_F20              = 309,
    SAPP_KEYCODE_F21              = 310,
    SAPP_KEYCODE_F22              = 311,
    SAPP_KEYCODE_F23              = 312,
    SAPP_KEYCODE_F24              = 313,
    SAPP_KEYCODE_F25              = 314,
    SAPP_KEYCODE_KP_0             = 320,
    SAPP_KEYCODE_KP_1             = 321,
    SAPP_KEYCODE_KP_2             = 322,
    SAPP_KEYCODE_KP_3             = 323,
    SAPP_KEYCODE_KP_4             = 324,
    SAPP_KEYCODE_KP_5             = 325,
    SAPP_KEYCODE_KP_6             = 326,
    SAPP_KEYCODE_KP_7             = 327,
    SAPP_KEYCODE_KP_8             = 328,
    SAPP_KEYCODE_KP_9             = 329,
    SAPP_KEYCODE_KP_DECIMAL       = 330,
    SAPP_KEYCODE_KP_DIVIDE        = 331,
    SAPP_KEYCODE_KP_MULTIPLY      = 332,
    SAPP_KEYCODE_KP_SUBTRACT      = 333,
    SAPP_KEYCODE_KP_ADD           = 334,
    SAPP_KEYCODE_KP_ENTER         = 335,
    SAPP_KEYCODE_KP_EQUAL         = 336,
    SAPP_KEYCODE_LEFT_SHIFT       = 340,
    SAPP_KEYCODE_LEFT_CONTROL     = 341,
    SAPP_KEYCODE_LEFT_ALT         = 342,
    SAPP_KEYCODE_LEFT_SUPER       = 343,
    SAPP_KEYCODE_RIGHT_SHIFT      = 344,
    SAPP_KEYCODE_RIGHT_CONTROL    = 345,
    SAPP_KEYCODE_RIGHT_ALT        = 346,
    SAPP_KEYCODE_RIGHT_SUPER      = 347,
    SAPP_KEYCODE_MENU             = 348,
} sapp_keycode;

/*
    sapp_touchpoint

    Describes a single touchpoint in a multitouch event (TOUCHES_BEGAN,
    TOUCHES_MOVED, TOUCHES_ENDED).

    Touch points are stored in the nested array sapp_event.touches[],
    and the number of touches is stored in sapp_event.num_touches.
*/
typedef struct sapp_touchpoint {
    uintptr_t identifier;
    float pos_x;
    float pos_y;
    bool changed;
} sapp_touchpoint;

/*
    sapp_mousebutton

    The currently pressed mouse button in the events MOUSE_DOWN
    and MOUSE_UP, stored in the struct field sapp_event.mouse_button.
*/
typedef enum sapp_mousebutton {
    SAPP_MOUSEBUTTON_LEFT = 0x0,
    SAPP_MOUSEBUTTON_RIGHT = 0x1,
    SAPP_MOUSEBUTTON_MIDDLE = 0x2,
    SAPP_MOUSEBUTTON_INVALID = 0x100,
} sapp_mousebutton;

/*
    These are currently pressed modifier keys (and mouse buttons) which are
    passed in the event struct field sapp_event.modifiers.
*/
enum {
    SAPP_MODIFIER_SHIFT = 0x1,      // left or right shift key
    SAPP_MODIFIER_CTRL  = 0x2,      // left or right control key
    SAPP_MODIFIER_ALT   = 0x4,      // left or right alt key
    SAPP_MODIFIER_SUPER = 0x8,      // left or right 'super' key
    SAPP_MODIFIER_LMB   = 0x100,    // left mouse button
    SAPP_MODIFIER_RMB   = 0x200,    // right mouse button
    SAPP_MODIFIER_MMB   = 0x400,    // middle mouse button
};

/*
    sapp_event

    This is an all-in-one event struct passed to the event handler
    user callback function. Note that it depends on the event
    type what struct fields actually contain useful values, so you
    should first check the event type before reading other struct
    fields.
*/
typedef struct sapp_event {
    uint64_t frame_count;               // current frame counter, always valid, useful for checking if two events were issued in the same frame
    sapp_event_type type;               // the event type, always valid
    sapp_keycode key_code;              // the virtual key code, only valid in KEY_UP, KEY_DOWN
    uint32_t char_code;                 // the UTF-32 character code, only valid in CHAR events
    bool key_repeat;                    // true if this is a key-repeat event, valid in KEY_UP, KEY_DOWN and CHAR
    uint32_t modifiers;                 // current modifier keys, valid in all key-, char- and mouse-events
    sapp_mousebutton mouse_button;      // mouse button that was pressed or released, valid in MOUSE_DOWN, MOUSE_UP
    float mouse_x;                      // current horizontal mouse position in pixels, always valid except during mouse lock
    float mouse_y;                      // current vertical mouse position in pixels, always valid except during mouse lock
    float mouse_dx;                     // relative horizontal mouse movement since last frame, always valid
    float mouse_dy;                     // relative vertical mouse movement since last frame, always valid
    float scroll_x;                     // horizontal mouse wheel scroll distance, valid in MOUSE_SCROLL events
    float scroll_y;                     // vertical mouse wheel scroll distance, valid in MOUSE_SCROLL events
    int num_touches;                    // number of valid items in the touches[] array
    sapp_touchpoint touches[SAPP_MAX_TOUCHPOINTS];  // current touch points, valid in TOUCHES_BEGIN, TOUCHES_MOVED, TOUCHES_ENDED
    int window_width;                   // current window- and framebuffer sizes in pixels, always valid
    int window_height;
    int framebuffer_width;              // = window_width * dpi_scale
    int framebuffer_height;             // = window_height * dpi_scale
} sapp_event;

/*
    sg_range

    A general pointer/size-pair struct and constructor macros for passing binary blobs
    into sokol_app.h.
*/
typedef struct sapp_range {
    const void* ptr;
    size_t size;
} sapp_range;
// disabling this for every includer isn't great, but the warnings are also quite pointless
#if defined(_MSC_VER)
#pragma warning(disable:4221)   /* /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y' */
#pragma warning(disable:4204)   /* VS2015: nonstandard extension used: non-constant aggregate initializer */
#endif
#if defined(__cplusplus)
#define SAPP_RANGE(x) sapp_range{ &x, sizeof(x) }
#else
#define SAPP_RANGE(x) (sapp_range){ &x, sizeof(x) }
#endif

/*
    sapp_image_desc

    This is used to describe image data to sokol_app.h (at first, window
    icons, later maybe cursor images).

    Note that the actual image pixel format depends on the use case:

    - window icon pixels are RGBA8
    - cursor images are ??? (FIXME)
*/
typedef struct sapp_image_desc {
    int width;
    int height;
    sapp_range pixels;
} sapp_image_desc;

/*
    sapp_icon_desc

    An icon description structure for use in sapp_desc.icon and
    sapp_set_icon().

    When setting a custom image, the application can provide a number of
    candidates differing in size, and sokol_app.h will pick the image(s)
    closest to the size expected by the platform's window system.

    To set sokol-app's default icon, set .sokol_default to true.

    Otherwise provide candidate images of different sizes in the
    images[] array.

    If both the sokol_default flag is set to true, any image candidates
    will be ignored and the sokol_app.h default icon will be set.
*/
typedef struct sapp_icon_desc {
    bool sokol_default;
    sapp_image_desc images[SAPP_MAX_ICONIMAGES];
} sapp_icon_desc;


typedef struct sapp_desc {
    void (*init_cb)(void);                  // these are the user-provided callbacks without user data
    void (*frame_cb)(void);
    void (*cleanup_cb)(void);
    void (*event_cb)(const sapp_event*);
    void (*fail_cb)(const char*);

    void* user_data;                        // these are the user-provided callbacks with user data
    void (*init_userdata_cb)(void*);
    void (*frame_userdata_cb)(void*);
    void (*cleanup_userdata_cb)(void*);
    void (*event_userdata_cb)(const sapp_event*, void*);
    void (*fail_userdata_cb)(const char*, void*);

    int width;                          // the preferred width of the window / canvas
    int height;                         // the preferred height of the window / canvas
    int sample_count;                   // MSAA sample count
    int swap_interval;                  // the preferred swap interval (ignored on some platforms)
    bool high_dpi;                      // whether the rendering canvas is full-resolution on HighDPI displays
    bool fullscreen;                    // whether the window should be created in fullscreen mode
    bool alpha;                         // whether the framebuffer should have an alpha channel (ignored on some platforms)
    const char* window_title;           // the window title as UTF-8 encoded string
    bool user_cursor;                   // if true, user is expected to manage cursor image in SAPP_EVENTTYPE_UPDATE_CURSOR
    bool enable_clipboard;              // enable clipboard access, default is false
    int clipboard_size;                 // max size of clipboard content in bytes
    bool enable_dragndrop;              // enable file dropping (drag'n'drop), default is false
    int max_dropped_files;              // max number of dropped files to process (default: 1)
    int max_dropped_file_path_length;   // max length in bytes of a dropped UTF-8 file path (default: 2048)
    sapp_icon_desc icon;                // the initial window icon to set

    /* backend-specific options */
    bool gl_force_gles2;                // if true, setup GLES2/WebGL even if GLES3/WebGL2 is available
    bool win32_console_utf8;            // if true, set the output console codepage to UTF-8
    bool win32_console_create;          // if true, attach stdout/stderr to a new console window
    bool win32_console_attach;          // if true, attach stdout/stderr to parent process
    const char* html5_canvas_name;      // the name (id) of the HTML5 canvas element, default is "canvas"
    bool html5_canvas_resize;           // if true, the HTML5 canvas size is set to sapp_desc.width/height, otherwise canvas size is tracked
    bool html5_preserve_drawing_buffer; // HTML5 only: whether to preserve default framebuffer content between frames
    bool html5_premultiplied_alpha;     // HTML5 only: whether the rendered pixels use premultiplied alpha convention
    bool html5_ask_leave_site;          // initial state of the internal html5_ask_leave_site flag (see sapp_html5_ask_leave_site())
    bool ios_keyboard_resizes_canvas;   // if true, showing the iOS keyboard shrinks the canvas
} sapp_desc;

/* HTML5 specific: request and response structs for
   asynchronously loading dropped-file content.
*/
typedef enum sapp_html5_fetch_error {
    SAPP_HTML5_FETCH_ERROR_NO_ERROR,
    SAPP_HTML5_FETCH_ERROR_BUFFER_TOO_SMALL,
    SAPP_HTML5_FETCH_ERROR_OTHER,
} sapp_html5_fetch_error;

typedef struct sapp_html5_fetch_response {
    bool succeeded;         /* true if the loading operation has succeeded */
    sapp_html5_fetch_error error_code;
    int file_index;         /* index of the dropped file (0..sapp_get_num_dropped_filed()-1) */
    uint32_t fetched_size;  /* size in bytes of loaded data */
    void* buffer_ptr;       /* pointer to user-provided buffer which contains the loaded data */
    uint32_t buffer_size;   /* size of user-provided buffer (buffer_size >= fetched_size) */
    void* user_data;        /* user-provided user data pointer */
} sapp_html5_fetch_response;

typedef struct sapp_html5_fetch_request {
    int dropped_file_index;                 /* 0..sapp_get_num_dropped_files()-1 */
    void (*callback)(const sapp_html5_fetch_response*);     /* response callback function pointer (required) */
    void* buffer_ptr;                       /* pointer to buffer to load data into */
    uint32_t buffer_size;                   /* size in bytes of buffer */
    void* user_data;                        /* optional userdata pointer */
} sapp_html5_fetch_request;

/* user-provided functions */
extern sapp_desc sokol_main(int argc, char* argv[]);

/* returns true after sokol-app has been initialized */
SOKOL_APP_API_DECL bool sapp_isvalid(void);
/* returns the current framebuffer width in pixels */
SOKOL_APP_API_DECL int sapp_width(void);
/* same as sapp_width(), but returns float */
SOKOL_APP_API_DECL float sapp_widthf(void);
/* returns the current framebuffer height in pixels */
SOKOL_APP_API_DECL int sapp_height(void);
/* same as sapp_height(), but returns float */
SOKOL_APP_API_DECL float sapp_heightf(void);
/* get default framebuffer color pixel format */
SOKOL_APP_API_DECL int sapp_color_format(void);
/* get default framebuffer depth pixel format */
SOKOL_APP_API_DECL int sapp_depth_format(void);
/* get default framebuffer sample count */
SOKOL_APP_API_DECL int sapp_sample_count(void);
/* returns true when high_dpi was requested and actually running in a high-dpi scenario */
SOKOL_APP_API_DECL bool sapp_high_dpi(void);
/* returns the dpi scaling factor (window pixels to framebuffer pixels) */
SOKOL_APP_API_DECL float sapp_dpi_scale(void);
/* show or hide the mobile device onscreen keyboard */
SOKOL_APP_API_DECL void sapp_show_keyboard(bool show);
/* return true if the mobile device onscreen keyboard is currently shown */
SOKOL_APP_API_DECL bool sapp_keyboard_shown(void);
/* query fullscreen mode */
SOKOL_APP_API_DECL bool sapp_is_fullscreen(void);
/* toggle fullscreen mode */
SOKOL_APP_API_DECL void sapp_toggle_fullscreen(void);
/* show or hide the mouse cursor */
SOKOL_APP_API_DECL void sapp_show_mouse(bool show);
/* show or hide the mouse cursor */
SOKOL_APP_API_DECL bool sapp_mouse_shown(void);
/* enable/disable mouse-pointer-lock mode */
SOKOL_APP_API_DECL void sapp_lock_mouse(bool lock);
/* return true if in mouse-pointer-lock mode (this may toggle a few frames later) */
SOKOL_APP_API_DECL bool sapp_mouse_locked(void);
/* return the userdata pointer optionally provided in sapp_desc */
SOKOL_APP_API_DECL void* sapp_userdata(void);
/* return a copy of the sapp_desc structure */
SOKOL_APP_API_DECL sapp_desc sapp_query_desc(void);
/* initiate a "soft quit" (sends SAPP_EVENTTYPE_QUIT_REQUESTED) */
SOKOL_APP_API_DECL void sapp_request_quit(void);
/* cancel a pending quit (when SAPP_EVENTTYPE_QUIT_REQUESTED has been received) */
SOKOL_APP_API_DECL void sapp_cancel_quit(void);
/* initiate a "hard quit" (quit application without sending SAPP_EVENTTYPE_QUIT_REQUSTED) */
SOKOL_APP_API_DECL void sapp_quit(void);
/* call from inside event callback to consume the current event (don't forward to platform) */
SOKOL_APP_API_DECL void sapp_consume_event(void);
/* get the current frame counter (for comparison with sapp_event.frame_count) */
SOKOL_APP_API_DECL uint64_t sapp_frame_count(void);
/* write string into clipboard */
SOKOL_APP_API_DECL void sapp_set_clipboard_string(const char* str);
/* read string from clipboard (usually during SAPP_EVENTTYPE_CLIPBOARD_PASTED) */
SOKOL_APP_API_DECL const char* sapp_get_clipboard_string(void);
/* set the window title (only on desktop platforms) */
SOKOL_APP_API_DECL void sapp_set_window_title(const char* str);
/* set the window icon (only on Windows and Linux) */
SOKOL_APP_API_DECL void sapp_set_icon(const sapp_icon_desc* icon_desc);
/* gets the total number of dropped files (after an SAPP_EVENTTYPE_FILES_DROPPED event) */
SOKOL_APP_API_DECL int sapp_get_num_dropped_files(void);
/* gets the dropped file paths */
SOKOL_APP_API_DECL const char* sapp_get_dropped_file_path(int index);

/* special run-function for SOKOL_NO_ENTRY (in standard mode this is an empty stub) */
SOKOL_APP_API_DECL void sapp_run(const sapp_desc* desc);

/* GL: return true when GLES2 fallback is active (to detect fallback from GLES3) */
SOKOL_APP_API_DECL bool sapp_gles2(void);

/* HTML5: enable or disable the hardwired "Leave Site?" dialog box */
SOKOL_APP_API_DECL void sapp_html5_ask_leave_site(bool ask);
/* HTML5: get byte size of a dropped file */
SOKOL_APP_API_DECL uint32_t sapp_html5_get_dropped_file_size(int index);
/* HTML5: asynchronously load the content of a dropped file */
SOKOL_APP_API_DECL void sapp_html5_fetch_dropped_file(const sapp_html5_fetch_request* request);

/* Metal: get bridged pointer to Metal device object */
SOKOL_APP_API_DECL const void* sapp_metal_get_device(void);
/* Metal: get bridged pointer to this frame's renderpass descriptor */
SOKOL_APP_API_DECL const void* sapp_metal_get_renderpass_descriptor(void);
/* Metal: get bridged pointer to current drawable */
SOKOL_APP_API_DECL const void* sapp_metal_get_drawable(void);
/* macOS: get bridged pointer to macOS NSWindow */
SOKOL_APP_API_DECL const void* sapp_macos_get_window(void);
/* iOS: get bridged pointer to iOS UIWindow */
SOKOL_APP_API_DECL const void* sapp_ios_get_window(void);

/* D3D11: get pointer to ID3D11Device object */
SOKOL_APP_API_DECL const void* sapp_d3d11_get_device(void);
/* D3D11: get pointer to ID3D11DeviceContext object */
SOKOL_APP_API_DECL const void* sapp_d3d11_get_device_context(void);
/* D3D11: get pointer to IDXGISwapChain object */
SOKOL_APP_API_DECL const void* sapp_d3d11_get_swap_chain(void);
/* D3D11: get pointer to ID3D11RenderTargetView object */
SOKOL_APP_API_DECL const void* sapp_d3d11_get_render_target_view(void);
/* D3D11: get pointer to ID3D11DepthStencilView */
SOKOL_APP_API_DECL const void* sapp_d3d11_get_depth_stencil_view(void);
/* Win32: get the HWND window handle */
SOKOL_APP_API_DECL const void* sapp_win32_get_hwnd(void);

/* WebGPU: get WGPUDevice handle */
SOKOL_APP_API_DECL const void* sapp_wgpu_get_device(void);
/* WebGPU: get swapchain's WGPUTextureView handle for rendering */
SOKOL_APP_API_DECL const void* sapp_wgpu_get_render_view(void);
/* WebGPU: get swapchain's MSAA-resolve WGPUTextureView (may return null) */
SOKOL_APP_API_DECL const void* sapp_wgpu_get_resolve_view(void);
/* WebGPU: get swapchain's WGPUTextureView for the depth-stencil surface */
SOKOL_APP_API_DECL const void* sapp_wgpu_get_depth_stencil_view(void);

/* Android: get native activity handle */
SOKOL_APP_API_DECL const void* sapp_android_get_native_activity(void);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for C++ */
inline void sapp_run(const sapp_desc& desc) { return sapp_run(&desc); }

#endif

// this WinRT specific hack is required when wWinMain is in a static library
#if defined(_MSC_VER) && defined(UNICODE)
#include <winapifamily.h>
#if defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#pragma comment(linker, "/include:wWinMain")
#endif
#endif

#endif // SOKOL_APP_INCLUDED

#ifdef SOKOL_APP_IMPL
#define SOKOL_APP_IMPL_INCLUDED (1)

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif

#include <string.h> // memset
#include <stddef.h> // size_t

uint64_t dummy_stm_time_ns = UINT64_C(1000000000);

typedef struct {
    bool valid;
    sapp_desc desc;
    bool mouse_shown;
    bool mouse_locked;
    bool want_quit;
    bool force_quit;
    uint64_t frame_count;
} dummy_sapp;

dummy_sapp _sapp;

void _sapp_call_event(const sapp_event *ev)
{
    if (_sapp.desc.event_cb) {
        _sapp.desc.event_cb(ev);
    } else if (_sapp.desc.event_userdata_cb) {
        _sapp.desc.event_userdata_cb(ev, _sapp.desc.user_data);
    }
}

int main(int argc, char **argv)
{
    sapp_desc desc = sokol_main(argc, argv);

    _sapp.valid = true;
    _sapp.desc = desc;
    _sapp.mouse_shown = true;
    _sapp.mouse_locked = true;

    if (_sapp.desc.init_cb) {
        _sapp.desc.init_cb();
    } else if (_sapp.desc.init_userdata_cb) {
        _sapp.desc.init_userdata_cb(_sapp.desc.user_data);
    }

    for (;;) {

        if (_sapp.force_quit) break;

        if (_sapp.want_quit) {
            sapp_event ev = { _sapp.frame_count, SAPP_EVENTTYPE_QUIT_REQUESTED };
            _sapp_call_event(&ev);
            if (_sapp.want_quit) break;
        }

        if (_sapp.desc.frame_cb) {
            _sapp.desc.frame_cb();
        } else if (_sapp.desc.frame_userdata_cb) {
            _sapp.desc.frame_userdata_cb(_sapp.desc.user_data);
        }

        dummy_stm_time_ns += UINT64_C(16666666);
        _sapp.frame_count++;
        #if defined(DUMMY_SAPP_MAX_FRAMES)
            if (_sapp.frame_count >= DUMMY_SAPP_MAX_FRAMES) break;
        #endif
    }

    if (_sapp.desc.cleanup_cb) {
        _sapp.desc.cleanup_cb();
    } else if (_sapp.desc.cleanup_userdata_cb) {
        _sapp.desc.cleanup_userdata_cb(_sapp.desc.user_data);
    }

    return 0;
}

/* returns true after sokol-app has been initialized */
SOKOL_API_IMPL bool sapp_isvalid(void) { return _sapp.valid; }
/* returns the current framebuffer width in pixels */
SOKOL_API_IMPL int sapp_width(void) { return _sapp.desc.width; }
/* same as sapp_width(), but returns float */
SOKOL_API_IMPL float sapp_widthf(void) { return (float)_sapp.desc.width; }
/* returns the current framebuffer height in pixels */
SOKOL_API_IMPL int sapp_height(void) { return _sapp.desc.height; }
/* same as sapp_height(), but returns float */
SOKOL_API_IMPL float sapp_heightf(void) { return (float)_sapp.desc.height; }
/* get default framebuffer color pixel format */
SOKOL_API_IMPL int sapp_color_format(void) { return 23; /* _SAPP_PIXELFORMAT_RGBA8 */ }
/* get default framebuffer depth pixel format */
SOKOL_API_IMPL int sapp_depth_format(void) { return 42; /* _SAPP_PIXELFORMAT_DEPTH_STENCIL */ }
/* get default framebuffer sample count */
SOKOL_API_IMPL int sapp_sample_count(void) { return _sapp.desc.sample_count; }
/* returns true when high_dpi was requested and actually running in a high-dpi scenario */
SOKOL_API_IMPL bool sapp_high_dpi(void) { return _sapp.desc.high_dpi; }
/* returns the dpi scaling factor (window pixels to framebuffer pixels) */
SOKOL_API_IMPL float sapp_dpi_scale(void) { return 1.0f; }
/* show or hide the mobile device onscreen keyboard */
SOKOL_API_IMPL void sapp_show_keyboard(bool show) { }
/* return true if the mobile device onscreen keyboard is currently shown */
SOKOL_API_IMPL bool sapp_keyboard_shown(void) { return false; }
/* query fullscreen mode */
SOKOL_API_IMPL bool sapp_is_fullscreen(void) { return _sapp.desc.fullscreen; }
/* toggle fullscreen mode */
SOKOL_API_IMPL void sapp_toggle_fullscreen(void) { _sapp.desc.fullscreen = !_sapp.desc.fullscreen; }
/* show or hide the mouse cursor */
SOKOL_API_IMPL void sapp_show_mouse(bool show) { _sapp.mouse_shown = show; }
/* show or hide the mouse cursor */
SOKOL_API_IMPL bool sapp_mouse_shown(void) { return _sapp.mouse_shown; }
/* enable/disable mouse-pointer-lock mode */
SOKOL_API_IMPL void sapp_lock_mouse(bool lock) { _sapp.mouse_locked = lock; }
/* return true if in mouse-pointer-lock mode (this may toggle a few frames later) */
SOKOL_APP_API_DECL bool sapp_mouse_locked(void) { return _sapp.mouse_locked; }
/* return the userdata pointer optionally provided in sapp_desc */
SOKOL_API_IMPL void* sapp_userdata(void) { return _sapp.desc.user_data; }
/* return a copy of the sapp_desc structure */
SOKOL_API_IMPL sapp_desc sapp_query_desc(void) { return _sapp.desc; }
/* initiate a "soft quit" (sends SAPP_EVENTTYPE_QUIT_REQUESTED) */
SOKOL_API_IMPL void sapp_request_quit(void) { _sapp.want_quit = true; }
/* cancel a pending quit (when SAPP_EVENTTYPE_QUIT_REQUESTED has been received) */
SOKOL_API_IMPL void sapp_cancel_quit(void) { _sapp.want_quit = false; }
/* initiate a "hard quit" (quit application without sending SAPP_EVENTTYPE_QUIT_REQUSTED) */
SOKOL_API_IMPL void sapp_quit(void) { _sapp.force_quit = true; }
/* call from inside event callback to consume the current event (don't forward to platform) */
SOKOL_API_IMPL void sapp_consume_event(void) { }
/* get the current frame counter (for comparison with sapp_event.frame_count) */
SOKOL_API_IMPL uint64_t sapp_frame_count(void) { return _sapp.frame_count; }
/* write string into clipboard */
SOKOL_API_IMPL void sapp_set_clipboard_string(const char* str) { }
/* read string from clipboard (usually during SAPP_EVENTTYPE_CLIPBOARD_PASTED) */
SOKOL_API_IMPL const char* sapp_get_clipboard_string(void) { return ""; }
/* set the window title (only on desktop platforms) */
SOKOL_API_IMPL void sapp_set_window_title(const char* str) { }
/* set the window icon (only on Windows and Linux) */
SOKOL_API_IMPL void sapp_set_icon(const sapp_icon_desc* icon_desc) { }
/* gets the total number of dropped files (after an SAPP_EVENTTYPE_FILES_DROPPED event) */
SOKOL_API_IMPL int sapp_get_num_dropped_files(void) { return 0; }
/* gets the dropped file paths */
SOKOL_API_IMPL const char* sapp_get_dropped_file_path(int index) { return NULL; }

/* GL: return true when GLES2 fallback is active (to detect fallback from GLES3) */
SOKOL_API_IMPL bool sapp_gles2(void) { return false; }

/* HTML5: enable or disable the hardwired "Leave Site?" dialog box */
SOKOL_API_IMPL void sapp_html5_ask_leave_site(bool ask) { }
/* HTML5: get byte size of a dropped file */
SOKOL_API_IMPL uint32_t sapp_html5_get_dropped_file_size(int index) { return 0; }
/* HTML5: asynchronously load the content of a dropped file */
SOKOL_API_IMPL void sapp_html5_fetch_dropped_file(const sapp_html5_fetch_request* request) { }

/* Metal: get bridged pointer to Metal device object */
SOKOL_API_IMPL const void* sapp_metal_get_device(void) { return NULL; }
/* Metal: get bridged pointer to this frame's renderpass descriptor */
SOKOL_API_IMPL const void* sapp_metal_get_renderpass_descriptor(void) { return NULL; }
/* Metal: get bridged pointer to current drawable */
SOKOL_API_IMPL const void* sapp_metal_get_drawable(void) { return NULL; }
/* macOS: get bridged pointer to macOS NSWindow */
SOKOL_API_IMPL const void* sapp_macos_get_window(void) { return NULL; }
/* iOS: get bridged pointer to iOS UIWindow */
SOKOL_API_IMPL const void* sapp_ios_get_window(void) { return NULL; }

/* D3D11: get pointer to ID3D11Device object */
SOKOL_API_IMPL const void* sapp_d3d11_get_device(void) { return NULL; }
/* D3D11: get pointer to ID3D11DeviceContext object */
SOKOL_API_IMPL const void* sapp_d3d11_get_device_context(void) { return NULL; }
/* D3D11: get pointer to IDXGISwapChain object */
SOKOL_API_IMPL const void* sapp_d3d11_get_swap_chain(void) { return NULL; }
/* D3D11: get pointer to ID3D11RenderTargetView object */
SOKOL_API_IMPL const void* sapp_d3d11_get_render_target_view(void) { return NULL; }
/* D3D11: get pointer to ID3D11DepthStencilView */
SOKOL_API_IMPL const void* sapp_d3d11_get_depth_stencil_view(void) { return NULL; }
/* Win32: get the HWND window handle */
SOKOL_API_IMPL const void* sapp_win32_get_hwnd(void) { return NULL; }

/* WebGPU: get WGPUDevice handle */
SOKOL_API_IMPL const void* sapp_wgpu_get_device(void) { return NULL; }
/* WebGPU: get swapchain's WGPUTextureView handle for rendering */
SOKOL_API_IMPL const void* sapp_wgpu_get_render_view(void) { return NULL; }
/* WebGPU: get swapchain's MSAA-resolve WGPUTextureView (may return null) */
SOKOL_API_IMPL const void* sapp_wgpu_get_resolve_view(void) { return NULL; }
/* WebGPU: get swapchain's WGPUTextureView for the depth-stencil surface */
SOKOL_API_IMPL const void* sapp_wgpu_get_depth_stencil_view(void) { return NULL; }

/* Android: get native activity handle */
SOKOL_API_IMPL const void* sapp_android_get_native_activity(void) { return NULL; }

#endif /* SOKOL_APP_IMPL */