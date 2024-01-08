/**
 * \file            sqz.cpp
 * \brief           Example usage of the SQZ image compression library
 */

/*
                    Copyright (c) 2024, Márcio Pais

                    SPDX-License-Identifier: MIT

Requirements:
    - "stb_image.h"         [https://github.com/nothings/stb/blob/master/stb_image.h]
    - "stb_image_write.h"   [https://github.com/nothings/stb/blob/master/stb_image_write.h]
    - "CLI11.hpp"           [https://github.com/CLIUtils/CLI11]
*/

#define SQZ_IMPLEMENTATION
#include "sqz.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR
#define STBI_NO_JPEG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "CLI11.hpp"

bool ends_with(std::string const & str, std::string const & suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

int main(int argc, char* argv[]) {
    SQZ_image_descriptor_t image = {};
    std::string input_file, output_file;
    std::size_t budget = 0u;
    CLI::App app{ "SQZ - Simple, scalable image codec" };
    app.require_subcommand(1);
    CLI::App* const encode = app.add_subcommand("c", "Compress a PNG/PGM/PPM/PNM image")->ignore_case();
    encode->add_option("input", input_file, "Input PNG/PGM/PPM/PNM image")->option_text(" ")->required()->check(CLI::ExistingFile);
    encode->add_option("output", output_file, "Output image")->option_text(" ")->required();
    encode->add_option("budget", budget, "Requested output image size")->option_text("(optional)")->check(CLI::PositiveNumber);
    encode->add_option("-l,--level", image.dwt_levels, "Number of DWT decompositions to perform (default: 5)")->option_text("(1.." + std::to_string(SQZ_DWT_MAX_LEVEL) + ")")->check(CLI::Range(1, SQZ_DWT_MAX_LEVEL))->default_val(SQZ_DWT_MAX_LEVEL);
    encode->add_option("-m,--mode", image.color_mode, "Internal color mode (default: Grayscale / YCoCg-R)\n0: Grayscale\n1: YCoCg-R\n2: Oklab\n3: logl1")->option_text("(0.." + std::to_string(SQZ_COLOR_MODE_COUNT - 1) + ")")->check(CLI::Range(0, SQZ_COLOR_MODE_COUNT - 1))->default_val(SQZ_COLOR_MODE_YCOCG_R);
    encode->add_option("-o,--order", image.scan_order, "DWT coefficient scanning order (default: Snake)\n0: Raster\n1: Snake\n2: Morton\n3: Hilbert")->option_text("(0.." + std::to_string(SQZ_SCAN_ORDER_COUNT - 1) + ")")->check(CLI::Range(0, SQZ_SCAN_ORDER_COUNT - 1))->default_val(SQZ_SCAN_ORDER_SNAKE);
    encode->add_flag("-s,--subsampling", image.subsampling, "Use additional chroma subsampling")->option_text(" ");
    CLI::App* const decode = app.add_subcommand("d", "Decompress a SQZ image")->ignore_case();
    decode->add_option("input", input_file, "Input SQZ image")->option_text(" ")->required()->check(CLI::ExistingFile);
    decode->add_option("output", output_file, "Output PGM image")->option_text(" ")->required();
    decode->add_option("budget", budget, "Size of the input compressed data that will be consumed")->option_text("(optional)")->check(CLI::PositiveNumber);
    try {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e) {
        return app.exit(CLI::CallForAllHelp());
    }
    std::FILE *input = nullptr, *output = nullptr;
    std::uint8_t *src = nullptr, *buffer = nullptr;
    if (encode->parsed()) {
        int width = 0, height = 0, channels = 0;
        if (!stbi_info(input_file.c_str(), &width, &height, &channels) || (width <= 0) || (height <= 0) || ((channels != 1) && (channels != 3))) {
            std::cout << "Invalid image header, parsing failed" << std::endl;
            return 1;
        }
        image.width = (std::size_t)width;
        image.height = (std::size_t)height;
        image.num_planes = (std::size_t)channels;
        if ((channels == 1) && (image.color_mode > SQZ_COLOR_MODE_GRAYSCALE)) {
            image.color_mode = SQZ_COLOR_MODE_GRAYSCALE;
        }
        src = (std::uint8_t*)stbi_load(input_file.c_str(), &width, &height, nullptr, channels);
        if (src == nullptr) {
            std::cout << "Error loading input image" << std::endl;
            return 2;
        }
        if (budget < SQZ_HEADER_SIZE + 1u) {    /* assume (near) lossless compression expected */
            budget = image.width * image.height * image.num_planes;
            budget += budget >> 2u;
        }
        buffer = (std::uint8_t*)std::calloc(budget, sizeof(std::uint8_t));
        if (buffer == nullptr) {
            std::cout << "Insufficient memory" << std::endl;
            return 7;
        }
        SQZ_status_t result = SQZ_encode(src, buffer, &image, &budget);
        std::free(src);
        if (result != SQZ_RESULT_OK) {
            std::cout << "Error compressing image, code: " << (int)result << std::endl;
        }
        else {
            output = std::fopen(output_file.c_str(), "wb");
            if (output == nullptr) {
                std::cout << "Error creating output image" << std::endl;
                return 8;
            }
            std::fwrite(buffer, sizeof(std::uint8_t), budget, output);
            std::fclose(output);
        }
        std::free(buffer);
        return (int)result;
    }
    else {
        input = std::fopen(input_file.c_str(), "rb");
        if (input == nullptr) {
            std::cout << "Error reading input image" << std::endl;
            return 1;
        }
        std::fseek(input, 0, SEEK_END);
        std::size_t size = std::ftell(input);
        std::fseek(input, 0, SEEK_SET);
        if ((budget < SQZ_HEADER_MAGIC + 1u) || (budget > size)) {
            budget = size;
        }
        size = 0u;
        src = (std::uint8_t*)std::malloc(budget);
        if (src == nullptr) {
            std::cout << "Insufficient memory" << std::endl;
            std::fclose(input);
            return 2;
        }
        if (fread(src, sizeof(std::uint8_t), budget, input) != budget) {
            std::cout << "Error reading input image" << std::endl;
            std::fclose(input);
            std::free(src);
            return 3;
        }
        std::fclose(input);
        SQZ_status_t result = SQZ_decode(src, buffer, budget, &size, &image);
        if (result != SQZ_BUFFER_TOO_SMALL) {
            std::cout << "Error parsing SQZ image, code: " << (int)result << std::endl;
            std::free(src);
            return (int)result;
        }
        buffer = (std::uint8_t*)std::malloc(size);
        if (buffer == nullptr) {
            std::cout << "Insufficient memory" << std::endl;
            std::free(src);
            return 4;
        }
        result = SQZ_decode(src, buffer, budget, &size, &image);
        std::free(src);
        if (result != SQZ_RESULT_OK) {
            std::cout << "Error decompressing SQZ image, code: " << (int)result << std::endl;
        }
        else {
            if (ends_with(output_file, ".png")) {
                if (!stbi_write_png(output_file.c_str(), (int)image.width, (int)image.height, (int)image.num_planes, buffer, 0)) {
                    std::cout << "Error writing output PNG image" << std::endl;
                    std::free(buffer);
                    return 5;
                }
            }
            else {
                output = std::fopen(output_file.c_str(), "wb");
                if (output == nullptr) {
                    std::cout << "Error creating output file" << std::endl;
                    std::free(buffer);
                    return 6;
                }
                if ((std::fprintf(output, "P%u\n%u %u\n255\n", 5u + (std::uint32_t)(image.color_mode != SQZ_COLOR_MODE_GRAYSCALE), (std::uint32_t)image.width, (std::uint32_t)image.height) < 0) ||
                    (std::fwrite(buffer, sizeof(std::uint8_t), size, output) != size))
                {
                    std::cout << "Error writing to output file" << std::endl;
                }
                std::fclose(output);
            }
        }
        std::free(buffer);
        return (int)result;
    }
}
