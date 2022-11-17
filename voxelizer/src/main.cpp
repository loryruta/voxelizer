#include <cstdio>
#include <filesystem>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>

#include "ai_scene_loader.hpp"
#include "scene.hpp"
#include "voxelize.hpp"
#include "octree.hpp"

void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam)
{
	if (severity <= GL_DEBUG_SEVERITY_MEDIUM && type == GL_DEBUG_TYPE_ERROR)
	{
		fprintf(stderr, "GL CALLBACK: type = 0x%x, severity = 0x%x, message = %s\n", type, severity, message);
		fflush(stderr);
	}
}

void run_voxelizer(
	std::filesystem::path const& input_file_path,
	uint32_t volume_height,
	std::filesystem::path const& output_file_path
)
{
	voxelizer::assimp_scene_loader scene_loader{};
	voxelizer::scene scene{};

	printf("Loading scene \"%s\"\n", input_file_path.u8string().c_str());

	scene_loader.load(scene, input_file_path);

	voxelizer::voxelize voxelize{};
	voxelizer::VoxelList voxel_list{};

	glm::vec3 area_min = scene.m_transformed_min;
	glm::vec3 area_size = scene.m_transformed_max - scene.m_transformed_min;
	glm::uvec3 volume_size = voxelizer::voxelize::calc_proportional_grid(area_size, volume_height);

	printf("Voxelizing scene to a (%d, %d, %d) volume\n", volume_size.x, volume_size.y, volume_size.z);

	voxelize(voxel_list, scene, volume_height, area_min, area_size);

	printf("Generated a voxel list of %zu elements\n", voxel_list.m_size);

	voxelizer::octree_builder octree_builder{};
	GLuint octree_buffer{};
	uint32_t octree_resolution = glm::log2(glm::max(glm::max(volume_size.x, volume_size.y), volume_size.z));
	size_t octree_bytesize = voxelizer::octree::get_octree_bytesize(octree_resolution);
	voxelizer::octree octree{};

	printf("Allocating an octree of resolution %d (%.1f MB)\n", octree_resolution, ((float) octree_bytesize / (1024 * 1024)));

	glGenBuffers(1, &octree_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, octree_buffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, octree_bytesize, nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

	printf("Building the octree\n");

	octree_builder.build(voxel_list, octree_resolution, octree_buffer, 0, octree);

	printf("Writing to the output file \"%s\"\n", output_file_path.u8string().c_str());

	GLuint const* octree_buffer_ptr =
		(GLuint const*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, octree_bytesize, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

	std::ofstream output_file_stream(output_file_path);
	output_file_stream.write(reinterpret_cast<char const*>(octree_buffer_ptr), octree_bytesize);
}

int main(int argc, char* argv[])
{
	argc--;
	argv++;

	if (argc < 3)
	{
		printf("Invalid command syntax: ./voxelizer <input-file> <volume-height> <output-file>\n");
		return 1;
	}

	// Input file
	std::filesystem::path input_file_path = argv[0];
	if (!std::filesystem::exists(input_file_path) || !std::filesystem::is_regular_file(input_file_path))
	{
		printf("Input file not found\n");
		return 2;
	}

	// Volume height
	int32_t volume_height = std::stoi(argv[1]);
	if (volume_height <= 0 || volume_height > 256)
	{
		printf("Volume height out of bounds: [1, 256]\n");
		return 3;
	}

	// Output file
	std::filesystem::path output_file_path = argv[2];

	printf("Initializing OpenGL context\n");

	if (glfwInit() != GLFW_TRUE)
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		fflush(stderr);

		return 1;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(23, 23 /* 23 is my favourite number :) Doesn't matter as it's a fake window */, "voxelizer", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		fflush(stderr);
		return 2;
	}

	//glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(message_callback, nullptr);

	run_voxelizer(input_file_path, volume_height, output_file_path);

	printf("Bye bye\n");

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
