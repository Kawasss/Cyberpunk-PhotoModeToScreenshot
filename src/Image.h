#pragma once
#include <vector>
#include <string>

struct Dimensions
{
	int width, height;
};

struct Image
{
	bool succeeded = false;
	Dimensions full, thumbnail;

	std::vector<uint8_t> fullData, thumbnailData;

	bool WriteToFile(const std::string& fullDst, const std::string& thumbnailDst);

	static Image ReadFromFile(const std::string& path);
};