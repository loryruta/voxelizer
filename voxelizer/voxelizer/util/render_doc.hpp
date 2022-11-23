#pragma once

#include <iostream>
#include <functional>

namespace rgc::renderdoc
{
	void watch(bool capture, std::function<void()> const& f);
}
