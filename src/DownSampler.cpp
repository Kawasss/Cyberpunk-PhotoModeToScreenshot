#include <exception>

#include "Downsampler.h"

struct ColorFull
{
	ColorFull() = default;
	ColorFull(uint16_t r, uint16_t g, uint16_t b) : r(r), g(g), b(b) {}

	uint16_t r, g, b;

	ColorFull operator+(const ColorFull& c)
	{
		return ColorFull(r + c.r, g + c.g, b + c.b);
	}

	ColorFull operator/(uint16_t val)
	{
		return ColorFull(r / val, g / val, b / val);
	}
};

struct Color
{
	uint8_t r, g, b;
};

ColorFull GetFullColor(Color color)
{
	return { color.r, color.g, color.b };
}

Color GetSmallColor(ColorFull color)
{
	return { (uint8_t)color.r, (uint8_t)color.g, (uint8_t)color.b };
}

std::vector<uint8_t> DownSampler::Run(uint8_t* data, int& width, int& height, int count)
{
	if (count <= 0)
		throw std::exception("Invalid downsample count given");

	std::vector<uint8_t> ret;
	for (int i = 0; i < count; i++)
	{
		ret = DownSampleOnce(data, width, height);
		data = ret.data();
	}
	return ret;
}

std::vector<uint8_t> DownSampler::DownSampleOnce(uint8_t* data, int& width, int& height)
{
	std::vector<uint8_t> ret((width * height * 3) / 2);

	Color* pixels = reinterpret_cast<Color*>(data);
	Color* dst    = reinterpret_cast<Color*>(ret.data());

	for (int y = 0; y < height; y += 2)
	{
		for (int x = 0; x < width; x += 2)
		{
			int indexBig   = y * width + x;
			int indexSmall = y / 2 * width / 2 + x / 2;

			// merge 2x2 pixels into 1 pixel

			ColorFull c00 = GetFullColor(pixels[indexBig + 0]);
			ColorFull c01 = GetFullColor(pixels[indexBig + 1]);
			ColorFull c10 = GetFullColor(pixels[indexBig + width + 0]);
			ColorFull c11 = GetFullColor(pixels[indexBig + width + 1]);

			ColorFull sum = c00 + c01 + c10 + c11;
			dst[indexSmall] = GetSmallColor(sum / 4);
		}
	}
	width  /= 2;
	height /= 2;
	return ret;
}