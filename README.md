# glTFRuntimeDraco
KHR_draco_mesh_compression support for glTFRuntime

This is a plugin for glTFRuntime (https://github.com/rdeioris/glTFRuntime) adding support for draco compressed assets.
Currently only Windows 64bit is supported (for other platforms you need to manually compile the draco static library available at https://github.com/google/draco).

To enable it just add it to the Plugins/ directory (it will automatically register itself)

Internally it works by adding two delegates to the GLTF parser (before and after primitives loading):

```cpp
FglTFRuntimeParser::OnPreLoadedPrimitive.AddStatic(FillPrimitiveAdditionalBufferViewsFromDraco);
FglTFRuntimeParser::OnLoadedPrimitive.AddStatic(FillPrimitiveIndicesFromDraco);
```

The first one keeps a cache of the compressed buffers, while the second one 'fixes' the primitive indices.

You can use it as a base for more advanced primitives filters.
