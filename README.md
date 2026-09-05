# A3Texture

High-performance native C++ texture converter for Arma 3's PAA format.
And more

## Features

- **Native C++ Performance** - Uses libsquish for DXT1/DXT5 compression
- **CLI Tool** - Batch conversion from command line
- **GUI Application** - Dear ImGui interface with drag & drop support
- **Format Support** - PNG, TGA, JPG, TIFF input formats
- **Channel Packing** - Builds _nohq/_smdi/_as/_dt from source maps, no Substance packing needed
- **Auto Mipmap Generation** - Generates all mipmap levels automatically
- **DXT Compression** - DXT1 (no alpha) and DXT5 (with alpha)
- **LZO Compression** - Automatic LZO compression for large mipmaps (>128px)

## Download

Prebuilt binaries for Windows and Linux are on the
[releases page](https://github.com/rpgshooter/a3Texture/releases). Nothing to
install: download, and on Linux mark it executable with `chmod +x`.

There are two to choose from.

| Build | What it is | When to take it |
|---|---|---|
| **Latest release** (`v1.0.0`) | Cut by hand, tested | Start here |
| **Nightly** | Rebuilt on every commit | You want a fix or feature that is not in a release yet |

The nightly is marked as a pre-release, so the version GitHub highlights as
"Latest" is always the stable one. The nightly sits directly below it and is
replaced each time, so it is always the current state of `main` — newer, and
correspondingly less tested.

Each release carries four files. Take the GUI unless you are scripting:

| File | Platform | Kind |
|---|---|---|
| `a3texture-gui.exe` | Windows | Windowed app |
| `a3texture-gui` | Linux | Windowed app |
| `a3texture-cli.exe` | Windows | Command line |
| `a3texture-cli` | Linux | Command line |

Every build knows what it is, which is worth quoting in a bug report:

```bash
a3texture-cli --version
```

A release prints its version plainly, `v1.0.0`. A nightly prints the release it
followed, how many commits it is past it, and the commit itself —
`v1.0.0-14-gabc1234` is fourteen commits after v1.0.0.

## Requirements

Only needed to build from source. To just use the tool, see
[Download](#download) above.

- CMake 3.20+
- C++17 compiler (MSVC, GCC, or Clang)
- vcpkg (for dependency management)

## Building from source

### 1. Install vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat  # Windows
```

Set environment variable:
```bash
setx VCPKG_ROOT "C:\path\to\vcpkg"
```

### 2. Build the Project

```bash
cd a3texture
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

vcpkg will automatically install all dependencies:
- libsquish (DXT compression)
- LZO (additional compression)
- stb (image loading)
- libtiff (TIFF loading)
- nlohmann-json (spec files)
- Dear ImGui + GLFW + GLAD (GUI)
- Boost.GIL (image processing)

## Usage

### GUI Application

```bash
./build/Release/a3texture-gui.exe
```

Features:
- Drag & drop files directly into the window
- Select output format (Auto, DXT1, DXT5)
- Batch conversion with progress tracking
- Real-time conversion statistics

### CLI Tool

**Single file:**
```bash
a3texture-cli texture.png texture.paa
a3texture-cli texture.png texture.paa --format DXT5
```

**Batch conversion:**
```bash
a3texture-cli --batch "*.png" --output-dir ./paa/
```

## Technical Details

### PAA Format Implementation

**Compression:**
- DXT1: 8:1 compression (RGB, 1-bit alpha)
- DXT5: 4:1 compression (RGBA, 8-bit alpha)
- LZO: Additional compression for textures >128px

**Mipmap Generation:**
- Bilinear downsampling
- Stops at 4x4 minimum size
- Stored in descending order (largest to smallest)

**Tags:**
- AVGCOLOR (GGATCGVA): Average texture color
- MAXCOLOR (GGATCXAM): Maximum color values
- FLAGTRANSP (GGATGALF): Transparency flag
- OFFSETS (GGATSFFO): Mipmap offset table

## Dependencies

- **libsquish** - DXT compression library
- **LZO** - LZO1X compression
- **stb_image** - PNG/TGA/JPG loading
- **libtiff** - TIFF loading
- **stb_image_write** - PNG writing
- **Boost.GIL** - Image resampling for mipmaps
- **Dear ImGui** - Immediate mode GUI
- **GLFW3** - Window management
- **GLAD** - OpenGL loader

## License

MIT License

## Credits

- Based on [gruppe-adler/grad_aff](https://github.com/gruppe-adler/grad_aff)
- Uses [libsquish](https://github.com/svn2github/libsquish) for DXT compression
- Uses [Dear ImGui](https://github.com/ocornut/imgui) for GUI
