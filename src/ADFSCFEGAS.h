#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace nigga {

	enum class BCFormat { BC4, BC5, BC7 };

	struct DDS_HEADER {
		uint32_t dwSize = 124;
		uint32_t dwFlags = 0x00021007 | 0x00080000; // Add DDSD_LINEARSIZE
		uint32_t dwHeight;
		uint32_t dwWidth;
		uint32_t dwPitchOrLinearSize;
		uint32_t dwDepth = 0;
		uint32_t dwMipMapCount = 0;
		uint32_t dwReserved1[11] = {};
		struct {
			uint32_t dwSize = 32;
			uint32_t dwFlags = 0x4; // DDPF_FOURCC
			uint32_t dwFourCC;
			uint32_t dwRGBBitCount = 0;
			uint32_t dwRBitMask = 0;
			uint32_t dwGBitMask = 0;
			uint32_t dwBBitMask = 0;
			uint32_t dwABitMask = 0;
		} ddspf;
		uint32_t dwCaps = 0x1000; // DDSCAPS_TEXTURE
		uint32_t dwCaps2 = 0;
		uint32_t dwCaps3 = 0;
		uint32_t dwCaps4 = 0;
		uint32_t dwReserved2 = 0;
	};

	struct DDS_HEADER_DX10 {
		uint32_t dxgiFormat;
		uint32_t resourceDimension = 3; // TEXTURE2D
		uint32_t miscFlag = 0;
		uint32_t arraySize = 1;
		uint32_t miscFlags2 = 0;
	};

	VkFormat map_dxgi_to_vk(uint32_t dxgi);
	std::vector<uint8_t> load_dds(const std::string& path, int &width, int &height, int &mipLevels, uint32_t &dxgiFormat);

}
