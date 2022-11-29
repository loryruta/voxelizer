#include <iostream>
#include <optional>
#include <filesystem>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <voxelizer/util/camera.hpp>
#include <voxelizer/ai_scene_loader.hpp>

#include "scene_renderer.hpp"
#include "octree_tracer.hpp"

bool g_show_octree = true;
bool g_show_scene  = false;

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
		g_show_octree = !g_show_octree;
	}
	else if (key == GLFW_KEY_F2 && action == GLFW_PRESS) // F2
	{
		g_show_scene = !g_show_scene;
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

	// Load octree
	printf("Loading octree at \"%s\"\n", argv[0]);

	GLuint octree_buffer{};
	glGenBuffers(1, &octree_buffer);

	std::ifstream input_file_stream(svo_file, std::ios::binary);

	input_file_stream.seekg(0, std::ios::end);
	size_t octree_buffer_size = input_file_stream.tellg();

	printf("Octree size is %zu bytes ~ %.1f MB\n", octree_buffer_size, octree_buffer_size / (float) (1024 * 1024));

	std::vector<char> octree_buffer_data(octree_buffer_size);

	input_file_stream.seekg(0, std::ios::beg);
	input_file_stream.read(octree_buffer_data.data(), octree_buffer_size);

	printf("Uploading octree on a GPU buffer\n");

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, octree_buffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, octree_buffer_size, octree_buffer_data.data(), NULL);

	// Load scene
	std::optional<voxelizer::scene> scene{};
	if (model_file)
	{
		voxelizer::assimp_scene_loader scene_loader{};

		scene = voxelizer::scene{};
		scene_loader.load(*scene, *model_file);
	}

	// Calc octree position
	glm::vec3 octree_min = glm::vec3(0);
	glm::vec3 octree_max = glm::vec3(1);

	glm::mat4 scene_transform{};
	if (scene)
	{
		scene_transform = glm::identity<glm::mat4>();
		scene_transform = glm::scale(scene_transform, glm::vec3(1) / (octree_max - octree_min));
		scene_transform = glm::translate(scene_transform, -scene->m_transformed_min);
	}

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

		if (dt > 0 && last_cursor_position)
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

		// Render octree
		if (g_show_octree)
		{
			octree_tracer.render(
				glm::uvec2(width, height),
				glm::vec3(0),
				glm::vec3(1),
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
		if (g_show_scene && scene)
		{
			scene_renderer.render(
				camera.projection() * camera.view(),
				scene_transform,
				*scene
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
