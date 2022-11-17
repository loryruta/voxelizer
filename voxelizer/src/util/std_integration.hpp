#pragma once

#include <glm/glm.hpp>

namespace std
{
	template<>
	struct hash<glm::ivec3>
	{
		size_t operator()(glm::ivec3 const& v) const
		{
			return std::hash<int>()(v.x) ^ std::hash<int>()(v.y) ^ std::hash<int>()(v.z);
		}

		bool operator()(glm::ivec3 const& a, glm::ivec3 const& b) const
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
	};

	template<>
	struct hash<glm::uvec3>
	{
		size_t operator()(glm::uvec3 const& v) const
		{
			return std::hash<int>()(v.x) ^ std::hash<int>()(v.y) ^ std::hash<int>()(v.z);
		}

		bool operator()(glm::uvec3 const& a, glm::uvec3 const& b) const
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
	};
}
