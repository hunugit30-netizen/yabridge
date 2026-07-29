# Memory

## Architecture

### Build System (Meson)
- yabridge uses Meson as its build system with Wine cross-compilation
- The build creates:
  - Native Linux shared libraries: `libyabridge-{vst2,vst3,clap}.so` (plugin entry points)
  - Wine host executables: `yabridge-host.exe.so` (64-bit) and `yabridge-host-32.exe.so` (32-bit bitbridge)
  - Chainloader libraries for each plugin format

### VST3 SDK Integration
- VST3 SDK is fetched as a Meson subproject from a forked repo (robbert-vdh/vst3sdk)
- The fork includes a custom `meson.build` that exposes source files and include directories
- The main build (`src/common/vst3/meson.build`) assembles static libraries for:
  - Native (Linux) VST3 plugin SDK
  - Wine 64-bit VST3 hosting SDK
  - Wine 32-bit VST3 hosting SDK (when bitbridge enabled)

### ARA SDK Integration (NEW)
- ARA SDK 2.3.0 is fetched as a Meson subproject from the official Celemony/ARA_SDK repo
- The ARA_SDK repo includes ARA_API and ARA_Library as git submodules
- A custom `meson.build` in `subprojects/ara/` builds:
  - ARA_API (header-only)
  - ARA_Host_Library (for DAW/host side)
  - ARA_Plugin_Library (for plugin side)
- Built for native, Wine 64-bit, and Wine 32-bit targets
- Only enabled when both `-Dvst3=true` and `-Dara=true` (default)

## Files

### Build Configuration
- `meson.build` - Main build configuration, defines options, dependencies, and targets
- `meson_options.txt` - Build options (bitbridge, clap, vst3, winedbg, **ara**)
- `cross-wine.conf` - Wine cross-compilation configuration
- `subprojects/*.wrap` - Subproject definitions for dependencies

### Subprojects
- `subprojects/vst3.wrap` - VST3 SDK (forked)
- `subprojects/ara.wrap` - ARA SDK (official Celemony repo)
- `subprojects/ara/meson.build` - ARA SDK Meson build definition
- `subprojects/clap.wrap`, `subprojects/asio.wrap`, etc. - Other dependencies

### VST3 Integration
- `src/common/vst3/meson.build` - VST3 SDK static library assembly + **ARA integration**

### Plugin Build
- `src/plugin/meson.build` - VST2/VST3/CLAP plugin shared library sources and dependencies
- `src/plugin/bridges/vst3.cpp` - VST3 plugin bridge implementation

### Wine Host Build
- `src/wine-host/meson.build` - Wine host executable sources and dependencies
- `src/wine-host/bridges/vst3.cpp` - VST3 wine host bridge implementation

## Patterns

### Adding a New Subproject Dependency
1. Create `.wrap` file in `subprojects/`
2. Create `meson.build` in `subprojects/<name>/` that exposes sources/includes via `set_variable()`
3. In consumer meson.build (e.g., `src/common/vst3/meson.build`):
   - Call `subproject('<name>')`
   - Get variables via `get_variable()`
   - Build static libraries for native/Wine targets
   - Create `declare_dependency()` objects
   - Expose via `set_variable()` for parent

### Conditional Dependencies
- Options defined in `meson_options.txt`
- Retrieved in `meson.build` via `get_option('<name>')`
- Used in `if with_<option>` blocks
- Compiler defines added via `compiler_options += '-DWITH_<OPTION>'`

### Wine Cross-Compilation
- Native builds: `native: true`
- Wine 64-bit: `native: false` + `wine_64bit_compiler_options`
- Wine 32-bit: `native: false` + `wine_32bit_compiler_options` (requires `with_bitbridge`)

## Open Tasks

### ARA SDK Integration
- [x] Add ARA SDK as Meson subproject (ara.wrap + ara/meson.build)
- [x] Add `ara` build option to meson_options.txt
- [x] Integrate ARA into VST3 build (src/common/vst3/meson.build)
- [x] Add ARA dependencies to VST3 plugin (src/plugin/meson.build)
- [x] Add ARA dependencies to Wine host (src/wine-host/meson.build)
- [ ] Implement ARA VST3 interface handling in plugin bridge (src/plugin/bridges/vst3-impls/)
- [ ] Implement ARA VST3 interface handling in wine host bridge (src/wine-host/bridges/vst3-impls/)
- [ ] Test build with `-Dara=true` in CI

## Gotchas

### Meson Subproject Limitations
- Cannot mix native and cross-compiled dependencies from the same CMake subproject
- Workaround: Subproject meson.build only exposes source files; parent builds static libraries for each target
- This pattern is used for both VST3 and ARA SDKs

### Wine GCC Compatibility
- VST3 SDK uses `Windows.h` (capital W) but file is `windows.h` on Linux
- VST3 SDK has attributes that cause warnings with Wine GCC
- A patch script (`tools/patch-vst3-sdk.sh`) fixes these issues
- ARA SDK may need similar patching if issues arise

### Symbol Visibility
- Linux builds use `-fvisibility=hidden` by default
- VST3/ARA SDK static libraries built with `warning_level=0` to suppress SDK warnings
- Plugin shared libraries don't use LTO (breaks Bitwig), chainloaders do

### ARA SDK Specifics
- ARA_API is header-only (C API)
- ARA_Library has C++ implementation with C files (ARADebug.c)
- Requires C++11 and C11 standards
- ARA_Library headers include ARA_API headers via `#include <ARA_API/...>`
- Need parent directory of ARA_Library in include path for this to work

## Last Session
- Implemented ARA SDK 2.3.0 integration as Meson subproject
- Created `subprojects/ara.wrap` pointing to Celemony/ARA_SDK releases/2.3.0
- Created `subprojects/ara/meson.build` building ARA_API, ARA_Host_Library, ARA_Plugin_Library
- Added `ara` option to meson_options.txt (default: true)
- Integrated ARA into src/common/vst3/meson.build (native + Wine 64/32-bit)
- Added ARA dependencies to VST3 plugin (ara_plugin_native_dep)
- Added ARA dependencies to Wine host (ara_host_wine_64bit_dep, ara_host_wine_32bit_dep)
- ARA only enabled when both VST3 and ARA options are true