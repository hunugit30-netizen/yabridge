## Status
Added comprehensive ARA (Audio Random Access) support for VST3 plugins. Implemented the core serialization infrastructure and proxy classes for all major ARA interfaces.

## Last modified files
- src/common/serialization/ara.h — Complete ARA serialization structures (control requests, callbacks, properties, responses)
- src/plugin/bridges/ara.h — Plugin-side ARA bridge with DocumentController, EditorRenderer, EditorView proxies
- src/plugin/bridges/ara.cpp — Plugin-side proxy implementations
- src/plugin/bridges/ara-impls/plugin-factory-proxy.cpp — Plugin factory proxy implementation
- src/wine-host/bridges/ara.h — Wine host ARA bridge with DocumentController proxy management
- src/wine-host/bridges/ara.cpp — Wine host ARA message handling
- src/wine-host/bridges/ara-factory-proxy.h/cpp — ARA factory proxy for serialization
- src/wine-host/bridges/ara-document-controller-proxy.h/cpp — DocumentController proxy with full ARA SDK integration
- src/wine-host/bridges/ara-editor-renderer-proxy.h/cpp — EditorRenderer proxy
- src/wine-host/bridges/ara-editor-view-proxy.h/cpp — EditorView proxy
- src/wine-host/bridges/ara-playback-renderer-proxy.h/cpp — PlaybackRenderer proxy
- src/wine-host/meson.build — Added new ARA proxy source files

## Architecture decisions
- ARA is implemented as a VST3 extension (ARA plugins are VST3 plugins with additional interfaces)
- Serialization uses bitsery with std::variant for message types (same pattern as VST3/CLAP)
- Two-process architecture: Linux plugin side ↔ Wine host side via Unix domain sockets
- Plugin side forwards ARA calls to Wine host; Wine host calls into actual ARA SDK
- ARA factory discovered via VST3 IPlugInEntryPoint::getARAFactory()
- DocumentController created per-document, other objects created on-demand

## Next steps
1. Full meson build with cross-compiler (requires mingw-w64 and Wine development headers)
2. Test with ARA-capable plugins like Melodyne
3. Implement EditorRenderer/EditorView on plugin side
4. Add PlaybackRenderer/EditorRenderer/EditorView message handling to Wine host run() loop
5. Implement content reader/writer for audio access and archive operations

## Open issues
- Build system requires cross-compiler (gcc-mingw-w64) and Wine development headers
- Plugin-side EditorRenderer/EditorView proxies not yet implemented
- Content reading (audio samples, note events) not yet implemented
- Archive read/write host refs not fully wired up