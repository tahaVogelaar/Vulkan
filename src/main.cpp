#include "first_app.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
/*
#define RGBCX_IMPLEMENTATION
#include "bc7enc_rdo/rgbcx.h"
#include "bc7enc_rdo/utils.h"
#define BC7ENC_IMPLEMENTATION
#include "bc7enc_rdo/bc7enc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace skibidi {
    namespace fs = std::filesystem;

    enum class BCFormat {
        BC4,
        BC5,
        BC7
    };

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

    // Map BC format to DXGI format
    uint32_t dxgi_format_for_bc(BCFormat fmt) {
        switch (fmt) {
            case BCFormat::BC4: return 80;  // DXGI_FORMAT_BC4_UNORM
            case BCFormat::BC5: return 83;  // DXGI_FORMAT_BC5_UNORM
            case BCFormat::BC7: return 98;  // DXGI_FORMAT_BC7_UNORM
            default: throw std::runtime_error("Unsupported BC format");
        }
    }

    // Write DDS with DX10 header
    void write_dds_bc(const std::string& path, int width, int height, const std::vector<uint8_t>& data, BCFormat format) {
        fs::create_directories(fs::path(path).parent_path());

        FILE* f = fopen(path.c_str(), "wb");
        if (!f) throw std::runtime_error("Failed to open output: " + path);

        const uint32_t DDS_MAGIC = 0x20534444; // 'DDS '
        fwrite(&DDS_MAGIC, sizeof(DDS_MAGIC), 1, f);

        DDS_HEADER header;
        header.dwHeight = height;
        header.dwWidth = width;
        header.dwPitchOrLinearSize = (uint32_t)data.size();
        header.ddspf.dwFourCC = 0x20374342; // Default 'BC7 '

        if (format == BCFormat::BC4) header.ddspf.dwFourCC = 0x344342; // 'BC4'
        if (format == BCFormat::BC5) header.ddspf.dwFourCC = 0x355342; // 'BC5'

        fwrite(&header, sizeof(header), 1, f);

        DDS_HEADER_DX10 dx10;
        dx10.dxgiFormat = dxgi_format_for_bc(format);
        fwrite(&dx10, sizeof(dx10), 1, f);

        if (fwrite(data.data(), data.size(), 1, f) != 1) {
            fclose(f);
            throw std::runtime_error("Failed to write DDS data: " + path);
        }

        fclose(f);
    }


    // Compress a 1-channel image into BC4
    void compress_bc4(const uint8_t* src, int w, int h, const std::string& outPath) {
        const int blocksX = w / 4;
        const int blocksY = h / 4;
        std::vector<uint8_t> out(blocksX * blocksY * 8);

        size_t o = 0;
        uint8_t block[16];
        for (int by = 0; by < blocksY; ++by) {
            for (int bx = 0; bx < blocksX; ++bx) {
                int idx = 0;
                for (int y = 0; y < 4; ++y)
                    for (int x = 0; x < 4; ++x)
                        block[idx++] = src[(by*4 + y)*w + bx*4 + x];
                rgbcx::encode_bc4(out.data() + o, block, false);
                o += 8;
            }
        }
        write_dds_bc(outPath, w, h, out, BCFormat::BC4);

    }

    // Compress a 2-channel image into BC5
    void compress_bc5(const uint8_t* src, int w, int h, const std::string& outPath) {
        const int blocksX = w / 4;
        const int blocksY = h / 4;
        std::vector<uint8_t> out(blocksX * blocksY * 16);

        size_t o = 0;
        uint8_t block[32];
        for (int by = 0; by < blocksY; ++by) {
            for (int bx = 0; bx < blocksX; ++bx) {
                int idx = 0;
                for (int y = 0; y < 4; ++y)
                    for (int x = 0; x < 4; ++x) {
                        int px = bx*4 + x;
                        int py = by*4 + y;
                        block[idx++] = src[(py*w + px)*2 + 0]; // R
                        block[idx++] = src[(py*w + px)*2 + 1]; // G
                    }
                rgbcx::encode_bc5(out.data() + o, block, false);
                o += 16;
            }
        }
        write_dds_bc(outPath, w, h, out, BCFormat::BC5);
    }

    // Compress a full RGBA image into BC7
    void compress_bc7(const uint8_t* src, int w, int h, const std::string& outPath) {
        const int blocksX = w / 4;
        const int blocksY = h / 4;
        std::vector<uint8_t> out(blocksX * blocksY * 16);

        bc7enc_compress_block_params params;
        bc7enc_compress_block_params_init(&params);

        uint8_t block[16*4];
        size_t o = 0;

        bc7enc_compress_block_init();
        for (int by = 0; by < h; by += 4) {
            for (int bx = 0; bx < w; bx += 4) {
                for (int y = 0; y < 4; ++y) {
                    for (int x = 0; x < 4; ++x) {
                        int px = bx + x;
                        int py = by + y;
                        int idx = (y*4 + x)*4;
                        const uint8_t* p = &src[(py*w + px)*4];
                        block[idx+0] = p[0]; // R
                        block[idx+1] = p[1]; // G
                        block[idx+2] = p[2]; // B
                        block[idx+3] = p[3]; // A
                    }
                }
                bc7enc_compress_block(&out[o], block, &params);
                o += 16;
            }
        }
        write_dds_bc(outPath, w, h, out, BCFormat::BC7);
    }

    // Auto-detect 1/2/3/4 channel textures and compress appropriately
    void compress_auto(const std::string& inputPath, const std::string& outputFolder) {
        int w, h, comp;
        uint8_t* pixels = stbi_load(inputPath.c_str(), &w, &h, &comp, 0);
        if (!pixels) throw std::runtime_error("Failed to load: " + inputPath);

        if (w % 4 || h % 4) throw std::runtime_error("BC formats require width/height divisible by 4");

        std::string name = fs::path(inputPath).stem().string();
        std::string outPath = (fs::path(outputFolder) / (name + ".dds")).string();

        // Detect if image is effectively 1-channel
        bool isGrayscale = true;
        bool isTwoChannel = true;
        for (int i = 0; i < w*h; ++i) {
            uint8_t r = pixels[i*comp + 0];
            uint8_t g = (comp >= 2) ? pixels[i*comp + 1] : 0;
            uint8_t b = (comp >= 3) ? pixels[i*comp + 2] : 0;
            if (r != g || r != b) isGrayscale = false;
            if (b != 0) isTwoChannel = false;
        }

        std::cout << "isGrayscale: " << isGrayscale << std::endl;
        std::cout << "isTwoChannel: " << isTwoChannel << std::endl;

        if (isGrayscale) {
            // extract red channel
            std::vector<uint8_t> rChannel(w*h);
            for (int i=0; i<w*h; ++i) rChannel[i] = pixels[i*comp + 0];
            compress_bc4(rChannel.data(), w, h, outPath);
        }
        else if (isTwoChannel && comp >= 2) {
            std::vector<uint8_t> rgChannel(w*h*2);
            for (int i=0; i<w*h; ++i) {
                rgChannel[i*2 + 0] = pixels[i*comp + 0];
                rgChannel[i*2 + 1] = pixels[i*comp + 1];
            }
            compress_bc5(rgChannel.data(), w, h, outPath);
        }
        else {
            // expand to 4 channels for BC7
            std::vector<uint8_t> rgba(w*h*4);
            for (int i=0; i<w*h; ++i) {
                rgba[i*4 + 0] = pixels[i*comp + 0];
                rgba[i*4 + 1] = (comp>=2) ? pixels[i*comp+1] : 0;
                rgba[i*4 + 2] = (comp>=3) ? pixels[i*comp+2] : 0;
                rgba[i*4 + 3] = (comp==4) ? pixels[i*comp+3] : 255;
            }
            compress_bc7(rgba.data(), w, h, outPath);
        }

        stbi_image_free(pixels);
    }
} // skibidi
*/
int main()
{
    if (true)
    {
        lve::FirstApp firstApp;
        firstApp.run();
    }
    /*else
    {
        rgbcx::init();

        std::string folderPath = "/home/taha/CLionProjects/untitled4/models/stuff/pkg_a_curtains/textures";
        std::string outputFolder = folderPath + "/dds";
        skibidi::fs::create_directories(outputFolder);

        for (const auto& entry : skibidi::fs::directory_iterator(folderPath))
        {
            if (!entry.is_regular_file()) continue;
            try {
                skibidi::compress_auto(entry.path().string(), outputFolder);
                std::cout << "Compressed: " << entry.path().filename() << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error compressing " << entry.path().filename() << ": " << e.what() << std::endl;
            }
        }
    }*/
}
