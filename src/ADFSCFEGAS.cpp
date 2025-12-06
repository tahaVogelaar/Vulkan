#include "ADFSCFEGAS.h"

namespace nigga {

	// Map BC format to DXGI format
	VkFormat map_dxgi_to_vk(uint32_t dxgi) {
		switch(dxgi) {
			case 98: return VK_FORMAT_BC7_UNORM_BLOCK; // BC7
			case 83: return VK_FORMAT_BC5_UNORM_BLOCK; // BC5
			case 80: return VK_FORMAT_BC4_UNORM_BLOCK; // BC4
			default: throw std::runtime_error("Unsupported DXGI format");
		}
	}



	std::vector<uint8_t> load_dds(const std::string& path, int &width, int &height, int &mipLevels, uint32_t &dxgiFormat) {
		std::ifstream f(path, std::ios::binary);
		if (!f) throw std::runtime_error("Failed to open DDS: " + path);

		uint32_t magic = 0;
		f.read(reinterpret_cast<char*>(&magic), sizeof(magic));
		if (magic != 0x20534444) // 'DDS '
			throw std::runtime_error("Not a DDS file: " + path);

		DDS_HEADER header;
		f.read(reinterpret_cast<char*>(&header), sizeof(header));

		width = static_cast<int>(header.dwWidth);
		height = static_cast<int>(header.dwHeight);
		mipLevels = header.dwMipMapCount ? static_cast<int>(header.dwMipMapCount) : 1;

		uint32_t fourCC = header.ddspf.dwFourCC;
		bool isDX10 = (fourCC == 0x30315844); // 'DX10'

		if (isDX10) {
			DDS_HEADER_DX10 dx10;
			f.read(reinterpret_cast<char*>(&dx10), sizeof(dx10));
			dxgiFormat = dx10.dxgiFormat;
		} else {
			// Map legacy FourCC to DXGI fallback values (common cases)
			switch (fourCC) {
				case 0x31545844: dxgiFormat = 71;  break; // 'DXT1' -> DXGI_FORMAT_BC1_UNORM (71)
				case 0x33545844: dxgiFormat = 77;  break; // 'DXT3' -> DXGI_FORMAT_BC2_UNORM (77)
				case 0x35545844: dxgiFormat = 77;  break; // 'DXT5' uses BC3 (77)
				case 0x32495441: dxgiFormat = 83;  break; // 'ATI2' -> BC5 (83)
				default:
					// If not recognized, set dxgiFormat to fourCC so caller can decide
					dxgiFormat = fourCC;
					break;
			}
		}

		// Read full remaining bytes (all mip levels compressed payload)
		std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

		if (data.empty())
			throw std::runtime_error("DDS has no image data: " + path);

		return data;
	}
}