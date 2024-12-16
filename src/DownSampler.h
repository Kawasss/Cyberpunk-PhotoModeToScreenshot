#pragma once
#include <vector>

class DownSampler
{
public:
	std::vector<uint8_t> Run(uint8_t* data, int& width, int& height, int count = 1); // must be in RGB format, without A

private:
	std::vector<uint8_t> DownSampleOnce(uint8_t* data, int& width, int& height); // this will halve the resolution
};