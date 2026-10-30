# Esoterica Prototype Game Engine

![Esoterica Logo](Docs/img/EE_Logo.png)

[Esoterica Engine](https://www.esotericaengine.com) is an MIT licensed prototype game engine framework. It's intended for use either as a proprietary engine starter pack, a technology demonstrator, an educational tool or an R&D framework. The goal of Esoterica is to provide a solid and robust starting point for folks that are looking to build their own tech or transition off of existing tech. It provides a lot of the boring and time consuming boilerplate systems (reflection, serialization, resource management, math, etc...) as well as a tool framework allowing people to rapidly build their own tool/editors.

We need to be clear that Esoterica is NOT an off the shelf game engine that you can just take and work with. It is intended for technical teams that have the ability to extend and build upon it.

## Important Note: Stability

>Esoterica is a prototype engine that's being developed in our spare time and as such there will likely be bugs and crashes (primarily with the tooling side). It is not intended as a stable production tool. If you encounter any crashes or bugs, please file issues and we'll try to get to them as soon as we can.

## Open Source Not Open Contribution

Esoterica is not a product and so there wont be any formal releases or versions. As such, we are not looking for any feature requests, nor are we looking for large PR contributions. Any feature requests or large PR will be rejected. That said, please feel free to create issues for bugs, clearly broken/incomplete pieces of code or for general questions.

## Level Editing / Tooling / Import Pipeline

The level editing experience is intentionally barebones and rough. In our experience, level editing workflows and tools are incredibly subjective. There is no way to provide tools or workflows that will work for everyone. It will also go against the goal of Esoterica, which is to be a starting point for custom engine development. If a team wants to use Esoterica as a base we want to provide the simplest base for them to build whatever tooling they need for their team and their game.

We are providing some tools in a more polished form like the animation tools and ragdoll editor. This is because we believe that this area is a significantly less subjective field as the problem is better defined and constrained. This makes it easier to build tooling and guide folks down a specific tooling path.

The import pipeline is currently a bit rough from a front-end experience, this is being iterated and played with currently to see if we can find a balance between simplicity and the relatively painful importing other engines provide.

## Documentation

As Esoterica is constantly being iterated and worked upon, it becomes hard to provide any documentation that isnt immediately out of date. Additionally, it becomes even less relevant for people forking it to build their own engine with. The code itself is heavily commented and written to be as readable as possible, this is how we intended to provide documentation moving forward.

We do have highlevel docs for the renderer here: <https://docs.esotericaengine.com>

And we have several presentations that cover the high level decisions regarding the entity model and the animation systems here: <https://www.esotericaengine.com/docs>

## Help Needed: Sample Game

If really you do wish to contribute, we do need some help. We are looking for folks to help us build out a simple sample game that we can include with the project. 

We are looking for the following: 

* *A technical animator* to build out a locomotion and combat anim set.
* *A technical character artist* to build out some test characters, as well as help define the character tech pipelines (deformation, cloth, procedural bones).

More details can be found here: <https://www.esotericaengine.com/contribute>

## What's included

* Basic core of a game engine (serialization, math, string handling, logging, etc...)
* Libclang based c++ reflection and code-generation
* Compiler based resource system with hot-reloading
* Modern bindless DX12 renderer - <https://docs.esotericaengine.com>
* Hybrid Actor/ECS object model - <https://www.youtube.com/watch?v=jjEsB611kxs>
* AAA quality high performance animation system - <https://www.youtube.com/watch?v=R-T3Mk5oDHI> - Used in multiple shipped games.
* Box3D integration and basic ragdoll tooling
* Basic editor infrastructure and tooling using DearImgui

## Screenshots

|                      Resource Pipeline                       |                Basic Editor                 |
|:------------------------------------------------------------:|:-------------------------------------------:|
| ![Esoterica Resource Server](Docs/img/EE_ResourceServer.png) | ![Esoterica Editor](Docs/img/EE_Editor.png) |

|                     Animation Graph                     |                Ragdoll Editor                 |
|:-------------------------------------------------------:|:---------------------------------------------:|
| ![Esoterica Animation Graph](Docs/img/EE_AnimGraph.png) | ![Esoterica Ragdoll](Docs/img/EE_Ragdoll.png) |

## Requirements

* Visual Studio 2026 (18.6.1+)
* Windows 11
* External Dependencies - Available from the releases section

## Building Esoterica

Esoterica uses vanilla msbuild for its build system. There are a set of property sheets that control all the build settings for Esoterica in the "code/property sheets" folder.

1. Get the external dependencies set up: there are two ways to do this:

    * Run the ```DownloadDependencies.bat``` script found in the root folder
    * Manually download the [external dependencies](https://github.com/BobbyAnguelov/Esoterica/releases/download/Dependencies/External.zip) archive and extract into the Esoterica root folder (X:/Esoterica Path/). You should end up with 'X:/Esoterica Path/External/'

1. Open the solution "Esoterica.sln"
1. Manually rebuild the "Esoterica.Scripts.Reflect" project (under the "0. Scripts" solution folder) - this will generate all the required Esoterica reflection code needed for the project to compile.
1. Build the solution

## Applications

Easiest way to get started, is just set the "Esoterica.Applications.Editor" as the startup project and hit run. If you want to run the engine, use the "Esoterica.Applications.Engine" project with the "-map data://path_to_map.map" argument.

* Engine - this is the standalone Esoterica engine client
* Editor - This is the Esoterica editor
* Resource Server - This is a standalone application that will compile and provide resources to the various Esoterica applications
* Reflector - This generates the Esoterica reflection data
* Resource Compiler - This processes resource compilation requests
* Tester - Empty console app used for random testing

## Third-party libraries

Esoterica would not exist without the open-source projects below. Third-party code remains under its own license, not the MIT license of the engine.

### Runtime and data

| Library                     | Used for                                                 | Project                                                                     |
|-----------------------------|----------------------------------------------------------|-----------------------------------------------------------------------------|
| EASTL + EABase              | EA's STL replacement (`TVector`, `TString`, ...)         | [electronicarts/EASTL](https://github.com/electronicarts/EASTL)             |
| rpmalloc                    | Lock-free, thread-caching general purpose allocator      | [mjansson/rpmalloc](https://github.com/mjansson/rpmalloc)                   |
| EnkiTS                      | Task scheduler backing the engine's job system           | [dougbinks/enkiTS](https://github.com/dougbinks/enkiTS)                     |
| moodycamel::ConcurrentQueue | Lock-free MPMC queues used by the threading layer        | [cameron314/concurrentqueue](https://github.com/cameron314/concurrentqueue) |
| rapidhash                   | Hashing (string IDs, resource paths)                     | [Nicoshev/rapidhash](https://github.com/Nicoshev/rapidhash)                 |
| PCG                         | Random number generation (`MathRandom`)                  | [imneme/pcg-cpp](https://github.com/imneme/pcg-cpp)                         |
| mINI                        | INI file reading and writing for settings/config         | [metayeti/mINI](https://github.com/metayeti/mINI)                           |
| MPack                       | MessagePack (de)serialization                            | [ludocode/mpack](https://github.com/ludocode/mpack)                         |
| pugixml                     | XML serialization of reflected types and resources       | [pugixml.org](https://pugixml.org)                                          |
| LZAV                        | Compression of embedded payloads (fonts, generated data) | [avaneev/lzav](https://github.com/avaneev/lzav)                             |
| STB                         | Image load/write/resize and font rasterization helpers   | [nothings/stb](https://github.com/nothings/stb)                             |

### Rendering and simulation

| Library                     | Used for                                                 | Project                                                                                                           |
|-----------------------------|----------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------|
| DirectXShaderCompiler (DXC) | HLSL 2021 to DXIL compilation                            | [microsoft/DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler)                             |
| D3D12MemoryAllocator        | GPU memory suballocation for the DX12 backend            | [GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator) |
| meshoptimizer               | Mesh optimization, simplification and cluster generation | [zeux/meshoptimizer](https://github.com/zeux/meshoptimizer)                                                       |
| Box3D                       | Rigid body physics and ragdoll simulation                | [erincatto/box3d](https://github.com/erincatto/box3d)                                                             |
| AMD AGS                     | AMD driver extension query and device creation           | [GPUOpen-LibrariesAndSDKs/AGS_SDK](https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK)                           |
| Freetype                    | Font rasterization for runtime and editor text           | [freetype.org](https://freetype.org)                                                                              |
| SMAA                        | Subpixel morphological anti-aliasing                     | [iryoku/smaa](https://github.com/iryoku/smaa)                                                                     |
| XeGTAO                      | Ground-truth ambient occlusion (GTAO)                    | [GameTechDev/XeGTAO](https://github.com/GameTechDev/XeGTAO)                                                       |
| Tony McMap Face             | Filmic tonemapping LUT and evaluation                    | [h3r2tic/tony-mc-mapface](https://github.com/h3r2tic/tony-mc-mapface)                                             |
| FidelityFX Parallel Sort    | GPU radix sort used by culling/compaction                | [GPUOpen-Effects/FidelityFX-ParallelSort](https://github.com/GPUOpen-Effects/FidelityFX-ParallelSort)             |
| SHforHLSL                   | Spherical harmonics helpers in HLSL                      | [TheRealMJP/SHforHLSL](https://github.com/TheRealMJP/SHforHLSL)                                                   |

### Editor, tooling and import

| Library         | Used for                                                   | Project                                                           |
|-----------------|------------------------------------------------------------|-------------------------------------------------------------------|
| Dear ImGui      | Editor and debug UI                                        | [ocornut/imgui](https://github.com/ocornut/imgui)                 |
| ImPlot          | Plotting in editor tooling                                 | [epezent/implot](https://github.com/epezent/implot)               |
| LLVM + libclang | C++ parsing for reflection and code generation (Reflector) | [llvm.org](https://llvm.org)                                      |
| ctt             | BCn/ASTC texture compression (ResourceCompiler)            | [cwfitzgerald/ctt](https://github.com/cwfitzgerald/ctt)           |
| ufbx            | FBX import                                                 | [ufbx/ufbx](https://github.com/ufbx/ufbx)                         |
| cgltf           | glTF import                                                | [jkuhlmann/cgltf](https://github.com/jkuhlmann/cgltf)             |
| tinyexr         | OpenEXR image import                                       | [syoyo/tinyexr](https://github.com/syoyo/tinyexr)                 |
| delabella       | Delaunay triangulation for 2D blend spaces                 | [msokalski/delabella](https://github.com/msokalski/delabella)     |
| subprocess.h    | Spawning out-of-process compilers and servers              | [sheredom/subprocess.h](https://github.com/sheredom/subprocess.h) |
| SQLite          | Resource compilation database                              | [sqlite.org](https://sqlite.org)                                  |

### Networking, profiling and debugging

| Library               | Used for                                                   | Project                                                                                       |
|-----------------------|------------------------------------------------------------|-----------------------------------------------------------------------------------------------|
| GameNetworkingSockets | Reliable UDP networking                                    | [ValveSoftware/GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) |
| IXWebSocket           | WebSocket transport for networking and editor/server comms | [machinezone/IXWebSocket](https://github.com/machinezone/IXWebSocket)                         |
| Optick                | CPU profiling instrumentation                              | [bombomby/optick](https://github.com/bombomby/optick)                                         |
| RenderDoc             | Frame capture API integration                              | [renderdoc.org](https://renderdoc.org)                                                        |
| WinPixEventRuntime    | PIX GPU event markers and captures                         | [microsoft/PixEvents](https://github.com/microsoft/PixEvents)                                 |

### Fonts and icons

| Asset                 | Used for                              | Project                                                                      | License    |
|-----------------------|---------------------------------------|------------------------------------------------------------------------------|------------|
| Roboto                | UI font                               | [fonts.google.com/specimen/Roboto](https://fonts.google.com/specimen/Roboto) | Apache 2.0 |
| Material Design Icons | Editor icon font and codepoint header | [pictogrammers.com/library/mdi](https://pictogrammers.com/library/mdi/)      | Apache 2.0 |

The Material Design Icons header (`Code/Base/Imgui/MaterialDesignIcons.h`) was generated with [juliettef/IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders).

### Commercial middleware

These packages are integrated behind preprocessor switches but are not distributed with the engine — you need your own license to enable them.

| Middleware   | Used for                                   | Website                                         |
|--------------|--------------------------------------------|-------------------------------------------------|
| Live++       | Live C++ hot-reload                        | [liveplusplus.tech](https://liveplusplus.tech/) |
| Superluminal | CPU profiler integration                   | [superluminal.eu](https://superluminal.eu/)     |
| NavPower     | Navigation mesh generation and pathfinding | [navpower.com](https://navpower.com/)           |

## Extra Info

If you use the [SmartCommandLineArguments](https://marketplace.visualstudio.com/items?itemName=MBulli.SmartCommandlineArguments) plugin for VS then there are supplied saved arguments to help you start the engine.
