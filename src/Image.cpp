#define STBI_FAILURE_USERMSG
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <filesystem>
#include <fstream>

#include <stb/stb_image_write.h>
#include <stb/stb_image.h>

#include "Image.h"
#include "DownSampler.h"

std::vector<uint8_t> ReadFile(const std::string& path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	std::vector<uint8_t> data;

	if (!file.is_open())
		return {};

	data.resize(file.tellg());
	file.seekg(0, std::ios::beg);

	file.read(reinterpret_cast<char*>(data.data()), data.size());
	return data;
}

Image Image::ReadFromFile(const std::string& path)
{
	Image image;
	std::cout << "Decoding '" << path << "'...\n";

	std::vector<uint8_t> pngData = ReadFile(path);
	if (pngData.empty())
		return image;

	int channels = 0;
	stbi_uc* raw = stbi_load_from_memory(pngData.data(), (int)pngData.size(), &image.full.width, &image.full.height, &channels, STBI_rgb);
	if (raw == nullptr)
		return image;

	image.thumbnail.width  = image.full.width;
	image.thumbnail.height = image.full.height;

	size_t end = image.full.width * image.full.height * STBI_rgb; // the channel should always be 3 elements (RGB)
	image.fullData = std::vector<uint8_t>(raw, raw + end);

	std::cout << "done!\n";

	DownSampler sampler;
	image.thumbnailData = sampler.Run(raw, image.thumbnail.width, image.thumbnail.height, 3);

	image.succeeded = true;
	return image;
}

bool Image::WriteToFile(const std::string& fullDst, const std::string& thumbnailDst)
{
	std::cout << "Encoding the full screenshot at '" << fullDst << "'...\n";

	if (!stbi_write_jpg(fullDst.c_str(), full.width, full.height, STBI_rgb, fullData.data(), 100))
	{
		std::cout << "\nFailed to encode the full screenshot, aborting...\n";
		return false;
	}

	std::cout << "done!\nEncoding the thumbnail at '" << thumbnailDst << "'... ";

	if (!stbi_write_jpg(thumbnailDst.c_str(), thumbnail.width, thumbnail.height, STBI_rgb, thumbnailData.data(), 100))
	{
		std::cout << "\nFailed to encode the thumbnail, deleting the full screenshot... ";
		std::filesystem::remove(fullDst); // if one file fails, then the other file should be deleted too in order to keep it clean
		std::cout << "cleanup completed!\n";
		return false;
	}
	std::cout << "done!\n";
	return true;
}