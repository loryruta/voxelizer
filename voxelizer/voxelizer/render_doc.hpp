#pragma once

#include <iostream>
#include <functional>

namespace voxelizer::renderdoc
{
	void watch(bool capture, std::function<void()> const& f);
}
