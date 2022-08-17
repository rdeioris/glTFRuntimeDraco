# glTFRuntimeDraco
KHR_draco_mesh_compression support for glTFRuntime

This is a plugin for glTFRuntime (https://github.com/rdeioris/glTFRuntime) adding support for draco compressed assets.

Windows, Linux x86_64, OSX, iOS and Android are supported (for other platforms you need to manually compile the draco static library available at https://github.com/google/draco and update the glTFRuntimeDraco.Build.cs file accordingly).

To enable it just add it to the Plugins/ directory (it will automatically register itself)

Internally it works by adding two delegates to the GLTF parser (before and after primitives loading):

```cpp
FglTFRuntimeParser::OnPreLoadedPrimitive.AddStatic(FillPrimitiveAdditionalBufferViewsFromDraco);
FglTFRuntimeParser::OnLoadedPrimitive.AddStatic(FillPrimitiveIndicesFromDraco);
```

The first one keeps a cache of the compressed buffers, while the second one 'fixes' the primitive indices.

You can use it as a base for more advanced primitives filters.

## Notes

The Android static library has been build from an Ubuntu 2022 system:

```sh
cmake ../ -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/arm64-android-ndk-libcpp.cmake -DDRACO_ANDROID_NDK_PATH=/usr/lib/android-ndk  -DANDROID_ABI=arm64-v8a
```

The iOS static library has been build from an OSX system:

```sh
cmake ../ -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/arm64-ios.cmake -DCMAKE_C_FLAGS=-fembed-bitcode -DCMAKE_CXX_FLAGS=-fembed-bitcode
```
