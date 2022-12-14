#include <iostream>
#include <optional>
#include <filesystem>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <voxelizer/ai_scene_loader.hpp>
#include <voxelizer/voxelize.hpp>
#include <voxelizer/octree_builder.hpp>

#include "scene_renderer.hpp"
#include "octree_tracer.hpp"
#include "camera.hpp"

bool g_show_scene = false;
bool g_show_octree = true;
bool g_show_debug_geometry = false;
int g_show_scene_projection = -1;

void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam)
{
	if (severity <= GL_DEBUG_SEVERITY_MEDIUM && type == GL_DEBUG_TYPE_ERROR)
	{
		fprintf(stderr, "GL CALLBACK: type = 0x%x, severity = 0x%x, message = %s\n", type, severity, message);
		fflush(stderr);
	}
}

void on_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) // ESCAPE
	{
		int input_mode = glfwGetInputMode(window, GLFW_CURSOR);
		if (input_mode == GLFW_CURSOR_DISABLED) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else if (input_mode == GLFW_CURSOR_NORMAL) glfwSetWindowShouldClose(window, true);
	}
	else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) // ENTER
	{
		int input_mode = glfwGetInputMode(window, GLFW_CURSOR);
		if (input_mode == GLFW_CURSOR_NORMAL) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else if (key == GLFW_KEY_F1 && action == GLFW_PRESS) // F1
	{
		g_show_scene_projection = -1;
		g_show_scene = !g_show_scene;
	}
	else if (key == GLFW_KEY_F2 && action == GLFW_PRESS) // F2
	{
		g_show_scene_projection = -1;
		g_show_octree = !g_show_octree;
	}
	else if (key == GLFW_KEY_F3 && action == GLFW_PRESS) // F3
	{
		g_show_debug_geometry = !g_show_debug_geometry;
	}
	else if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_3) && action == GLFW_PRESS) // 1, 2, 3
	{
		g_show_scene_projection = key - GLFW_KEY_1;
	}
}

void process_freecam_movement_and_rotation(
	GLFWwindow* window,
	float dt,
	tdogl::Camera& camera,
	glm::dvec2 const& cursor_position_delta
)
{
	// Movement
	const float k_movement_speed = 0.1f;

	float speed = k_movement_speed * (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) ? 10 : 1);
	float intensity = speed * dt;

	if (glfwGetKey(window, GLFW_KEY_A)) camera.offsetPosition(-camera.right() * intensity);
	if (glfwGetKey(window, GLFW_KEY_D)) camera.offsetPosition(camera.right() * intensity);
	if (glfwGetKey(window, GLFW_KEY_W)) camera.offsetPosition(camera.forward() * intensity);
	if (glfwGetKey(window, GLFW_KEY_S)) camera.offsetPosition(-camera.forward() * intensity);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) camera.offsetPosition(-glm::vec3(0, 1, 0) * intensity);
	if (glfwGetKey(window, GLFW_KEY_SPACE)) camera.offsetPosition(glm::vec3(0, 1, 0) * intensity);

	// Rotation
	const float k_rotation_speed = 2.0f;

	float dp = (float) cursor_position_delta.y * k_rotation_speed * dt;
	float dy = (float) cursor_position_delta.x * k_rotation_speed * dt;

	camera.offsetOrientation(dp, dy);
}

std::pair<GLuint, size_t> load_octree_from_file(
	char const* filename,
	glm::uvec3& volume_size,
	uint32_t& octree_resolution
)
{
	printf("Loading octree at \"%s\"\n", filename);

	uint32_t version{};
	uint32_t octree_bytesize{};

	std::ifstream input_file_stream(filename, std::ios::binary);

	input_file_stream.read((char*) &version, sizeof(uint32_t));
	input_file_stream.read((char*) &volume_size, sizeof(glm::uvec3));
	input_file_stream.read((char*) &octree_resolution, sizeof(uint32_t));
	input_file_stream.read((char*) &octree_bytesize, sizeof(uint32_t));

	std::vector<char> octree_buffer_data(octree_bytesize);
	input_file_stream.read(octree_buffer_data.data(), octree_bytesize);

	printf("Octree loaded - Version: %d, Volume size: (%d, %d, %d), Resolution: %d, Bytesize: %d (~%.1f MB)\n",
		version,
		volume_size.x, volume_size.y, volume_size.z,
		octree_resolution,
		octree_bytesize,
		octree_bytesize / (float) (1024 * 1024)
	);

	printf("Uploading octree on a GPU buffer\n");

	GLuint octree_buffer{};
	glGenBuffers(1, &octree_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, octree_buffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, octree_bytesize, octree_buffer_data.data(), NULL);

	return {
		octree_buffer,
		octree_bytesize,
	};
}

std::pair<GLuint, size_t> build_octree_from_scene(voxelizer::scene const& scene, uint32_t volume_height)
{
	voxelizer::voxelize voxelize{};
	voxelizer::voxel_list voxel_list{};

	glm::vec3 area_size = scene.get_transformed_size();
	glm::uvec3 volume_size = voxelizer::voxelize::calc_proportional_grid(area_size, volume_height);
	uint32_t max_volume_side = glm::max(glm::max(volume_size.x, volume_size.y), volume_size.z);

	printf("Voxelizing scene - area size: (%.2f, %.2f, %.2f) volume: (%d, %d, %d), max side: %d\n",
		area_size.x,
		area_size.y,
		area_size.z,
		volume_size.x,
		volume_size.y,
		volume_size.z,
		max_volume_side
	);

	voxelize(voxel_list, scene, volume_height, scene.m_transformed_min, scene.get_transformed_size());

	voxelizer::octree_builder octree_builder{};
	GLuint octree_buffer{};
	uint32_t octree_resolution = (uint32_t)glm::ceil(glm::log2((float) max_volume_side));
	size_t octree_bytesize = voxelizer::octree::get_octree_bytesize(octree_resolution);

	glGenBuffers(1, &octree_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, octree_buffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, octree_bytesize, NULL, NULL);

	voxelizer::octree octree{};
	octree_builder.build(voxel_list, octree_resolution, octree_buffer, 0, octree);

	return {
		octree_buffer,
		octree_bytesize,
	};
}

int main(int argc, char** argv)
{
	argc--;
	argv++;

	if (argc < 1)
	{
		printf("Invalid command syntax: ./viewer <svo-file> [model-file]\n");
		return 1;
	}

	// SVO file
	std::filesystem::path svo_file = argv[0];
	if (!std::filesystem::exists(svo_file))
	{
		printf("Invalid file: %s\n", svo_file.u8string().c_str());
		return 2;
	}

	// Model file
	std::optional<std::filesystem::path> model_file{};
	if (argc >= 2)
	{
		model_file = argv[1];
		if (!std::filesystem::exists(*model_file))
		{
			printf("Invalid file: %s\n", model_file->u8string().c_str());
			return 3;
		}
	}

	if (glfwInit() != GLFW_TRUE)
	{
		std::cerr << "GLFW failed to initialize" << std::endl;
		return 3;
	}

	GLFWwindow* window = glfwCreateWindow(720, 720, "viewer", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "GLAD failed to initialize." << std::endl;
		return 4;
	}

	glfwShowWindow(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetKeyCallback(window, on_key);

	printf("Initializing OpenGL context\n");

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(message_callback, nullptr);

	tdogl::Camera camera{};
	voxelizer::octree_tracer octree_tracer{};
	voxelizer::scene_renderer scene_renderer{};

	// Load scene
	voxelizer::scene scene{};
	voxelizer::assimp_scene_loader scene_loader{};

	printf("Loading scene: \"%s\"\n", model_file->u8string().c_str());

	scene_loader.load(scene, *model_file);

	printf("Scene loaded\n");

	// Load octree
	glm::uvec3 volume_size{};
	uint32_t octree_resolution{};
	auto [octree_buffer, octree_buffer_size] = load_octree_from_file(argv[0], volume_size, octree_resolution);

	uint32_t volume_max_side = glm::max(volume_size.x, glm::max(volume_size.y, volume_size.z));

	// Calc octree position
	glm::vec3 volume_position = glm::vec3(0);

	glm::vec3 octree_min = volume_position;
	glm::vec3 octree_max = glm::vec3(glm::exp2((float) octree_resolution) / float(volume_max_side)); // Constraint max side of the volume between (0, 1)

	glm::vec3 scene_size = scene.get_transformed_size();
	uint32_t scene_max_side = glm::max(scene_size.x, glm::max(scene_size.y, scene_size.z));

	glm::mat4 scene_transform{}; // Constraint the max side of the scene between (0, 1)
	scene_transform = glm::identity<glm::mat4>();
	scene_transform = glm::scale(scene_transform, glm::vec3(1) / glm::vec3(scene_max_side));
	scene_transform = glm::translate(scene_transform, -scene.m_transformed_min + volume_position);

	voxelizer::voxelize voxelize{};
	glm::mat4 scene_normalization_matrix{};
	glm::mat4 scene_projection_matrices[3]{};
	scene_normalization_matrix =
		voxelize.create_scene_normalization_matrix(scene.m_transformed_min, scene.m_transformed_max - scene.m_transformed_min);
	voxelize.create_projection_matrices(scene_projection_matrices);

	// Debug renderer
	voxelizer::debug_renderer debug_renderer{};

	// Main loop
	printf("Starting loop\n");

	std::optional<double> last_time{};
	std::optional<glm::dvec2> last_cursor_position{};

	while (!glfwWindowShouldClose(window))
	{
		// Update
		glfwPollEvents();

		float dt = 0.0f;
		if (last_time.has_value())
		{
			dt = static_cast<float>(glfwGetTime() - *last_time);
 		}

		last_time = glfwGetTime();

		glm::dvec2 cursor_position{};
		glfwGetCursorPos(window, &cursor_position.x, &cursor_position.y);

		if (
			dt > 0 &&
			last_cursor_position &&
			glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED
		)
		{
			process_freecam_movement_and_rotation(
				window,
				dt,
				camera,
				cursor_position - *last_cursor_position
			);
		}

		last_cursor_position = cursor_position;

		/*
		printf("Camera position=(%.1f, %.1f, %.1f) rotation=(%.1f, %.1f)\n",
			camera.position().x, camera.position().y, camera.position().z,
			camera.horizontalAngle(), camera.verticalAngle()
		);*/

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.7f, 0.7f, 0.7f, 0);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		camera.setViewportAspectRatio(width / (float) height);

		// Render debug geometry
		if (g_show_debug_geometry)
		{
			debug_renderer.cube(glm::vec3(0), glm::vec3(1), glm::vec3(1, 0, 0));
			debug_renderer.flush(camera.view(), camera.projection());
		}

		// Render octree
		if (g_show_octree)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			octree_tracer.render(
				glm::uvec2(width, height),
				octree_min,
				octree_max,
				camera.projection(),
				camera.view(),
				camera.position(),
				octree_buffer,
				0,
				octree_buffer_size,
				0
			);
		}

		// Render scene
		if (g_show_scene)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			scene_renderer.render(
				camera.projection(),
				camera.view(),
				scene_transform,
				scene
			);
		}

		// Render scene projections
		if (g_show_scene_projection >= 0)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			scene_renderer.render(
				scene_projection_matrices[g_show_scene_projection],
				glm::mat4(1),
				scene_normalization_matrix,
				scene
			);
		}

		// Swap
		glfwSwapBuffers(window);
	}
	
	printf("Destroying\n");

	glDeleteBuffers(1, &octree_buffer);

	glfwDestroyWindow(window);
	glfwTerminate();
}

