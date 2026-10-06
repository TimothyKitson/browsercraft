#pragma once
#include <string>

// Reads the current frame back from the GPU and writes it to a PNG.
// Returns the path written, or an empty string on failure.
std::string saveScreenshot(int width, int height, const std::string& directory);
