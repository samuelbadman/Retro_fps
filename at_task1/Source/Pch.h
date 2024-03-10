#pragma once

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#include <xaudio2.h>
#include <x3daudio.h>
#include <wrl/client.h>

// Standard library
#include <iostream>
#include <cstdint>
#include <functional>
#include <vector>
#include <array>
#include <optional>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <queue>

// GLM maths library
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Maths/glm/vec2.hpp"
#include "Maths/glm/vec3.hpp"
#include "Maths/glm/vec4.hpp"
#include "Maths/glm/mat3x3.hpp"
#include "Maths/glm/mat4x4.hpp"
#include "Maths/glm/ext/matrix_transform.hpp"
#include "Maths/glm/ext/matrix_clip_space.hpp"
#include "Maths/glm/gtc/quaternion.hpp"
#include "Maths/glm/gtc/random.hpp"

// entt
#include "Game/entt/entt.hpp"

// stb image
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "Renderer/stb_image.h"

// Vulkan
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
