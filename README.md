# voxelizer

A tool, and a library, that converts a 3D model to a 3D volume, represented as a Sparse Voxel Octree. Written in modern OpenGL.

This code was originally part of `mdmc`, another project of mine meant to convert a 3D model to a Minecraft schematic. I've decided to modularize this part
as it could be useful for other tasks (and hopefully be useful to other people).

## Usage as a tool

You can generate the octree out of the 3d model using the following command:
```
./voxelizer <model-file> <volume-height> <output-file>
```

You can visualize the output octree by running the following command:
```
./viewer <octree-file> [model-file]
```

Where `model-file` is optional and can be used to compare the original model with the result.

## The octree format

The output file consists of an array of little endian `uint32_t`, in binary format.

The first 8 `uint32_t` correspond to the first octree level. Every value can be either:
- A **parent node**: can be identified because the MSB is set. The remaining bits are the index of the first child node
- A **leaf node**: can be identified because the MSB is not set. The remaining bits are the voxel color. Excluding the MSB: 7 bits for alpha, 8 bits for blue, 8 bits for green, 8 bits for red
- An **empty node**: is 0

## Usage as a library

### Depending on it (CMake users only)

Once you have integrated the code in your project (e.g. either by using git submodules, CMake's FetchContent...), you can add it as a dependency to your project:

```cmake
add_subdirectory(voxelizer)
```

Then you have to include its directories and link the library to your target:

```cmake
target_include_directories(<your-target> PUBLIC "voxelizer/voxelizer")
target_link_libraries(<your-target> PUBLIC voxelizer)
```

### The API _(as for the first version)_

As a first step, you have to create a representation of the 3d model that the voxelizer pipeline is compatible with (i.e. you have to initialize the `voxelizer::scene` object). You can either load it from a file or initialize it from already loaded data (this is the hard part, I'll cover it, probably).

To load it from a model file:
```c++
#include <voxelizer/ai_scene_loader.hpp>
#include <voxelizer/scene.hpp>

voxelizer::assimp_scene_loader scene_loader{};
voxelizer::scene scene{};
char const* my_model_file = /* ... */;

scene_loader.load(scene, my_model_file);
```

Then you can run the voxelization process:
```c++
#include <voxelizer/voxelize.hpp>

voxelizer::voxelize voxelize{};
voxelizer::voxel_list voxel_list{};
uint32_t volume_height = /* ... */;

voxelize(voxel_list, scene, volume_height, scene.m_transformed_min, scene.get_transformed_size());
```

Finally build the octree:
```c++
#include <voxelizer/voxelize.hpp>
#include <voxelizer/octree_builder.hpp>

voxelizer::octree_builder octree_builder{};
glm::uvec3 volume_size = voxelizer::voxelize::calc_proportional_grid(scene.get_transformed_size(), volume_height);
uint32_t max_volume_side = glm::max(glm::max(volume_size.x, volume_size.y), volume_size.z);
uint32_t octree_resolution = (uint32_t) glm::ceil(glm::log2((float) max_volume_side));
size_t octree_bytesize = voxelizer::octree::get_octree_bytesize(octree_resolution);

GLuint octree_buffer{};
glGenBuffers(1, &octree_buffer);
glBindBuffer(GL_SHADER_STORAGE_BUFFER, octree_buffer);
glBufferStorage(GL_SHADER_STORAGE_BUFFER, octree_bytesize, nullptr, NULL);

voxelizer::octree octree{};
octree_builder.build(voxel_list, octree_resolution, octree_buffer, 0, octree); 
```

The built data structure is encapsulated in the `octree` object.

