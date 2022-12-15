#include "render_doc.hpp"

#include <cstdio>
#include <iostream>

#include <renderdoc_app.h>
#define NOMINMAX
#include <windows.h>

RENDERDOC_API_1_1_2* g_handle = nullptr;

void renderdoc_init()
{
	if (HMODULE module = GetModuleHandleA("renderdoc.dll"))
	{
		auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI) GetProcAddress(module, "RENDERDOC_GetAPI");
		int result = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**) &g_handle);
		if (result != 1)
		{
			std::cerr << "RenderDoc failed to initialize, result: " << result << std::endl;
			return;
		}

		printf("Renderdoc module initialized\n");
	}
}

void voxelizer::renderdoc::watch(bool capture, std::function<void()> const& f)
{
	if (g_handle == nullptr) {
		renderdoc_init();
	}

	if (g_handle && capture)
	{
		g_handle->StartFrameCapture(NULL, NULL);
	}

	f();

	if (g_handle && capture)
	{
		g_handle->EndFrameCapture(NULL, NULL);
	}
}
