#include "ai_scene_loader.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ------------------------------------------------------------------------------------------------
// voxelizer::mesh
// ------------------------------------------------------------------------------------------------

void load_position_vbo(voxelizer::mesh& mesh, aiMesh const& ai_mesh)
{
	glBindVertexArray(mesh.m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vbos[voxelizer::mesh::attribute::POSITION]);

	glBufferData(GL_ARRAY_BUFFER, size_t(ai_mesh.mNumVertices) * 3 * sizeof(GLfloat), ai_mesh.mVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(voxelizer::mesh::attribute::POSITION);
	glVertexAttribPointer(voxelizer::mesh::attribute::POSITION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind
	glBindVertexArray(0);
}

void load_normal_vbo(voxelizer::mesh& mesh, aiMesh const& ai_mesh)
{
	glBindVertexArray(mesh.m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vbos[voxelizer::mesh::attribute::NORMAL]);

	glEnableVertexAttribArray(voxelizer::mesh::attribute::NORMAL);

	if (ai_mesh.HasNormals())
	{
		glBufferData(GL_ARRAY_BUFFER, size_t(ai_mesh.mNumVertices) * 3 * sizeof(GLfloat), ai_mesh.mNormals, GL_STATIC_DRAW);

		glVertexAttribPointer(voxelizer::mesh::attribute::NORMAL, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}
	else
	{
		GLfloat normals[] = { 0.0f, 0.0f, 0.0f };
		glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat), normals, GL_STATIC_DRAW);

		glVertexAttribPointer(voxelizer::mesh::attribute::NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribDivisor(voxelizer::mesh::attribute::NORMAL, ai_mesh.mNumVertices);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void load_uv_vbo(voxelizer::mesh& mesh, aiMesh const& ai_mesh)
{
	glBindVertexArray(mesh.m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vbos[voxelizer::mesh::attribute::UV]);

	glEnableVertexAttribArray(voxelizer::mesh::attribute::UV);

	if (ai_mesh.HasTextureCoords(0))
	{
		glBufferData(GL_ARRAY_BUFFER, size_t(ai_mesh.mNumVertices) * 3 * sizeof(GLfloat), ai_mesh.mTextureCoords[0], GL_STATIC_DRAW);

		glVertexAttribPointer(voxelizer::mesh::attribute::UV, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}
	else
	{
		GLfloat uv[] = { 0.0f, 0.0f };
		glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(GLfloat), uv, GL_STATIC_DRAW);

		glVertexAttribPointer(voxelizer::mesh::attribute::UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribDivisor(voxelizer::mesh::attribute::UV, ai_mesh.mNumVertices);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void load_color_vbo(voxelizer::mesh& mesh, aiMesh const& ai_mesh)
{
	glBindVertexArray(mesh.m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vbos[voxelizer::mesh::attribute::COLOR]);

	glEnableVertexAttribArray(voxelizer::mesh::attribute::COLOR);

	if (ai_mesh.HasVertexColors(voxelizer::mesh::attribute::COLOR))
	{
		glBufferData(GL_ARRAY_BUFFER, size_t(ai_mesh.mNumVertices) * 4 * sizeof(float), ai_mesh.mColors[0], GL_STATIC_DRAW);

		glVertexAttribPointer(voxelizer::mesh::attribute::COLOR, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	}
	else
	{
		GLfloat color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), color, GL_STATIC_DRAW);

		glVertexAttribPointer(voxelizer::mesh::attribute::COLOR, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribDivisor(voxelizer::mesh::attribute::COLOR, ai_mesh.mNumVertices);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void load_ebo(voxelizer::mesh& mesh, aiMesh const& ai_mesh)
{
	mesh.m_element_count = size_t(ai_mesh.mNumFaces) * 3;

	GLuint* indices = new GLuint[mesh.m_element_count];

	for (size_t i = 0; i < ai_mesh.mNumFaces; i++)
	{
		aiFace& face = ai_mesh.mFaces[i];

		if (face.mNumIndices != 3)
			throw; // std::cerr << "face.mNumIndices != 3" << std::endl;

		indices[i * 3] = face.mIndices[0];
		indices[i * 3 + 1] = face.mIndices[1];
		indices[i * 3 + 2] = face.mIndices[2];
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_element_count * sizeof(GLuint), indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);
}

void calc_transformed_min_max(voxelizer::mesh& mesh, aiMesh const& ai_mesh)
{
	mesh.m_transformed_min = glm::vec3(std::numeric_limits<float>::infinity());
	mesh.m_transformed_max = glm::vec3(-std::numeric_limits<float>::infinity());

	for (size_t i = 0; i < ai_mesh.mNumVertices; i++)
	{
		aiVector3D position = ai_mesh.mVertices[i];
		glm::vec3 transformed_position =  glm::vec3(mesh.m_transform * glm::vec4(position.x, position.y, position.z, 1.0));

		mesh.m_transformed_min = glm::min(mesh.m_transformed_min, transformed_position);
		mesh.m_transformed_max = glm::max(mesh.m_transformed_max, transformed_position);
	}
}

voxelizer::mesh load_mesh(aiMesh const& ai_mesh, aiMatrix4x4 const& ai_transform)
{
	voxelizer::mesh mesh{};

	mesh.m_triangle_count = ai_mesh.mNumFaces;
	mesh.m_transform = glm::transpose(glm::make_mat4(ai_transform[0]));

	load_position_vbo(mesh, ai_mesh);
	load_normal_vbo(mesh, ai_mesh);
	load_color_vbo(mesh, ai_mesh);
	load_uv_vbo(mesh, ai_mesh);

	load_ebo(mesh, ai_mesh);

	calc_transformed_min_max(mesh, ai_mesh);

	return mesh;
}

// ================================================================================================================================
// voxelizer::material
// ================================================================================================================================

void load_material_texture(
	aiScene const& ai_scene,
	GLuint const& texture,
	aiTextureType texture_type,
	std::filesystem::path const& folder,
	aiMaterial const* ai_material
)
{
	glBindTexture(GL_TEXTURE_2D, texture);

	aiString path{};
	if (aiGetMaterialTexture(ai_material, texture_type, 0, &path) == aiReturn_SUCCESS && path.length > 0)
	{
		int width, height, comp;
		stbi_uc* image_data{};

		if (path.C_Str()[0] == '*') // Embedded
		{
			int32_t texture_id = std::atoi(path.C_Str() + 1);
			aiTexture* ai_texture = ai_scene.mTextures[texture_id];

			printf("[assimp_scene_loader] Embedded texture %d (width=%d, height=%d)\n", texture_id, ai_texture->mWidth, ai_texture->mHeight);

			size_t texture_size = ai_texture->mWidth * (ai_texture->mHeight > 0 ? ai_texture->mHeight : 1);
			image_data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(ai_texture->pcData), texture_size, &width, &height, &comp, 0);
		}
		else // External file
		{
			std::filesystem::path texture_path = folder / path.C_Str();

			printf("[assimp_scene_loader] Loading external texture at \"%s\"\n", texture_path.u8string().c_str());

			image_data = stbi_load((folder / path.C_Str()).u8string().c_str(), &width, &height, &comp, STBI_rgb);
		}

		if (image_data == nullptr)
		{
			fprintf(stderr, "[assimp_scene_loader] Failed to load texture \"%s\"\n", path.C_Str());
			fflush(stderr);

			throw std::runtime_error("Failed to load texture");
		}

		if (comp == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
		}
		else if (comp == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		}

		stbi_image_free(image_data);
	}
	else
	{
		GLfloat empty_image[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, empty_image);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void load_material_color(glm::vec4& color, const char* key, unsigned int type, unsigned int index, const aiMaterial* ai_material)
{
	if (aiGetMaterialColor(ai_material, key, type, index, (aiColor4D*) &color[0]) != aiReturn_SUCCESS)
		color = glm::vec4(0);
}

std::shared_ptr<voxelizer::material> load_material(
	aiScene const& ai_scene,
	std::filesystem::path const& folder,
	aiMaterial const* ai_material
)
{
	std::shared_ptr<voxelizer::material> material = std::make_shared<voxelizer::material>();
	voxelizer::material::type type{};
	
	type = voxelizer::material::type::NONE;
	load_material_texture(ai_scene, material->get_texture(type), aiTextureType_NONE, folder, ai_material);
	material->get_color(type) = glm::vec4(1);

	type = voxelizer::material::type::DIFFUSE;
	load_material_texture(ai_scene, material->get_texture(type), aiTextureType_DIFFUSE, folder, ai_material);
	load_material_color(material->get_color(type), AI_MATKEY_COLOR_DIFFUSE, ai_material);

	type = voxelizer::material::type::AMBIENT;
	load_material_texture(ai_scene, material->get_texture(type), aiTextureType_AMBIENT, folder, ai_material);
	load_material_color(material->get_color(type), AI_MATKEY_COLOR_AMBIENT, ai_material);

	type = voxelizer::material::type::SPECULAR;
	load_material_texture(ai_scene, material->get_texture(type), aiTextureType_SPECULAR, folder, ai_material);
	load_material_color(material->get_color(type), AI_MATKEY_COLOR_SPECULAR, ai_material);

	type = voxelizer::material::type::EMISSIVE;
	load_material_texture(ai_scene, material->get_texture(type), aiTextureType_EMISSIVE, folder, ai_material);
	load_material_color(material->get_color(type), AI_MATKEY_COLOR_EMISSIVE, ai_material);

	return material;
}

// ------------------------------------------------------------------------------------------------
// Node
// ------------------------------------------------------------------------------------------------

void load_node(voxelizer::scene& scene, aiScene const& ai_scene, const std::filesystem::path& folder, aiMatrix4x4 ai_transform, const aiNode* ai_node)
{
	ai_transform *= ai_node->mTransformation;

	for (size_t i = 0; i < ai_node->mNumMeshes; i++)
	{
		auto ai_mesh = ai_scene.mMeshes[ai_node->mMeshes[i]];

		voxelizer::mesh mesh = load_mesh(*ai_mesh, ai_transform);
		mesh.m_material = load_material(ai_scene, folder, ai_scene.mMaterials[ai_mesh->mMaterialIndex]);

		scene.m_transformed_min = glm::min(scene.m_transformed_min, mesh.m_transformed_min);
		scene.m_transformed_max = glm::max(scene.m_transformed_max, mesh.m_transformed_max);

		scene.m_meshes.push_back(std::move(mesh));
	}

	for (size_t i = 0; i < ai_node->mNumChildren; i++)
	{
		load_node(scene, ai_scene, folder, ai_transform, ai_node->mChildren[i]);
	}
}

// ------------------------------------------------------------------------------------------------
// Scene
// ------------------------------------------------------------------------------------------------

voxelizer::assimp_scene_loader::assimp_scene_loader()
{
}

void voxelizer::assimp_scene_loader::load(scene& scene, std::filesystem::path const& path)
{
	Assimp::Importer importer;
	aiScene const* ai_scene = importer.ReadFile(path.u8string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

	scene.m_transformed_min = glm::vec3(std::numeric_limits<float>::infinity());
	scene.m_transformed_max = glm::vec3(-std::numeric_limits<float>::infinity());

	load_node(scene, *ai_scene, path.parent_path(), aiMatrix4x4(),  ai_scene->mRootNode);
}
