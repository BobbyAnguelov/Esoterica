# Viewer example

This example contains a small model viewer using [Sokol](https://github.com/floooh/sokol).

## Building

To build the example you need to compile `../../ufbx.c`, `external.c`, and `viewer.c` and link
with the necessary platform libraries.

### Linux

```bash
# Install dependencies if missing (Debian/Ubuntu specific here)
sudo apt install -y libgl1-mesa-dev libx11-dev libxi-dev libxcursor-dev

# Compile and link system libraries
clang -lm -ldl -lGL -lX11 -lXi -lXcursor ../../ufbx.c external.c viewer.c -o viewer

# Run the executable
./viewer /path/to/my/model.fbx
```

### Windows

Create a new Visual Studio solution and add `../../ufbx.c`, `external.c`, and `viewer.c` as source files.
Either build and run from the command line giving the desired model as an argument or
set the command line arguments from the project "Debugging" settings.

## Shaders

The compiled shaders are committed to the repository, so unless modifying `.glsl` files you don't need to do anything.
The shaders are compiled using [sokol-shdc](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md),
you can download the prebuilt binaries from [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin).

```bash
# Compile the mesh shader to a header
sokol-shdc --input shaders/mesh.glsl --output shaders/mesh.h --slang glsl330:hlsl5:metal_macos -b
```
