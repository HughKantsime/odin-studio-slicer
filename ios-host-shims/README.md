# ios-host-shims

Source files that must be compiled into the host app binary to make
`ODINSlicer.xcframework` loadable on iOS device runtimes older than the SDK
the xcframework was built against.

Under AGPL-3.0 §5, these count as **corresponding source** for the combined
binary because the AGPL-licensed `libslic3r` cannot be loaded at runtime
without them. They're maintained here so a single public repo offers the
complete corresponding source.

The host app (`HughKantsime/odin-studio`) copies these files into
`App/Slicer/` at build time; there is no fork. Any patch accepted upstream
here propagates to the app's next release.

## Files

| File | Why it exists |
|---|---|
| `libcxx_hash_shim.cpp` | Defines `std::__1::__hash_memory(void const*, size_t)`. SDK 26.4 libc++ calls this out-of-line; iOS 26.2 (and earlier) don't export it. |
| `tbbmalloc_shim.c` | Weak shims for TBB's `scalable_*` allocator interface forwarding to libc malloc. TBB's own tbbmalloc library ships as a dylib on desktop; we build TBB static and don't link tbbmalloc. |

## License

All files here are under **AGPL-3.0-or-later** to match libslic3r. See
`../LICENSE.txt` for the full text.
