#include "Pch.h"
#include "Renderer.h"
#include "Console.h"
#include "Binary/Binary.h"
#include "Maths/Maths.h"

#include "Game/Level.h"
#include "Game/Components/StaticMeshComponent.h"
#include "Game/Components/TransformComponent.h"

// Remove windows CreateSemaphore definition
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif // CreateSemaphore

class QueueFamilyIndices
{
public:
    bool IsComplete() const { return GraphicsFamilyIndex.has_value() && ComputeFamilyIndex.has_value() && TransferFamilyIndex.has_value(); }

    uint32_t GetGraphicsFamilyIndex() const { return GraphicsFamilyIndex.value(); }
    void SetGraphicsFamilyIndex(const uint32_t index) { GraphicsFamilyIndex = index; }

    uint32_t GetComputeFamilyIndex() const { return ComputeFamilyIndex.value(); }
    void SetComputeFamilyIndex(const uint32_t index) { ComputeFamilyIndex = index; }

    uint32_t GetTransferFamilyIndex() const { return TransferFamilyIndex.value(); }
    void SetTransferFamilyIndex(const uint32_t index) { TransferFamilyIndex = index; }

private:
    std::optional<uint32_t> GraphicsFamilyIndex;
    std::optional<uint32_t> ComputeFamilyIndex;
    std::optional<uint32_t> TransferFamilyIndex;
};

class Geometry
{
public:
    VkBuffer& GetVertexBuffer() { return VertexBuffer; }
    VkDeviceMemory& GetVertexBufferMemory() { return VertexBufferMemory; }
    VkBuffer& GetIndexBuffer() { return IndexBuffer; }
    VkDeviceMemory& GetIndexBufferMemory() { return IndexBufferMemory; }
    void SetIndexCount(const uint32_t count) { IndexCount = count; }

    const VkBuffer& GetVertexBuffer() const { return VertexBuffer; }
    const VkBuffer& GetIndexBuffer() const { return IndexBuffer; }
    const uint32_t GetIndexCount() const { return IndexCount; }

    void Reset()
    {
        VertexBuffer = VK_NULL_HANDLE;
        VertexBufferMemory = VK_NULL_HANDLE;
        IndexBuffer = VK_NULL_HANDLE;
        IndexBufferMemory = VK_NULL_HANDLE;
        IndexCount = 0;
    }

private:
    VkBuffer VertexBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory VertexBufferMemory{ VK_NULL_HANDLE };
    VkBuffer IndexBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory IndexBufferMemory{ VK_NULL_HANDLE };
    uint32_t IndexCount{ 0 };
};

class Texture
{
public:
    VkImage& GetImage() { return Image; }
    VkDeviceMemory& GetImageMemory() { return ImageMemory; }
    VkImageView& GetImageView() { return ImageView; }

    const VkImage& GetImage() const { return Image; }
    const VkDeviceMemory& GetImageMemory() const { return ImageMemory; }
    const VkImageView& GetImageView() const { return ImageView; }

    void Reset()
    {
        Image = VK_NULL_HANDLE;
        ImageMemory = VK_NULL_HANDLE;
        ImageView = VK_NULL_HANDLE;
    }

private:
    VkImage Image{ VK_NULL_HANDLE };
    VkDeviceMemory ImageMemory{ VK_NULL_HANDLE };
    VkImageView ImageView{ VK_NULL_HANDLE };
};

// Settings
constexpr auto MAX_SYNCHRONIZATION_TIMEOUT_DURATION{ std::chrono::nanoseconds::max().count() };
constexpr glm::vec4 CLEAR_COLOR{ 1.0f, 0.0f, 1.0f, 1.0f };
constexpr size_t MAX_FRAMES_IN_FLIGHT{ 3 };
constexpr uint32_t MAX_DRAW_ITEMS_PER_FRAME{ 64 };
constexpr size_t WORLD_MATRIX_SIZE_BYTES{ sizeof(glm::mat4) };

static uint32_t gSwapchainImageCount{ 3 }; // Triple buffering
static VkFormat gDepthStencilFormat{ VK_FORMAT_UNDEFINED };
static bool gStencilAvailable{ false };

const std::string gVertexShaderPath{ "Shaders/binary/VertexShader.spv" };
const std::string gFragmentShaderPath{ "Shaders/binary/FragmentShader.spv" };

// Uniform buffers
struct PerObjectUniforms
{
    glm::mat4 WorldMatrix{ glm::identity<glm::mat4>() };
    glm::mat4 NormalMatrix{ glm::identity<glm::mat4>() };
    glm::vec4 Data1{ 0.0f, 0.0f, 0.0f, 0.0f };
    uint32_t SamplerID{ 0 };
    uint32_t TextureID{ 0 };
};

struct PerFrameUniforms
{
    glm::vec4 DirectionalLightColor;
    glm::vec4 DirectionalLightWorldSpaceDirection;
};

struct PerRenderPassUniforms
{
    glm::mat4 ViewMatrix{ glm::identity<glm::mat4>() };
    glm::mat4 ProjectionMatrix{ glm::identity<glm::mat4>() };
    glm::vec4 CameraWorldSpacePosition{ 0.0f, 0.0f, 0.0f, 1.0f };
};

constexpr uint32_t UNIFORM_BUFFER_COUNT{ 3 };

std::vector<void*> gMappedPerObjectUniformBuffers;
std::vector<void*> gMappedPerFrameUniformBuffers;
std::vector<void*> gMappedPerRenderPassUniformBuffers;

// Vulkan object handles
static VkInstance gInstance{ VK_NULL_HANDLE };
static VkPhysicalDevice gPhysicalDevice{ VK_NULL_HANDLE };
static VkPhysicalDeviceProperties gPhysicalDeviceProperties{};
static VkPhysicalDeviceMemoryProperties gPhysicalDeviceMemoryProperties{};
static VkPhysicalDeviceFeatures gPhysicalDeviceFeatures{};
static VkDevice gDevice{ VK_NULL_HANDLE };
static QueueFamilyIndices gQueueFamilyIndices{};
static VkQueue gGraphicsQueue{ VK_NULL_HANDLE };
static VkQueue gTransferQueue{ VK_NULL_HANDLE };
static VkRenderPass gRenderPass{ VK_NULL_HANDLE };
static std::vector<VkSemaphore> gImageAvailableSemaphores;
static std::vector<VkSemaphore> gRenderFinishedSemaphores;
static std::vector<VkFence> gInFlightFences;
static std::vector<VkFence> gImagesInFlight;
static VkDescriptorSetLayout gDescriptorSetLayout{ VK_NULL_HANDLE };
static VkPipelineLayout gGraphicsPipelineLayout{ VK_NULL_HANDLE };
static VkPipeline gGraphicsPipeline{ VK_NULL_HANDLE };
static VkCommandPool gGraphicsCommandPool{ VK_NULL_HANDLE };
static std::vector<VkCommandBuffer> gGraphicsCommandBuffers;
static VkCommandPool gTransferTemporaryCommandPool{ VK_NULL_HANDLE };
static VkDescriptorPool gDescriptorPool{ VK_NULL_HANDLE };
static std::vector<VkDescriptorSet> gDescriptorSets;

static std::vector<VkBuffer> gPerObjectUniformBuffers;
static std::vector<VkDeviceMemory> gPerObjectUniformBuffersMemory;
static std::vector<VkBuffer> gPerFrameUniformBuffers;
static std::vector<VkDeviceMemory> gPerFrameUniformBuffersMemory;
static std::vector<VkBuffer> gPerRenderPassUniformBuffers;
static std::vector<VkDeviceMemory> gPerRenderPassUniformBuffersMemory;
static VkDeviceSize gMinUniformBufferOffsetAlignment{ 0 };

static VkSurfaceKHR gSurface{ VK_NULL_HANDLE };
static VkSwapchainKHR gSwapchain{ VK_NULL_HANDLE };
static std::vector<VkImage> gSwapchainImages;
static std::vector<VkFramebuffer> gFramebuffers;
static std::vector<VkImageView> gSwapchainImageViews;
static VkImage gDepthStencilImage{ VK_NULL_HANDLE };
static VkImageView gDepthStencilImageView{ VK_NULL_HANDLE };
static VkDeviceMemory gDepthStencilImageMemory{ VK_NULL_HANDLE };
static VkSurfaceCapabilitiesKHR gSurfaceCapabilities{};
static VkSurfaceFormatKHR gSurfaceFormat{};
uint32_t gSurfaceWidth{ 0 };
uint32_t gSurfaceHeight{ 0 };

VkViewport gViewport{};
VkRect2D gScissor{};

// Layer and extension names
static std::vector<const char*> gEnabledInstanceLayerNames;
static std::vector<const char*> gEnabledInstanceExtensionNames;
static std::vector<const char*> gEnabledDeviceLayerNames;
static std::vector<const char*> gEnabledDeviceExtensionNames;

// Geometry
constexpr uint32_t MAX_LOADED_GEOMETRY_COUNT{ 1024 };

static std::array<Geometry, MAX_LOADED_GEOMETRY_COUNT> gLoadedGeometry{};
static std::queue<uint32_t> gAvailableGeometryIDs;
static std::vector<uint32_t> gUsedGeometryIDs;

// Textures
constexpr uint32_t MAX_LOADED_TEXTURE_COUNT{ 32 };

static std::array<Texture, MAX_LOADED_TEXTURE_COUNT> gLoadedTextures{};
static std::queue<uint32_t> gAvailableTextureIDs;
static std::vector<uint32_t> gUsedTextureIDs;

// Samplers
constexpr uint32_t SAMPLER_COUNT{ 2 };

static std::vector<VkSampler> gSamplers(SAMPLER_COUNT, VK_NULL_HANDLE);

// Descriptor layout
constexpr uint32_t DYNAMIC_OFFSET_COUNT{ 2 };
constexpr size_t PER_OBJECT_UNIFORMS_DYNAMIC_OFFSET_INDEX{ 0 };
constexpr size_t PER_RENDER_PASS_UNIFORMS_DYNAMIC_OFFSET_INDEX{ 1 };

// Rendering
constexpr uint32_t MAX_RENDER_PASS_COUNT{ 2 };

static size_t gCurrentFrame{ 0 };
static uint32_t gImageIndex{ 0 };
static VkCommandBuffer gCurrentFrameCommandBuffer{ VK_NULL_HANDLE };
static uint32_t gDrawItemSubmitCount{ 0 };
static uint32_t gRenderPassCount{ 0 };
static std::array<uint32_t, DYNAMIC_OFFSET_COUNT> gDynamicOffsets{};

// Debug
static VkDebugReportCallbackEXT gDebugReport{ VK_NULL_HANDLE };
VkDebugReportCallbackCreateInfoEXT gDebugCallbackCreateInfo{};

bool gDebugEnabled{ false };
Renderer::EVulkanDebugReportLevel gDebugReportLevel{ Renderer::EVulkanDebugReportLevel::WARNING };

#ifdef _DEBUG
static const char* gDebugValidationLayerName{ "VK_LAYER_KHRONOS_validation" };

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT{ nullptr };
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT{ nullptr };

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugReportFlagsEXT msgFlags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t srcObject,
    size_t location,
    int32_t msgCode,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT && 
        gDebugReportLevel <= Renderer::EVulkanDebugReportLevel::DEBUG)
    {
        LOG(std::string("VULKAN DEBUG @[") + layerPrefix + std::string("]:") + msg);
    }
    else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT &&
        gDebugReportLevel <= Renderer::EVulkanDebugReportLevel::INFO)
    {
        LOG(std::string("VULKAN INFO @[") + layerPrefix + std::string("]:") + msg);
    }
    else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT &&
        gDebugReportLevel <= Renderer::EVulkanDebugReportLevel::WARNING)
    {
        LOG(std::string("VULKAN WARNING @[") + layerPrefix + std::string("]:") + msg);
    }
    else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT &&
        gDebugReportLevel <= Renderer::EVulkanDebugReportLevel::PERFORMANCE_WARNING)
    {
        LOG(std::string("VULKAN PERFORMANCE WARNING @[") + layerPrefix + std::string("]:") + msg);
    }
    else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT &&
        gDebugReportLevel <= Renderer::EVulkanDebugReportLevel::ERR)
    {
        LOG(std::string("VULKAN ERROR @[") + layerPrefix + std::string("]:") + msg);
    }

    return false;
}

static bool LoadDebugReportCallback(VkInstance instance)
{
    // Get function addresses for create and destroy debug report callback
    fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

    // Check the function pointers are valid
    if (fvkCreateDebugReportCallbackEXT == nullptr || fvkDestroyDebugReportCallbackEXT == nullptr)
    {
        return false;
    }

    // Describe the callback create info
    gDebugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    gDebugCallbackCreateInfo.pfnCallback = VulkanDebugCallback;
    gDebugCallbackCreateInfo.flags =
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;


    // Create the debug report callback
    return fvkCreateDebugReportCallbackEXT(instance, &gDebugCallbackCreateInfo, nullptr, &gDebugReport) == VK_SUCCESS;
}

static bool EnableDebugLayersAndExtensions()
{
    // Get the instance layer property count
    uint32_t instancePropertyCount;
    if (vkEnumerateInstanceLayerProperties(&instancePropertyCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    // Get the instance layer properties
    std::vector<VkLayerProperties> instanceLayerProperties(instancePropertyCount);
    if (vkEnumerateInstanceLayerProperties(&instancePropertyCount, instanceLayerProperties.data()) != VK_SUCCESS)
    {
        return false;
    }

    // Check if the Khronos debug validation layer is installed
    bool validationLayerInstalled{ false };
    LOG("Installed instance layers:");
    for (auto& property : instanceLayerProperties)
    {
        // Early exit if the validation layer has already been found
        if (validationLayerInstalled)
        {
            break;
        }

        LOG(property.layerName);

        // If the layer name is the debug validation layer name
        if (strcmp(property.layerName, gDebugValidationLayerName) == 0)
        {
            // The debug validaiton layer is installed
            validationLayerInstalled = true;
        }
    }

    // Add the debug validation layer name to the enabled instance layer names if it is installed
    if (validationLayerInstalled)
    {
        // Instance layers
        gEnabledInstanceLayerNames.push_back(gDebugValidationLayerName);

        // Enable the debug report extension at the instance level
        gEnabledInstanceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        // Device layers
        gEnabledDeviceLayerNames.push_back(gDebugValidationLayerName);

        LOG(std::string("\n") + std::string(gDebugValidationLayerName) + " enabled.");
        LOG(std::string(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) + " enabled.\n");

        return true;
    }
    else
    {
        LOG("\nDebug layers were not enabled. " + std::string(gDebugValidationLayerName) + " could not be found.\n");
        
        return false;
    }
}
#endif // _DEBUG

static void EnableLayersAndExtensions()
{
    // Enable the WSI surface extension
    gEnabledInstanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    gEnabledInstanceExtensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    // Enable swapchain extension
    gEnabledDeviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

static bool CreateInstance()
{
    // Describe application info struct
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
    appInfo.pApplicationName = "Vulkan renderer";

    // Describe instance create info
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(gEnabledInstanceLayerNames.size());
    instanceCreateInfo.ppEnabledLayerNames = gEnabledInstanceLayerNames.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(gEnabledInstanceExtensionNames.size());
    instanceCreateInfo.ppEnabledExtensionNames = gEnabledInstanceExtensionNames.data();
    instanceCreateInfo.pNext = &gDebugCallbackCreateInfo;

    // Create the vulkan instance
    return vkCreateInstance(&instanceCreateInfo, nullptr, &gInstance) == VK_SUCCESS;
}

static VkPhysicalDevice GetPhysicalDevice(VkDeviceSize& minUniformBufferOffsetAlignment, 
    VkPhysicalDeviceProperties& physicalDeviceProperties, VkPhysicalDeviceMemoryProperties& phyiscalDeviceMemoryProperties, VkPhysicalDeviceFeatures& physicalDeviceFeatures)
{
    // Retrieve amount of connected physical devices
    uint32_t deviceCount{ 0 };
    if (vkEnumeratePhysicalDevices(gInstance, &deviceCount, nullptr) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    // Check that there was at least 1 physical device
    if (deviceCount == 0)
    {
        return VK_NULL_HANDLE;
    }

    // Retrieve the connected physical devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (vkEnumeratePhysicalDevices(gInstance, &deviceCount, devices.data()) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    // Use the physical device that is a dedicated GPU and has the highest video memory
    VkDeviceSize maxVideoMemory{ 0 };
    VkDeviceSize bestMinUniformBufferOffsetAlignment{ 0 };
    VkPhysicalDeviceProperties bestDeviceProperties{};
    VkPhysicalDeviceMemoryProperties bestDeviceMemoryProperties{};
    VkPhysicalDeviceFeatures bestDeviceFeatures{};
    VkPhysicalDevice bestDevice{ VK_NULL_HANDLE };
    for (const auto& device : devices)
    {
        // Get the physical device properties
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Get the physical device memory properties
        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);

        // Get the physical device features
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // If the physical device is not a dedicated GPU or does not have a memory heap, skip to the next physical device
        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            deviceMemoryProperties.memoryHeapCount == 0)
        {
            continue;
        }

        // Get the physical device's video memory size
        auto videoMemorySize = deviceMemoryProperties.memoryHeaps[0].size;

        // Check if the physical device has more video memory
        if (videoMemorySize > maxVideoMemory)
        {
            // Set this device as the best device
            bestDevice = device;

            // Update max video memory
            maxVideoMemory = videoMemorySize;

            // Store minUniformBufferOffetAlignment
            bestMinUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;

            // Store device properties
            bestDeviceProperties = deviceProperties;

            // Store device memory properties
            bestDeviceMemoryProperties = deviceMemoryProperties;

            // Store device features
            bestDeviceFeatures = deviceFeatures;
        }
    }

    // Check a valid device was found
    if (bestDevice == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }

    // Set minimumUniformBufferOffsetAlignment
    minUniformBufferOffsetAlignment = bestMinUniformBufferOffsetAlignment;

    // Set physical device properties
    physicalDeviceProperties = bestDeviceProperties;

    // Set physical device memory properties
    phyiscalDeviceMemoryProperties = bestDeviceMemoryProperties;

    // Set physical device features
    physicalDeviceFeatures = bestDeviceFeatures;

    return bestDevice;
}

static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device)
{
    QueueFamilyIndices indices{};

    // Get the physical device's queue family count
    uint32_t queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Get the physical device's queue family properties
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Get the physical device's queue family indices
    uint32_t index{ 0 };
    for (const auto& queueFamily : queueFamilies)
    {
        // Early exit if all queue family indices have been found
        if (indices.IsComplete())
        {
            break;
        }

        // If the queue family is a graphics queue
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            // Set the graphics family index to index
            indices.SetGraphicsFamilyIndex(index);
        }
        // Else if the queue family is a compute queue
        else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            // Set the compute family index to index
            indices.SetComputeFamilyIndex(index);
        }
        // else if the queue family is a transfer queue
        else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            // Set the transfer family index to index
            indices.SetTransferFamilyIndex(index);
        }

        // Increment index
        ++index;
    }

    return indices;
}

static bool CreateLogicalDevice(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceFeatures& physicalDeviceFeatures,
    const uint32_t graphicsQueueFamilyIndex, const uint32_t graphicsQueueCount, 
    const uint32_t computeQueueFamilyIndex, const uint32_t computeQueueCount, 
    const uint32_t transferQueueFamilyIndex, const uint32_t transferQueueCount)
{
    // Declare device queue create info vector
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;

    // Describe the graphics queue create info if a graphics queue is requested and add it to the vector
    if (graphicsQueueCount > 0)
    {
        auto& info = deviceQueueCreateInfos.emplace_back();
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = graphicsQueueFamilyIndex;
        info.queueCount = graphicsQueueCount;
        float queuePriority = 1.0f;
        info.pQueuePriorities = &queuePriority;
    }

    // Describe the compute queue create info if a compute queue is requested and add it to the vector
    if (computeQueueCount > 0)
    {
        auto& info = deviceQueueCreateInfos.emplace_back();
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = computeQueueFamilyIndex;
        info.queueCount = computeQueueCount;
        float queuePriority = 1.0f;
        info.pQueuePriorities = &queuePriority;
    }

    // Describe the transfer queue create info if a transfer queue is requested and add it to the vector
    if (transferQueueCount > 0)
    {
        auto& info = deviceQueueCreateInfos.emplace_back();
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = transferQueueFamilyIndex;
        info.queueCount = transferQueueCount;
        float queuePriority = 1.0f;
        info.pQueuePriorities = &queuePriority;
    }

    // Enable null descriptors
    VkPhysicalDeviceRobustness2FeaturesEXT robustnessFeatures{};
    robustnessFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    robustnessFeatures.nullDescriptor = VK_TRUE;

    // Enable binding partially bound descriptors
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
    descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.pNext = &robustnessFeatures;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;

    // Describe device create info
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &descriptorIndexingFeatures;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(gEnabledDeviceLayerNames.size());
    deviceCreateInfo.ppEnabledLayerNames = gEnabledDeviceLayerNames.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(gEnabledDeviceExtensionNames.size());
    deviceCreateInfo.ppEnabledExtensionNames = gEnabledDeviceExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    // Create the logical device
    return vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &gDevice) == VK_SUCCESS;
}

static bool CreateWin32Surface(VkInstance instance, HWND windowHandle, HINSTANCE hInstance, VkSurfaceKHR* pSurface)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = windowHandle;
    surfaceCreateInfo.hinstance = hInstance;

    return vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, pSurface) == VK_SUCCESS;
}

static bool QueryWSISupport(VkPhysicalDevice physicalDevice, const uint32_t graphicsQueueFamilyIndex, VkSurfaceKHR surface)
{
    // Check WSI extension is supported
    VkBool32 wsiSupported;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamilyIndex, surface, &wsiSupported) != VK_SUCCESS)
    {
        return false;
    }
 
    return wsiSupported;
}

static bool QuerySurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabalities, uint32_t& surfaceWidth, uint32_t& surfaceHeight)
{
    // Get the surface capabalities
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabalities) != VK_SUCCESS)
    {
        return false;
    }

    // Get the created surface size
    if (pSurfaceCapabalities->currentExtent.width < UINT32_MAX)
    {
        surfaceWidth = pSurfaceCapabalities->currentExtent.width;
        surfaceHeight = pSurfaceCapabalities->currentExtent.height;
    }

    return true;
}

static bool QuerySurfaceFormat(const uint32_t graphicsQueueFamilyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR* surfaceFormat)
{
    // Get the surface formats count
    uint32_t surfaceFormatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    // Get the surface formats
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()) != VK_SUCCESS)
    {
        return false;
    }

    // Get the first available surface format
    const auto& firstAvailableSurfaceFormat = surfaceFormats[0];

    // If the first available surface format is not specified
    if (firstAvailableSurfaceFormat.format == VK_FORMAT_UNDEFINED)
    {
        // Set the format to 32 bit rgba and sRGB color space
        surfaceFormat->format = VK_FORMAT_B8G8R8A8_UNORM;
        surfaceFormat->colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {
        // Use the first available format
        *surfaceFormat = firstAvailableSurfaceFormat;
    }

    return true;
}

static bool CreateSwapchain(
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    uint32_t imageCount, 
    VkPresentModeKHR targetPresentMode,
    VkSurfaceKHR surface, 
    const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
    const VkSurfaceFormatKHR& surfaceFormat, 
    const uint32_t surfaceWidth, 
    const uint32_t surfaceHeight, 
    VkSwapchainKHR* pSwapchain)
{
    // Check if the surface supports the requested amount of images
    if (imageCount < surfaceCapabilities.minImageCount + 1)
    {
        imageCount = surfaceCapabilities.minImageCount + 1;
    }

    // Check if the max image count needs to be checked as a max image count of 0 can be an unlimited amount of swapchain images
    if (surfaceCapabilities.maxImageCount > 0)
    {
        if (imageCount > surfaceCapabilities.maxImageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }
    }

    // Create default present mode first in first out
    VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR };

    // Get the amount of present modes supported by the surface
    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    // Get the supported present modes
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
    {
        return false;
    }

    // Check if the target present mode is supported
    for (const auto& mode : presentModes)
    {
        // If the present mode is the target present mode
        if (mode == targetPresentMode)
        {
            // Set the present mode to the mode
            presentMode = mode;
            LOG("Target present mode is supported. Using target present mode.\n");
            break;
        }
    }

    if (presentMode != targetPresentMode)
    {
        LOG("Target present mode is not supported. using default present mode: VK_PRESENT_MODE_FIFO_KHR.\n");
    }

    // Describe the swapchain create info
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent.width = surfaceWidth;
    createInfo.imageExtent.height = surfaceHeight;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    createInfo.pQueueFamilyIndices = nullptr; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create the swapchain
    return vkCreateSwapchainKHR(device, &createInfo, nullptr, pSwapchain) == VK_SUCCESS;
}

static bool GetSwapchainImagesAndCreateViews(
    VkDevice device,
    VkSwapchainKHR swapchain,
    std::vector<VkImage>& swapchainImages, 
    uint32_t swapchainImageCount,
    std::vector<VkImageView>& swapchainImageViews,
    const VkFormat imageFormat)
{
    // Resize the spawchain image vectors
    swapchainImages.resize(swapchainImageCount);
    swapchainImageViews.resize(swapchainImageCount);

    // Get the swapchain images
    if (vkGetSwapchainImagesKHR(device, swapchain, &gSwapchainImageCount, swapchainImages.data()) != VK_SUCCESS)
    {
        return false;
    }

    // Create an image view for each swapchain image
    for (uint32_t i = 0; i < swapchainImageCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = imageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

static uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties, const VkMemoryRequirements& memoryRequirements,
    const VkMemoryPropertyFlags memoryProperties)
{
    auto requiredProperties{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
    {
        if (memoryRequirements.memoryTypeBits & (1 << i))
        {
            if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties)
            {
                return i;
            }
        }
    }

    return UINT32_MAX;
}

static bool CreateDepthStencilImageAndView(VkDevice device, VkPhysicalDevice physicalDevice,const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    uint32_t width, uint32_t height, VkImage* pImage, VkImageView* pImageView, VkDeviceMemory* pMemory)
{
    // Find supported depth stencil format
    std::array<VkFormat, 5> tryFormats{
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM
    };

    for (const auto& format : tryFormats)
    {
        // Get the format's properties
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

        // Check the format's optimal tiling features supports a depth stencil attachment
        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            // Use the format
            gDepthStencilFormat = format;
            break;
        }
    }

    // Check a supported format was found
    if (gDepthStencilFormat == VK_FORMAT_UNDEFINED)
    {
        return false;
    }

    // Check if a stencil buffer is supported
    if (gDepthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        gDepthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
        gDepthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
        gDepthStencilFormat == VK_FORMAT_S8_UINT)
    {
        gStencilAvailable = true;
    }
    else
    {
        gStencilAvailable = false;
    }

    // Describe the image create info
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = gDepthStencilFormat;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0; // Ignored when sharing mode is VK_SHARING_MODE_EXCLUSIVE
    imageCreateInfo.pQueueFamilyIndices = nullptr; // Ignored when sharing mode is VK_SHARING_MODE_EXCLUSIVE
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Create the image
    if (vkCreateImage(device, &imageCreateInfo, nullptr, pImage) != VK_SUCCESS)
    {
        return false;
    }

    // Get the depth stencil image memory requirements
    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, *pImage, &imageMemoryRequirements);

    // Get the memory type index
    uint32_t memoryIndex = FindMemoryTypeIndex(physicalDeviceMemoryProperties, imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Check a memory index was found
    if (memoryIndex == UINT32_MAX)
    {
        return false;
    }

    // Describe image memory allocation
    VkMemoryAllocateInfo memoryAllocInfo{};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = imageMemoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = memoryIndex;

    // Allocate memory for the image
    if (vkAllocateMemory(device, &memoryAllocInfo, nullptr, pMemory) != VK_SUCCESS)
    {
        return false;
    }

    // Bind image memory
    if (vkBindImageMemory(device, *pImage, *pMemory, 0) != VK_SUCCESS)
    {
        return false;
    }

    // Descrive the image view create info
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = *pImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = gDepthStencilFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (gStencilAvailable ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    // Create the image view
    if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, pImageView) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

static bool CreateCommandPool(VkDevice device, const uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, VkCommandPool* pCommandPool)
{
    // Describe create info
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = flags;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    // Create the command pool
    return vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, pCommandPool) == VK_SUCCESS;
}

static bool AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, VkCommandBuffer* pCommandBuffers)
{
    // Describe allocate info
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    // Allocate command buffers
    return vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, pCommandBuffers) == VK_SUCCESS;
}

static bool CreateRenderPass(VkDevice device, const VkFormat& surfaceFormat, const VkFormat& depthStencilFormat, VkRenderPass* pRenderPass)
{
    // Create an array of attachments for the render pass. Only 1 currently as the render pass 
    // will only use a color attachment. This will need to be increased when a depth stencil attachment is created
    std::array<VkAttachmentDescription, 2> attachments{};
    auto& colorAttachment = attachments[0];
    colorAttachment.flags = 0;
    colorAttachment.format = surfaceFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Create an array of attachment references. This color attachment will be referenced in the fragment shader at layout location = 0
    std::array<VkAttachmentReference, 1> subpass0ColorAttachments{};
    subpass0ColorAttachments[0].attachment = 0;
    subpass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create depth stencil attachment reference. Only 1 depth stencil attachment is allowed per subpass
    auto& depthStencilAttachment = attachments[1];
    depthStencilAttachment.format = depthStencilFormat;
    depthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachment.stencilLoadOp = gStencilAvailable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference subpass0DepthStencilAttachment{};
    subpass0DepthStencilAttachment.attachment = 1;
    subpass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create an array of subpasses
    std::array<VkSubpassDescription, 1> subpasses{};
    auto& subpass0 = subpasses[0];
    subpass0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass0.inputAttachmentCount = 0;
    subpass0.pInputAttachments = nullptr;
    subpass0.colorAttachmentCount = static_cast<uint32_t>(subpass0ColorAttachments.size());
    subpass0.pColorAttachments = subpass0ColorAttachments.data();
    subpass0.pDepthStencilAttachment = &subpass0DepthStencilAttachment;

    // Describe dependency 
    VkSubpassDependency dependency{};
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Describe create info
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    // Create the render pass
    return vkCreateRenderPass(device, &createInfo, nullptr, pRenderPass) == VK_SUCCESS;
}

static bool CreateFramebuffers(
    VkDevice device,
    const uint32_t swapchainImageCount, 
    VkRenderPass renderPass, 
    const uint32_t surfaceWidth, 
    const uint32_t surfaceHeight, 
    const std::vector<VkImageView>& swapchainImageViews,
    VkImageView depthStencilImageView,
    std::vector<VkFramebuffer>& framebuffers)
{
    // Resize the framebuffers vector to create a framebuffer for each image in the swapchain
    framebuffers.resize(swapchainImageCount);

    // Create a framebuffer for each image in the swapchain
    for (uint32_t i = 0; i < swapchainImageCount; ++i)
    {
        // Create image views to the render pass attachments. The color attachment is the swapchain image this framebuffer
        // refers to and the depth stencil attachment is the depth stencil attachment
        std::array<VkImageView, 2> attachments{
            swapchainImageViews[i],
            depthStencilImageView
        };

        // Describe framebuffer create info
        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.width = surfaceWidth;
        createInfo.height = surfaceHeight;
        createInfo.layers = 1;

        // Create a framebuffer
        if (vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

static bool CreateFence(VkDevice device, VkFence* pFence)
{
    // Describe fence create info
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create the fence
    return vkCreateFence(device, &createInfo, nullptr, pFence) == VK_SUCCESS;
}

static bool CreateSemaphore(VkDevice device, VkSemaphore* pSemaphore)
{
    // Describe semaphore create info
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Create the semaphore
    return vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, pSemaphore) == VK_SUCCESS;
}

static bool CreateShaderModule(VkDevice device, const BinaryBuffer& shaderBinaryBuffer, VkShaderModule* pShaderModule)
{
    // Describe shader module create info
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderBinaryBuffer.GetBufferLength();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBinaryBuffer.GetBufferPointer());

    // Create the shader module
    return vkCreateShaderModule(device, &createInfo, nullptr, pShaderModule) == VK_SUCCESS;
}

static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice)
{
    // Get available types of memory
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    // Find a memory type that supports the buffer
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        // typeFilter and properties are bit fields. Check to see if the bit is set for each supported memory type and 
        // requested properties
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    // Use max uint32 value to specifiy an error. Requested memory type is not supported
    return UINT32_MAX;
}

static bool BeginSingleSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* pCommandBuffer)
{
    // Allocate temporary command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, pCommandBuffer) != VK_SUCCESS)
    {
        return false;
    }

    // Begin recording the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(*pCommandBuffer, &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

static bool EndAndSubmitSingleSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
    // Stop recoding commands
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        return false;
    }

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        return false;
    }

    // Wait for the command buffer to finish executing
    if (vkQueueWaitIdle(queue) != VK_SUCCESS)
    {
        return false;
    }

    // Delete the command buffer
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    return true;
}

static bool CreateBuffer(
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    const VkSharingMode sharingMode,
    const uint32_t queueFamilyIndexCount,
    const uint32_t* pQueueFamilyIndices,
    VkBuffer* pBuffer,
    VkDeviceMemory* pBufferMemory)
{
    // Describe create info
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = pQueueFamilyIndices;

    // Create the buffer
    if (vkCreateBuffer(device, &createInfo, nullptr, pBuffer) != VK_SUCCESS)
    {
        return false;
    }

    // Get buffer memory requirements
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, *pBuffer, &memReqs);

    // Get a suitable memory type
    auto memType = FindMemoryType(
        memReqs.memoryTypeBits,
        properties,
        physicalDevice);

    // Check if memType is valid
    if (memType == UINT32_MAX)
    {
        return false;
    }

    // Allocate vram memory for the buffer
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memType;

    if (vkAllocateMemory(device, &allocInfo, nullptr, pBufferMemory) != VK_SUCCESS)
    {
        return false;
    }

    // Associate memory with the buffer
    if (vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

static void DestroyBuffer(VkBuffer buffer, VkDeviceMemory bufferMemory)
{
    vkDestroyBuffer(gDevice, buffer, nullptr);
    vkFreeMemory(gDevice, bufferMemory, nullptr);
}

static bool CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkImage* pImage, VkDeviceMemory* pImageMemory)
{
    // Describe image create info
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = nullptr;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    // Create the image
    if (vkCreateImage(gDevice, &imageInfo, nullptr, pImage) != VK_SUCCESS)
    {
        return false;
    }

    // Get image memory requirements
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(gDevice, *pImage, &memRequirements);

    // Describe memory allocation info
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gPhysicalDevice);

    // Allocate image memory for the texture
    if (vkAllocateMemory(gDevice, &allocInfo, nullptr, pImageMemory) != VK_SUCCESS)
    {
        return false;
    }

    // Bind image memory to the texture image
    if (vkBindImageMemory(gDevice, *pImage, *pImageMemory, 0) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

static void DestroyImage(VkImage image, VkDeviceMemory imageMemory)
{
    vkDestroyImage(gDevice, image, nullptr);
    vkFreeMemory(gDevice, imageMemory, nullptr);
}

static bool CreateSampler(VkDevice device, VkFilter filter, VkSampler* pSampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = gPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;

    return vkCreateSampler(device, &samplerInfo, nullptr, pSampler) == VK_SUCCESS;
}

static void CopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    // Copy the source buffer to the destination buffer
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, const uint32_t mipLevels,
    VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // If transitioning to transfer destination, writes to the image must wait until after the transition
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    // Else if transitioning to shader read only, shader read operations must wait until after the transition
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        assert(false && "Unsupported image layout transition.");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, 
        destinationStage,
        0,
        0, 
        nullptr,
        0, 
        nullptr,
        1,
        &barrier
    );
}

static void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

// Generates a full mipmap chain for the image, leaving it in a state suitable for reading from the fragment shader
static void GenerateMipmaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(gPhysicalDevice, format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        assert(false && "Image format does not support linear blitting.");
    }

    // Describe common properties of image barrier among every transition to be made
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    // Record blit commands
    int32_t mipWidth = textureWidth;
    int32_t mipHeight = textureHeight;

    for (uint32_t i = 1; i < mipLevels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        // Transition mipmap level to transfer source layout
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        // Describe blit region to take from the above mip level and blit it's data to a region half the size on the below mip level
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        // Blit the mip level
        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // Transition the current mip level to shader read only
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        // Halve the dimensions of the mipmap ensuring that each dimension is at least 1
        if (mipWidth > 1)
        {
            mipWidth /= 2;
        }

        if (mipHeight > 1)
        {
            mipHeight /= 2;
        }
    }

    // Transition the last mip level to shader read only as the last mip level is not blitted from in the loop
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);
}

bool Renderer::Init(const glm::vec2& windowClientAreaResolution, HWND windowHandle)
{
    // Enable debug layers and extensions if being compiled in debug
#ifdef _DEBUG
    gDebugEnabled = EnableDebugLayersAndExtensions();
#endif // _DEBUG

    // Enable layers and extensions
    EnableLayersAndExtensions();

    // Create the vulkan instance
    if (!CreateInstance())
    {
        return false;
    }

    // Load debug report callback functions if debug is enabled
#ifdef _DEBUG
    if (gDebugEnabled)
    {
        if (!LoadDebugReportCallback(gInstance))
        {
            return false;
        }
    }
#endif // _DEBUG

    // Get the dedicated physical device with the highest vram
    gPhysicalDevice = GetPhysicalDevice(gMinUniformBufferOffsetAlignment, gPhysicalDeviceProperties, gPhysicalDeviceMemoryProperties, gPhysicalDeviceFeatures);

    // Check if the physical device is invalid
    if (gPhysicalDevice == VK_NULL_HANDLE)
    {
        return false;
    }

    // Get the physical device's queue family indices
    gQueueFamilyIndices = GetQueueFamilyIndices(gPhysicalDevice);

    // Check all queue family indices are valid
    if (!gQueueFamilyIndices.IsComplete())
    {
        LOG("Used physical device does not support all queue types.\n");
        return false;
    }

    // Get the graphics and transfer queue family indices
    auto graphicsQueueFamilyIndex = gQueueFamilyIndices.GetGraphicsFamilyIndex();
    auto transferQueueFamilyIndex = gQueueFamilyIndices.GetTransferFamilyIndex();


    // Enable sampler anisotropy feature if the physical device supports it
    VkPhysicalDeviceFeatures enabledFeatures{};
    if (gPhysicalDeviceFeatures.samplerAnisotropy)
    {
        enabledFeatures.samplerAnisotropy = VK_TRUE;
    }

    // Create the logical device
    if (!CreateLogicalDevice(gPhysicalDevice, enabledFeatures, graphicsQueueFamilyIndex, 1,
        gQueueFamilyIndices.GetComputeFamilyIndex(), 0, transferQueueFamilyIndex, 1))
    {
        return false;
    }

    // Create the win32 surface
    if (!CreateWin32Surface(gInstance, windowHandle, ::GetModuleHandle(nullptr), &gSurface))
    {
        return false;
    }

    // Check WSI extension is supported by the surface
    if (!QueryWSISupport(gPhysicalDevice, graphicsQueueFamilyIndex, gSurface))
    {
        return false;
    }

    // Get the surface capabilities
    if (!QuerySurfaceCapabilities(gPhysicalDevice, gSurface, &gSurfaceCapabilities, gSurfaceWidth, gSurfaceHeight))
    {
        return false;
    }

    // Get the surface format
    if (!QuerySurfaceFormat(graphicsQueueFamilyIndex, gPhysicalDevice, gSurface, &gSurfaceFormat))
    {
        return false;
    }
    
    // Create the swapchain with triple buffering. Use VK_PRESENT_MODE_MAILBOX_KHR to disable vsync. Use VK_PRESENT_MODE_FIFO_KHR to enable vsync
    if (!CreateSwapchain(
        gDevice,
        gPhysicalDevice, 
        gSwapchainImageCount, 
        VK_PRESENT_MODE_FIFO_KHR,
        gSurface,
        gSurfaceCapabilities, 
        gSurfaceFormat,
        gSurfaceWidth, 
        gSurfaceHeight,
        &gSwapchain))
    {
        return false;
    }

    // Get the number of swapchain images
    if (vkGetSwapchainImagesKHR(gDevice, gSwapchain, &gSwapchainImageCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    // Get swapchain images and create swapchain image views
    if (!GetSwapchainImagesAndCreateViews(
        gDevice,
        gSwapchain,
        gSwapchainImages,
        gSwapchainImageCount,
        gSwapchainImageViews,
        gSurfaceFormat.format))
    {
        return false;
    }

    // Create depth stencil image and image view
    if (!CreateDepthStencilImageAndView(gDevice, gPhysicalDevice, gPhysicalDeviceMemoryProperties,
        gSurfaceWidth, gSurfaceHeight, &gDepthStencilImage, &gDepthStencilImageView, &gDepthStencilImageMemory))
    {
        return false;
    }

    // Create the render pass
    if (!CreateRenderPass(gDevice, gSurfaceFormat.format, gDepthStencilFormat, &gRenderPass))
    {
        return false;
    }

    // Create framebuffers
    if (!CreateFramebuffers(gDevice,
        gSwapchainImageCount,
        gRenderPass,
        gSurfaceWidth,
        gSurfaceHeight,
        gSwapchainImageViews,
        gDepthStencilImageView,
        gFramebuffers))
    {
        return false;
    }

    // Get the graphics queue
    vkGetDeviceQueue(gDevice, graphicsQueueFamilyIndex, 0, &gGraphicsQueue);

    // Get the transfer queue
    vkGetDeviceQueue(gDevice, transferQueueFamilyIndex, 0, &gTransferQueue);

    // Create a graphics command pool
    if (!CreateCommandPool(gDevice, graphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &gGraphicsCommandPool))
    {
        return false;
    }

    // Allocate graphics command buffers in the graphics command pool
    gGraphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    if (!AllocateCommandBuffers(gDevice, gGraphicsCommandPool, MAX_FRAMES_IN_FLIGHT, gGraphicsCommandBuffers.data()))
    {
        return false;
    }

    // Create a transfer command pool for temporary transfer command buffers
    if (!CreateCommandPool(
        gDevice, 
        transferQueueFamilyIndex, 
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        &gTransferTemporaryCommandPool))
    {
        return false;
    }

    // Create fences
    gInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (!CreateFence(gDevice, &gInFlightFences[i]))
        {
            return false;
        }
    }

    // Initialise images in flight vector
    gImagesInFlight.resize(gSwapchainImageCount, VK_NULL_HANDLE);

    // Create semaphores
    gImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (!CreateSemaphore(gDevice, &gImageAvailableSemaphores[i]))
        {
            return false;
        }
    }

    gRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (!CreateSemaphore(gDevice, &gRenderFinishedSemaphores[i]))
        {
            return false;
        }
    }

    // Create per object uniform buffers for each frame
    gPerObjectUniformBuffers.resize(static_cast<size_t>(gSwapchainImageCount));
    gPerObjectUniformBuffersMemory.resize(static_cast<size_t>(gSwapchainImageCount));

    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (!CreateBuffer(
            gDevice,
            gPhysicalDevice,
            MAX_DRAW_ITEMS_PER_FRAME * gMinUniformBufferOffsetAlignment,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr,
            &gPerObjectUniformBuffers[i],
            &gPerObjectUniformBuffersMemory[i]))
        {
            return false;
        }
    }

    // Map per object uniform buffers
    gMappedPerObjectUniformBuffers.resize(static_cast<size_t>(gSwapchainImageCount));
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (vkMapMemory(gDevice, gPerObjectUniformBuffersMemory[i], 0, 
            gMinUniformBufferOffsetAlignment * MAX_DRAW_ITEMS_PER_FRAME, 0, &gMappedPerObjectUniformBuffers[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    // Create per frame uniform buffers for each frame
    gPerFrameUniformBuffers.resize(static_cast<size_t>(gSwapchainImageCount));
    gPerFrameUniformBuffersMemory.resize(static_cast<size_t>(gSwapchainImageCount));

    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (!CreateBuffer(
            gDevice,
            gPhysicalDevice,
            sizeof(PerFrameUniforms),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr,
            &gPerFrameUniformBuffers[i],
            &gPerFrameUniformBuffersMemory[i]))
        {
            return false;
        }
    }

    // Map per frame uniform buffers
    gMappedPerFrameUniformBuffers.resize(static_cast<size_t>(gSwapchainImageCount));
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (vkMapMemory(gDevice, gPerFrameUniformBuffersMemory[i], 0,
            sizeof(PerFrameUniforms), 0, &gMappedPerFrameUniformBuffers[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    // Create per render pass uniform buffers for each frame
    gPerRenderPassUniformBuffers.resize(static_cast<size_t>(gSwapchainImageCount));
    gPerRenderPassUniformBuffersMemory.resize(static_cast<size_t>(gSwapchainImageCount));

    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (!CreateBuffer(
            gDevice,
            gPhysicalDevice,
            MAX_RENDER_PASS_COUNT * gMinUniformBufferOffsetAlignment,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr,
            &gPerRenderPassUniformBuffers[i],
            &gPerRenderPassUniformBuffersMemory[i]))
        {
            return false;
        }
    }

    // Map per render pass uniform buffers
    gMappedPerRenderPassUniformBuffers.resize(static_cast<size_t>(gSwapchainImageCount));
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        if (vkMapMemory(gDevice, gPerRenderPassUniformBuffersMemory[i], 0,
            MAX_RENDER_PASS_COUNT * gMinUniformBufferOffsetAlignment, 0, &gMappedPerRenderPassUniformBuffers[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    // Shader binding ////////////////////////////////////////////////
    // Describe descriptor pool sizes
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = gSwapchainImageCount * UNIFORM_BUFFER_COUNT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[1].descriptorCount = SAMPLER_COUNT;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizes[2].descriptorCount = MAX_LOADED_TEXTURE_COUNT;

    // Describe uniform buffer descriptor pool create info
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = gSwapchainImageCount;

    // Create descriptor pool
    if (vkCreateDescriptorPool(gDevice, &poolInfo, nullptr, &gDescriptorPool) != VK_SUCCESS)
    {
        return false;
    }

    std::array<VkDescriptorSetLayoutBinding, 5> layoutBindings{};
    // Describe binding 0 - vertex shader per object uniform buffer
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Describe binding 1 - vertex shader per frame uniform buffer
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Describe binding 2 - vertex shader per render pass uniform buffer
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Describe binding 3 - fragment shader samplers
    layoutBindings[3].descriptorCount = SAMPLER_COUNT;
    layoutBindings[3].binding = 3;
    layoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Describe binding 4 - fragment shader textures
    layoutBindings[4].descriptorCount = MAX_LOADED_TEXTURE_COUNT;
    layoutBindings[4].binding = 4;
    layoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Describe binding flags
    VkDescriptorBindingFlags bindingFlag{};
    bindingFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
    bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.pBindingFlags = &bindingFlag;

    // Describe the descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = &bindingFlagsInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    // Create the descriptor set layout
    if (vkCreateDescriptorSetLayout(gDevice, &layoutInfo, nullptr, &gDescriptorSetLayout) != VK_SUCCESS)
    {
        return false;
    }

    // Update descriptor sets needs to be called once all images and samplers have been loaded before rendering
    // vkUpdateDescriptorSets is now called explicitly from outside the renderer

    // Create a copy of the descriptor set layout for each descriptor set in the pool
    std::vector<VkDescriptorSetLayout> layouts(gSwapchainImageCount, gDescriptorSetLayout);

    // Describe descriptor set allocate info
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = gDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    // Allocate descriptor sets in the descriptor pool
    gDescriptorSets.resize(static_cast<size_t>(gSwapchainImageCount)); // Using a seperate descriptor set for each swapchain image
    if (vkAllocateDescriptorSets(gDevice, &allocInfo, gDescriptorSets.data()) != VK_SUCCESS)
    {
        return false;
    }

    // Graphics pipeline ////////////////////////////////////////////////
    // Read in shader binary
    BinaryBuffer vertexShaderBinary{};
    if (!Binary::ReadBinaryIntoBuffer(gVertexShaderPath, vertexShaderBinary))
    {
        LOG("Failed to read vertex shader binary.");
        return false;
    }

    BinaryBuffer fragmentShaderBinary{};
    if (!Binary::ReadBinaryIntoBuffer(gFragmentShaderPath, fragmentShaderBinary))
    {
        LOG("Failed to read fragment shader binary.");
        return false;
    }

    // Create shader modules
    VkShaderModule vertexShaderModule;
    if (!CreateShaderModule(gDevice, vertexShaderBinary, &vertexShaderModule))
    {
        return false;
    }

    VkShaderModule fragmentShaderModule;
    if (!CreateShaderModule(gDevice, fragmentShaderBinary, &fragmentShaderModule))
    {
        return false;
    }

    // Describe vertex shader stage create info
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShaderModule;
    vertexShaderStageInfo.pName = "main";

    // Describe fragment shader stage create info
    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShaderModule;
    fragmentShaderStageInfo.pName = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

    // Describe input binding
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex1Pos1UV1Norm);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Describe attributes
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    // Position attribute
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    // UV attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(glm::vec3);

    // Vertex normal attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = sizeof(glm::vec3) + sizeof(glm::vec2);

    // Describe vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Describe the viewport
    gViewport.x = 0.0f;
    gViewport.y = 0.0f;
    gViewport.width = static_cast<float>(gSurfaceWidth);
    gViewport.height = static_cast<float>(gSurfaceHeight);
    gViewport.minDepth = 0.0f;
    gViewport.maxDepth = 1.0f;

    // Describe the scissor rect
    gScissor.offset = { 0, 0 };
    gScissor.extent = { gSurfaceWidth, gSurfaceHeight };

    // Describe viewport state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &gViewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &gScissor;

    // Describe rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    // Describe multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Describe depth stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;
    depthStencilState.stencilTestEnable = gStencilAvailable ? VK_FALSE : VK_FALSE; // depthStencilState.front and depthStencilState.back need to be configured before using the stencil buffer
    depthStencilState.front = {};
    depthStencilState.back = {};

    // Describe color blending state
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Describe dynamic states
    //const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    //VkPipelineDynamicStateCreateInfo dynamicState{};
    //dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicState.dynamicStateCount = _countof(dynamicStates);
    //dynamicState.pDynamicStates = dynamicStates;

    // Describe pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &gDescriptorSetLayout;

    // Create pipeline layout
    if (vkCreatePipelineLayout(gDevice, &pipelineLayoutInfo, nullptr, &gGraphicsPipelineLayout) != VK_SUCCESS)
    {
        return false;
    }

    // Describe graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = _countof(shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = gGraphicsPipelineLayout;
    pipelineInfo.renderPass = gRenderPass;
    pipelineInfo.subpass = 0;

    // Create graphics pipeline
    if (vkCreateGraphicsPipelines(gDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gGraphicsPipeline) != VK_SUCCESS)
    {
        return false;
    }

    // Destroy shader modules
    vkDestroyShaderModule(gDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(gDevice, fragmentShaderModule, nullptr);

    // Add available geometry IDs to queue
    for (uint32_t i = 0; i < MAX_LOADED_GEOMETRY_COUNT; ++i)
    {
        gAvailableGeometryIDs.push(i);
    }

    // Add available texture IDs to queue
    for (uint32_t i = 0; i < MAX_LOADED_TEXTURE_COUNT; ++i)
    {
        gAvailableTextureIDs.push(i);
    }

    // Create samplers
    // Linear filter sampler
    if (!CreateSampler(gDevice, VK_FILTER_LINEAR, &gSamplers[0]))
    {
        return false;
    }

    // Nearest neighbour filter sampler
    if (!CreateSampler(gDevice, VK_FILTER_NEAREST, &gSamplers[1]))
    {
        return false;
    }

    return true;
}

void Renderer::UpdateDescriptorSets()
{
    // Populate descriptor sets with descriptor info
    // Descriptor count in set = buffer version count * buffers in descriptor set
    // Need to write 9 descriptors for uniform buffers as there are 3 uniform buffers with 3 versions in the descriptor set
    // Different versions of each descriptor is needed for each descriptor set as buffer data the descriptor describes changes frame to frame
    std::vector<VkDescriptorBufferInfo> bufferInfos(static_cast<size_t>(gSwapchainImageCount) * UNIFORM_BUFFER_COUNT);
    // Sampler descriptors are the same across descriptor sets as the samplers are static and will not change during a frame
    std::vector<VkDescriptorImageInfo> samplerInfos(SAMPLER_COUNT);
    // Image descriptors are the same across descriptor sets as the textures are static and will not change during a frame
    std::vector<VkDescriptorImageInfo> imageInfos(MAX_LOADED_TEXTURE_COUNT);

    auto uniformBufferDescriptorWriteCount = static_cast<size_t>(gSwapchainImageCount) * UNIFORM_BUFFER_COUNT;
    auto samplerDescriptorWriteCount = static_cast<size_t>(gSwapchainImageCount);
    auto textureDescriptorWriteCount = static_cast<size_t>(gSwapchainImageCount);

    std::vector<VkWriteDescriptorSet> descriptorWrites(
        uniformBufferDescriptorWriteCount +
        samplerDescriptorWriteCount +
        textureDescriptorWriteCount
    );

    // Populate per object uniform buffer descriptors in each descriptor set
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        auto& bufferInfo = bufferInfos[i];
        bufferInfo.buffer = gPerObjectUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = gMinUniformBufferOffsetAlignment;

        auto& descriptorWrite = descriptorWrites[i];
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = gDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
    }

    // Populate per frame uniform buffer descriptors
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        auto& bufferInfo = bufferInfos[static_cast<size_t>(i) + gSwapchainImageCount];
        bufferInfo.buffer = gPerFrameUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(PerFrameUniforms);

        auto& descriptorWrite = descriptorWrites[static_cast<size_t>(i) + gSwapchainImageCount];
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = gDescriptorSets[i];
        descriptorWrite.dstBinding = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
    }

    // Populate per render pass buffer descriptors
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        auto& bufferInfo = bufferInfos[static_cast<size_t>(i) + (static_cast<size_t>(gSwapchainImageCount) * 2)];
        bufferInfo.buffer = gPerRenderPassUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = gMinUniformBufferOffsetAlignment;

        auto& descriptorWrite = descriptorWrites[static_cast<size_t>(i) + (static_cast<size_t>(gSwapchainImageCount) * 2)];
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = gDescriptorSets[i];
        descriptorWrite.dstBinding = 2;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
    }

    // Describe descriptor for each sampler
    for (uint32_t i = 0; i < SAMPLER_COUNT; ++i)
    {
        auto& samplerInfo = samplerInfos[i];
        samplerInfo.sampler = gSamplers[i];
    }

    // Populate sampler descriptors in each descriptor set
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        auto& descriptorWrite = descriptorWrites[static_cast<size_t>(i) + uniformBufferDescriptorWriteCount];
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = gDescriptorSets[i];
        descriptorWrite.dstBinding = 3;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.descriptorCount = SAMPLER_COUNT;
        descriptorWrite.pImageInfo = samplerInfos.data();
    }

    // Describe descriptor for each texture
    for (uint32_t i = 0; i < MAX_LOADED_TEXTURE_COUNT; ++i)
    {
        auto& imageInfo = imageInfos[i];
        imageInfo.sampler = nullptr;
        imageInfo.imageView = gLoadedTextures[i].GetImageView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // Populate texture descriptors in each descriptor set
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        auto& descriptorWrite = descriptorWrites[static_cast<size_t>(i) + uniformBufferDescriptorWriteCount + samplerDescriptorWriteCount];
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = gDescriptorSets[i];
        descriptorWrite.dstBinding = 4;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = MAX_LOADED_TEXTURE_COUNT;
        descriptorWrite.pImageInfo = imageInfos.data();
    }

    vkUpdateDescriptorSets(gDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

bool Renderer::Shutdown()
{
    // Wait for all queues to finish work
    if (vkQueueWaitIdle(gGraphicsQueue) != VK_SUCCESS && vkQueueWaitIdle(gTransferQueue) != VK_SUCCESS)
    {
        return false;
    }

    // Unmap per object, per render pass and per frame uniform buffers
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        vkUnmapMemory(gDevice, gPerObjectUniformBuffersMemory[i]);
    }
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        vkUnmapMemory(gDevice, gPerRenderPassUniformBuffersMemory[i]);
    }
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        vkUnmapMemory(gDevice, gPerFrameUniformBuffersMemory[i]);
    }

    // Destroy remaining loaded geometry
    for(const auto& id : gUsedGeometryIDs)
    {
        // Get the geometry instance
        auto& geometry = gLoadedGeometry[id];

        // Destroy the geometry buffers
        DestroyBuffer(geometry.GetVertexBuffer(), geometry.GetVertexBufferMemory());
        DestroyBuffer(geometry.GetIndexBuffer(), geometry.GetIndexBufferMemory());
    }

    // Destroy remaining loaded textures
    for (const auto& id : gUsedTextureIDs)
    {
        // Get the texture instance
        auto& texture = gLoadedTextures[id];

        // Destroy the texture images
        DestroyImage(texture.GetImage(), texture.GetImageMemory());
        vkDestroyImageView(gDevice, texture.GetImageView(), nullptr);
    }

    // Destroy samplers
    for (uint32_t i = 0; i < SAMPLER_COUNT; ++i)
    {
        vkDestroySampler(gDevice, gSamplers[i], nullptr);
    }

    // Destroy descriptor set layout
    vkDestroyDescriptorSetLayout(gDevice, gDescriptorSetLayout, nullptr);

    // Destroy pipeline
    vkDestroyPipeline(gDevice, gGraphicsPipeline, nullptr);

    // Destroy pipeline layout
    vkDestroyPipelineLayout(gDevice, gGraphicsPipelineLayout, nullptr);

    // Destroy semaphores
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(gDevice, gImageAvailableSemaphores[i], nullptr);
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(gDevice, gRenderFinishedSemaphores[i], nullptr);
    }

    // Destroy fences
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyFence(gDevice, gInFlightFences[i], nullptr);
    }

    // Destroy framebuffers
    for (auto& framebuffer : gFramebuffers)
    {
        vkDestroyFramebuffer(gDevice, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }

    // Destroy render pass
    vkDestroyRenderPass(gDevice, gRenderPass, nullptr);
    gRenderPass = VK_NULL_HANDLE;

    // Destroy depth stencil image, image view and free memory
    vkDestroyImageView(gDevice, gDepthStencilImageView, nullptr);
    vkFreeMemory(gDevice, gDepthStencilImageMemory, nullptr);
    vkDestroyImage(gDevice, gDepthStencilImage, nullptr);

    // Destroy swapchain image views
    for (auto view : gSwapchainImageViews)
    {
        vkDestroyImageView(gDevice, view, nullptr);
        view = VK_NULL_HANDLE;
    }

    // Destroy descriptor pool
    vkDestroyDescriptorPool(gDevice, gDescriptorPool, nullptr);

    // Destroy per frame uniform buffers
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        DestroyBuffer(gPerFrameUniformBuffers[i], gPerFrameUniformBuffersMemory[i]);
    }

    // Destroy per render pass bufers
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        DestroyBuffer(gPerRenderPassUniformBuffers[i], gPerRenderPassUniformBuffersMemory[i]);
    }

    // Destroy per object uniform buffers
    for (uint32_t i = 0; i < gSwapchainImageCount; ++i)
    {
        DestroyBuffer(gPerObjectUniformBuffers[i], gPerObjectUniformBuffersMemory[i]);
    }

    // Destroy the swapchain
    vkDestroySwapchainKHR(gDevice, gSwapchain, nullptr);
    gSwapchain = VK_NULL_HANDLE;

    // Destroy the surface
    vkDestroySurfaceKHR(gInstance, gSurface, nullptr);
    gSurface = VK_NULL_HANDLE;

    // Destroy transfer comand pool
    vkDestroyCommandPool(gDevice, gTransferTemporaryCommandPool, nullptr);
    gTransferTemporaryCommandPool = VK_NULL_HANDLE;

    // Destroy the graphics command pool
    vkDestroyCommandPool(gDevice, gGraphicsCommandPool, nullptr);
    gGraphicsCommandPool = VK_NULL_HANDLE;

    // Destroy the logical device
    vkDestroyDevice(gDevice, nullptr);
    gDevice = VK_NULL_HANDLE;

    // If debug is enabled
#ifdef _DEBUG
    if (gDebugEnabled)
    {
        // Destroy debug report callback
        fvkDestroyDebugReportCallbackEXT(gInstance, gDebugReport, nullptr);
        gDebugReport = VK_NULL_HANDLE;
        gDebugEnabled = false;
    }
#endif // _DEBUG

    // Destroy the vulkan instance
    //vkDestroyInstance(gInstance, nullptr); // Commented out as the program crashes when destroying the instance when using a more modern vulkan version.
    gInstance = VK_NULL_HANDLE;

    // Invalidate the physical device
    gPhysicalDevice = VK_NULL_HANDLE;

    return true;
}

void Renderer::SetVulkanDebugReportLevel(const EVulkanDebugReportLevel level)
{
    gDebugReportLevel = level;
}

bool Renderer::BeginFrame(const Renderer::DirectionalLight& directionalLight)
{
    // Wait for the previous frame to finish executing on the GPU
    if (vkWaitForFences(gDevice, 1, &gInFlightFences[gCurrentFrame], VK_TRUE, MAX_SYNCHRONIZATION_TIMEOUT_DURATION) != VK_SUCCESS)
    {
        return false;
    }

    // Get the next available swapchain image
    if (vkAcquireNextImageKHR(gDevice,
        gSwapchain,
        std::chrono::nanoseconds::max().count(),
        gImageAvailableSemaphores[gCurrentFrame],
        VK_NULL_HANDLE,
        &gImageIndex) != VK_SUCCESS)
    {
        return false;
    }

    // Check if a previous frame is using the current image
    if (gImagesInFlight[gImageIndex] != VK_NULL_HANDLE)
    {
        // Wait for the GPU to signal the fence it is finished writing to the image
        vkWaitForFences(gDevice, 1, &gImagesInFlight[gImageIndex], VK_TRUE, MAX_SYNCHRONIZATION_TIMEOUT_DURATION);
    }
    // Set the image as in use by the current frame
    gImagesInFlight[gImageIndex] = gInFlightFences[gCurrentFrame];

    // Describe command buffer begin info 
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Begin recording command buffer
    gCurrentFrameCommandBuffer = gGraphicsCommandBuffers[gCurrentFrame];
    if (vkBeginCommandBuffer(gCurrentFrameCommandBuffer, &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    // Describe render area
    VkRect2D renderArea{};
    renderArea.offset.x = 0;
    renderArea.offset.y = 0;
    renderArea.extent.width = gSurfaceWidth;
    renderArea.extent.height = gSurfaceHeight;

    // Describe attachment clear values
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color.float32[0] = CLEAR_COLOR.r;
    clearValues[0].color.float32[1] = CLEAR_COLOR.g;
    clearValues[0].color.float32[2] = CLEAR_COLOR.b;
    clearValues[0].color.float32[3] = CLEAR_COLOR.a;

    clearValues[1].depthStencil = { 1.0f, 0 };

    // Describe render pass begin info
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = gRenderPass;
    renderPassBeginInfo.framebuffer = gFramebuffers[gImageIndex];
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    // Begin render pass
    vkCmdBeginRenderPass(gCurrentFrameCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind graphics pipeline
    vkCmdBindPipeline(gCurrentFrameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gGraphicsPipeline);

    // Update per frame uniforms with this frame's data
    PerFrameUniforms perFrameUniforms{};

    auto directionLightColorIntensity = directionalLight.GetColor() * directionalLight.GetIntensity();
    perFrameUniforms.DirectionalLightColor = glm::vec4(directionLightColorIntensity.r, directionLightColorIntensity.g, directionLightColorIntensity.b, 1.0f);
    perFrameUniforms.DirectionalLightWorldSpaceDirection = glm::vec4(directionalLight.GetDirection(), 0.0f);

    // Copy per frame uniform buffer
    memcpy(gMappedPerFrameUniformBuffers[gCurrentFrame], &perFrameUniforms, sizeof(PerFrameUniforms));

    return true;
}

void Renderer::BeginRenderPass(const glm::vec3& viewPosition,
    const glm::vec3& viewRotation,
    const Renderer::CameraSettings& cameraSettings)
{
    assert(gRenderPassCount < MAX_RENDER_PASS_COUNT && "An unsupported amount of render passes are begun this frame.");

    PerRenderPassUniforms perRenderPassUniforms{};

    perRenderPassUniforms.ViewMatrix = Maths::CalculateViewMatrix(viewPosition, viewRotation);

    if (cameraSettings.ProjectionMode == Renderer::EProjectionMode::PERSPECTIVE)
    {
        perRenderPassUniforms.ProjectionMatrix = Maths::CalculatePerspectiveProjectionMatrix(
            cameraSettings.PerspectiveFOV,
            gViewport.width,
            gViewport.height,
            cameraSettings.PerspectiveNearClipPlane,
            cameraSettings.PerspectiveFarClipPlane);
    }
    else
    {
        perRenderPassUniforms.ProjectionMatrix = Maths::CalculateOrthographicProjectionMatrix(
            cameraSettings.AspectWidth * cameraSettings.OrthographicWidth,
            cameraSettings.AspectHeight * cameraSettings.OrthographicWidth,
            cameraSettings.OrthographicNearClipPlane,
            cameraSettings.OrthographicFarClipPlane);
    }

    perRenderPassUniforms.CameraWorldSpacePosition = glm::vec4(viewPosition.x, viewPosition.y, viewPosition.z, 1.0f);

    // Copy per frame uniform buffer
    memcpy(static_cast<uint8_t*>(gMappedPerRenderPassUniformBuffers[gCurrentFrame]) + (static_cast<uint64_t>(gRenderPassCount) * gMinUniformBufferOffsetAlignment), 
        &perRenderPassUniforms, sizeof(perRenderPassUniforms));

    // Set dynamic offset for the per render pass uniform
    gDynamicOffsets[PER_RENDER_PASS_UNIFORMS_DYNAMIC_OFFSET_INDEX] = gRenderPassCount * static_cast<uint32_t>(gMinUniformBufferOffsetAlignment);

    ++gRenderPassCount;
}

bool Renderer::EndFrame()
{
    // End render pass
    vkCmdEndRenderPass(gCurrentFrameCommandBuffer);

    // End recording command buffer
    if (vkEndCommandBuffer(gCurrentFrameCommandBuffer) != VK_SUCCESS)
    {
        return false;
    }

    // Get the current in flight frame's wait and signal semaphores
    VkSemaphore waitSemaphores[] = { gImageAvailableSemaphores[gCurrentFrame] };
    VkSemaphore signalSemaphores[] = { gRenderFinishedSemaphores[gCurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    // Describe submit info
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = _countof(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &gCurrentFrameCommandBuffer;
    submitInfo.signalSemaphoreCount = _countof(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the current frame's fence
    if (vkResetFences(gDevice, 1, &gInFlightFences[gCurrentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    // Submit command buffer signalling the current frame's fence
    if (vkQueueSubmit(gGraphicsQueue, 1, &submitInfo, gInFlightFences[gCurrentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    // Describe present info
    VkResult presentResult = VkResult::VK_RESULT_MAX_ENUM;
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = _countof(signalSemaphores);
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &gSwapchain;
    presentInfo.pImageIndices = &gImageIndex;
    presentInfo.pResults = &presentResult;

    // Queue the current frame's image for presentation
    if (vkQueuePresentKHR(gGraphicsQueue, &presentInfo) != VK_SUCCESS)
    {
        return false;
    }

    // Check the present result for errors
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    gCurrentFrame = (gCurrentFrame + 1) % gSwapchainImageCount;
    gDrawItemSubmitCount = 0;
    gRenderPassCount = 0;

    return true;
}

bool Renderer::Submit(
    const Renderer::DrawItem* drawItems,
    uint32_t drawItemCount
)
{
    assert(gDrawItemSubmitCount < MAX_DRAW_ITEMS_PER_FRAME && "Unsupported number of draw items submitted to the renderer this frame.");

    // Update per object uniforms with this frame's submitted draw items
    PerObjectUniforms perObjectUniforms{};

    for (uint32_t i = 0; i < drawItemCount; ++i)
    {
        const auto& drawItem = drawItems[i];
        perObjectUniforms.WorldMatrix = drawItem.GetWorldMatrix();

        glm::mat3 worldMatrix3x3 = drawItem.GetWorldMatrix();
        perObjectUniforms.NormalMatrix = glm::inverse(glm::transpose(worldMatrix3x3));

        perObjectUniforms.SamplerID = drawItem.GetSamplerID();
        perObjectUniforms.TextureID = drawItem.GetTextureID();
        const auto& textureScale = drawItem.GetTextureScale();
        perObjectUniforms.Data1.r = textureScale.r;
        perObjectUniforms.Data1.g = textureScale.g;

        // Copy per object uniform buffer. An object's world matrix is stored as 64 bytes in 256 byte contiguous chunks
        // This is due to the GPU's (Nvidia GTX 1080) minUniformBufferOffsetAlignment requiring 256 byte offsets
        memcpy(static_cast<uint8_t*>(gMappedPerObjectUniformBuffers[gCurrentFrame]) + ((static_cast<uint64_t>(i) + gDrawItemSubmitCount) * gMinUniformBufferOffsetAlignment), 
            &perObjectUniforms, sizeof(perObjectUniforms));
    }

    // For each submitted draw item
    for (uint32_t i = 0; i < drawItemCount; ++i)
    {
        assert(drawItems[i].GetGeometryID() < MAX_LOADED_GEOMETRY_COUNT && "Draw item geometry ID is invalid.");

        // Set dynamic offset for the per object uniform buffer
        gDynamicOffsets[PER_OBJECT_UNIFORMS_DYNAMIC_OFFSET_INDEX] = (i + gDrawItemSubmitCount) * static_cast<uint32_t>(gMinUniformBufferOffsetAlignment);

        // Bind descriptor set for the current frame
        vkCmdBindDescriptorSets(gCurrentFrameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gGraphicsPipelineLayout, 
            0, 1, &gDescriptorSets[gCurrentFrame],
            static_cast<uint32_t>(gDynamicOffsets.size()), gDynamicOffsets.data());

        // Get geometry instance from draw item
        auto& geometry = gLoadedGeometry[drawItems[i].GetGeometryID()];

        // Bind vertex buffers
        const VkBuffer vertexBuffers[] = { geometry.GetVertexBuffer() };
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(gCurrentFrameCommandBuffer, 0, _countof(vertexBuffers), vertexBuffers, offsets);

        // Bind index buffer
        vkCmdBindIndexBuffer(gCurrentFrameCommandBuffer, geometry.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Draw
        vkCmdDrawIndexed(gCurrentFrameCommandBuffer, geometry.GetIndexCount(), 1, 0, 0, 0);
    }

    gDrawItemSubmitCount += drawItemCount;

    return true;
}

bool Renderer::SubmitLevel(Level& level)
{
    // Get the level's ecs registry
    auto& ecsRegistry = level.GetECSRegistry();

    // Sort static meshes so that meshes with an alpha blended material are drawn last
    ecsRegistry.sort<StaticMeshComponent>([](const auto& lhs, const auto& rhs)
        {
            return lhs.Material.AlphaBlended < rhs.Material.AlphaBlended;
        });

    // TODO Sort static meshes with an alpha blended material based on distance from the camera

    // Create a view of entities that contain a transform and static mesh component
    auto renderableView = ecsRegistry.view<TransformComponent, StaticMeshComponent>();
    
    // Build draw items vector
    std::vector<Renderer::DrawItem> drawItems;
    drawItems.reserve(static_cast<size_t>(renderableView.size_hint()));

    // For each entity in the view
    for (auto [renderableEntity, renderableTransform, renderableStaticMesh] : renderableView.each())
    {
        // Check if the static mesh component is not set to be visible
        if (!renderableStaticMesh.Visible)
        {
            // Skip this entity
            continue;
        }

        // Construct the draw item
        drawItems.emplace_back(
            renderableStaticMesh.GeometryID,
            static_cast<Renderer::ESampler>(renderableStaticMesh.Material.SamplerID),
            renderableStaticMesh.Material.TextureID,
            renderableStaticMesh.Material.TextureScale,
            Maths::CalculateWorldMatrix(renderableTransform.Transform)
        );
    }

    // Submit draw items
    return Submit(
        drawItems.data(),
        static_cast<uint32_t>(drawItems.size())
    );
}

bool Renderer::SubmitHUD(HUD& hud)
{
    // Get the HUD's ecs registry
    auto& ecsRegistry = hud.GetECSRegistry();

    // Sort static meshes so that meshes with an alpha blended material are drawn last
    ecsRegistry.sort<StaticMeshComponent>([](const auto& lhs, const auto& rhs)
        {
            return lhs.Material.AlphaBlended < rhs.Material.AlphaBlended;
        });

    // Create a view of entities that contain a transform and static mesh component
    auto renderableView = ecsRegistry.view<TransformComponent, StaticMeshComponent>();

    // Build draw items vector
    std::vector<Renderer::DrawItem> drawItems;
    drawItems.reserve(static_cast<size_t>(renderableView.size_hint()));

    // For each entity in the view
    for (auto [renderableEntity, renderableTransform, renderableStaticMesh] : renderableView.each())
    {
        // Check if the static mesh component is not set to be visible
        if (!renderableStaticMesh.Visible)
        {
            // Skip this entity
            continue;
        }

        // Construct the draw item
        drawItems.emplace_back(
            renderableStaticMesh.GeometryID,
            static_cast<Renderer::ESampler>(renderableStaticMesh.Material.SamplerID),
            renderableStaticMesh.Material.TextureID,
            renderableStaticMesh.Material.TextureScale,
            Maths::CalculateWorldMatrix(renderableTransform.Transform)
        );
    }

    // Submit draw items
    return Submit(
        drawItems.data(),
        static_cast<uint32_t>(drawItems.size())
    );

    return true;
}

bool Renderer::LoadGeometry(const Vertex1Pos1UV1Norm* vertices, const uint32_t vertexCount, const uint32_t* indices, const uint32_t indexCount, uint32_t* pID)
{
    // Get the queue family indices that will share access to buffer resources
    const uint32_t sharedAccessQueueFamilyIndices[] = { gQueueFamilyIndices.GetGraphicsFamilyIndex(), gQueueFamilyIndices.GetTransferFamilyIndex() };

    // Calculate buffer sizes
    const auto vertexBufferSize = sizeof(Vertex1Pos1UV1Norm) * vertexCount;
    const auto indexBufferSize = sizeof(uint32_t) * indexCount;

    // Assert max loaded geometry count has not been reached
    assert(!gAvailableGeometryIDs.empty() && "Max loaded geometry count reached.");

    // Get an available geometry instance
    *pID = gAvailableGeometryIDs.front();
    gAvailableGeometryIDs.pop();
    auto& geometry = gLoadedGeometry[*pID];
    gUsedGeometryIDs.push_back(*pID);

    // Create a vertex buffer
    // Create CPU visible staging buffer for vertex data
    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory;
    if (!CreateBuffer(
        gDevice,
        gPhysicalDevice,
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        &vertexStagingBuffer,
        &vertexStagingBufferMemory))
    {
        return false;
    }

    // Map staging buffer GPU memory
    void* pData;
    if (vkMapMemory(gDevice, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &pData) != VK_SUCCESS)
    {
        return false;
    }

    // Upload vertex data to the staging buffer
    memcpy(pData, vertices, static_cast<size_t>(vertexBufferSize));

    // Unmap GPU memory
    vkUnmapMemory(gDevice, vertexStagingBufferMemory);

    // Create GPU only vertex buffer
    if (!CreateBuffer(
        gDevice,
        gPhysicalDevice,
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_SHARING_MODE_CONCURRENT,
        _countof(sharedAccessQueueFamilyIndices),
        sharedAccessQueueFamilyIndices,
        &geometry.GetVertexBuffer(),
        &geometry.GetVertexBufferMemory()))
    {
        return false;
    }

    // Create index buffer
    // Create CPU visible staging buffer
    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;
    if (!CreateBuffer(gDevice,
        gPhysicalDevice,
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        &indexStagingBuffer,
        &indexStagingBufferMemory))
    {
        return false;
    }

    // Map staging buffer GPU memory
    if (vkMapMemory(gDevice, indexStagingBufferMemory, 0, indexBufferSize, 0, &pData) != VK_SUCCESS)
    {
        return false;
    }

    // Upload vertex data to the staging buffer
    memcpy(pData, indices, static_cast<size_t>(indexBufferSize));

    // Create GPU only index buffer
    if (!CreateBuffer(
        gDevice,
        gPhysicalDevice,
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_SHARING_MODE_CONCURRENT,
        _countof(sharedAccessQueueFamilyIndices),
        sharedAccessQueueFamilyIndices,
        &geometry.GetIndexBuffer(),
        &geometry.GetIndexBufferMemory()))
    {
        return false;
    }

    // Set index count
    geometry.SetIndexCount(indexCount);

    // Begin single submit command buffer
    VkCommandBuffer commandBuffer;
    if (!BeginSingleSubmitCommandBuffer(gDevice, gTransferTemporaryCommandPool, &commandBuffer))
    {
        return false;
    }

    // Copy vertex staging buffer to the vertex buffer
    CopyBuffer(commandBuffer, vertexStagingBuffer, geometry.GetVertexBuffer(), vertexBufferSize);

    // Copy index staging buffer to the index buffer
    CopyBuffer(commandBuffer, indexStagingBuffer, geometry.GetIndexBuffer(), indexBufferSize);

    // End single submit command buffer
    if (!EndAndSubmitSingleSubmitCommandBuffer(gDevice, gTransferTemporaryCommandPool, gTransferQueue, commandBuffer))
    {
        return false;
    }

    // Delete staging buffer and memory
    DestroyBuffer(vertexStagingBuffer, vertexStagingBufferMemory);
    DestroyBuffer(indexStagingBuffer, indexStagingBufferMemory);

    return true;
}

bool Renderer::LoadPlaneGeometryPrimitive(const float width, uint32_t* pID)
{
    const float halfWidth = width / 2.0f;

    const std::array<Renderer::Vertex1Pos1UV1Norm, 4> planeVertices = {
        Vertex1Pos1UV1Norm(glm::vec3(-halfWidth, -halfWidth, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        Vertex1Pos1UV1Norm(glm::vec3(halfWidth, -halfWidth, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        Vertex1Pos1UV1Norm(glm::vec3(halfWidth, halfWidth, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        Vertex1Pos1UV1Norm(glm::vec3(-halfWidth, halfWidth, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f))
    };

    const std::array<uint32_t, 6> planeIndices = {
        0, 1, 2, 2, 3, 0
    };

    return Renderer::LoadGeometry(planeVertices.data(), static_cast<uint32_t>(planeVertices.size()),
        planeIndices.data(), static_cast<uint32_t>(planeIndices.size()),
        pID);
}

bool Renderer::LoadCubeGeometryPrimitive(const float width, uint32_t* pID)
{
    const float halfWidth = width / 2.0f;

    const std::array<Renderer::Vertex1Pos1UV1Norm, 24> cubeVertices{
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f )),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    const std::array<uint32_t, 36> cubeIndices = {
        13,18,23,13,23,11,9,21,15,21,17,15,12,14,19,14,16,19,7,6,22,6,20,22,5,4,10,4,8,10,3,2,0,2,1,0
    };

    return Renderer::LoadGeometry(cubeVertices.data(), static_cast<uint32_t>(cubeVertices.size()),
        cubeIndices.data(), static_cast<uint32_t>(cubeIndices.size()),
        pID);
}

bool Renderer::LoadSphereGeometryPrimitive(const float radius, const int32_t sectors, const int32_t stacks, uint32_t* pID)
{
    constexpr float PI = glm::pi<float>();

    float sectorStep = 2.f * PI / static_cast<float>(sectors);
    float stackStep = PI / stacks;
    float sectorAngle = 0.f;
    float stackAngle = 0.f;

    float xy = 0.f;
    float lengthInv = 1.0f / radius;

    std::vector<Renderer::Vertex1Pos1UV1Norm> sphereVertices;

    for (int i = stacks; i >= 0; i--)
    {
        stackAngle = PI / 2 - i * stackStep;
        xy = radius * glm::cos(stackAngle);

        for (int j = 0; j <= sectors; j++)
        {
            Renderer::Vertex1Pos1UV1Norm vert;

            sectorAngle = j * sectorStep;

            vert.Pos.x = xy * glm::cos(sectorAngle);
            vert.Pos.y = xy * glm::sin(sectorAngle);
            vert.Pos.z = radius * glm::sin(stackAngle);

            vert.UV.x = static_cast<float>(j) / sectors;
            vert.UV.y = static_cast<float>(i) / stacks;

            vert.Norm.x = vert.Pos.x * lengthInv;
            vert.Norm.y = vert.Pos.y * lengthInv;
            vert.Norm.z = vert.Pos.z * lengthInv;

            sphereVertices.push_back(vert);
        }
    }

    std::vector<uint32_t> sphereIndices;

    unsigned int k1 = 0;
    unsigned int k2 = 0;

    for (int i = 0; i < stacks; i++)
    {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; j++, k1++, k2++)
        {
            if (i != 0)
            {
                sphereIndices.push_back(k1);
                sphereIndices.push_back(k2);
                sphereIndices.push_back(k1 + 1);
            }

            if (i != (stacks - 1))
            {
                sphereIndices.push_back(k1 + 1);
                sphereIndices.push_back(k2);
                sphereIndices.push_back(k2 + 1);
            }
        }
    }

    return Renderer::LoadGeometry(sphereVertices.data(), static_cast<uint32_t>(sphereVertices.size()), sphereIndices.data(), static_cast<uint32_t>(sphereIndices.size()), pID);
}

bool Renderer::LoadCylinderGeometryPrimitive(const float baseRadius, const float topRadius, const float height, const int32_t sectors, const int32_t stacks, uint32_t* pID)
{
    // Vertices.
    std::vector<Renderer::Vertex1Pos1UV1Norm> cylinderVertices;

    // Side.
    float z = 0.f;
    float radius = 0.f;

    // Get normals for cylinder sides.
    std::vector<float> sideNormals;
    float sectorStep = glm::two_pi<float>() / sectors;
    float sectorAngle = 0.f;

    float zAngle = atan2(baseRadius - topRadius, height);
    float x0 = cos(zAngle);
    float y0 = 0;
    float z0 = sin(zAngle);

    for (int32_t i = 0; i <= sectors; ++i)
    {
        sectorAngle = i * sectorStep;
        sideNormals.push_back(cos(sectorAngle) * x0 - sin(sectorAngle) * y0);
        sideNormals.push_back(sin(sectorAngle) * x0 + cos(sectorAngle) * y0);
        sideNormals.push_back(z0);
    }

    // Get unit circle vertices.
    std::vector<float> unitCircleVertices;

    for (int i = 0; i <= sectors; ++i)
    {
        sectorAngle = i * sectorStep;
        unitCircleVertices.push_back(cos(sectorAngle));
        unitCircleVertices.push_back(sin(sectorAngle));
        unitCircleVertices.push_back(0);
    }

    for (int i = stacks; i >= 0; i--)
    {
        z = -(height * 0.5f) + (float)i / stacks * height;
        radius = baseRadius + (float)i / stacks * (topRadius - baseRadius);
        float t = 1.0f - (float)i / stacks;

        for (int32_t j = 0, k = 0; j <= sectors; ++j, k += 3)
        {
            Renderer::Vertex1Pos1UV1Norm vert;
            vert.Pos = glm::vec3(unitCircleVertices[k] * radius, unitCircleVertices[static_cast<size_t>(k) + 1] * radius, z);
            vert.UV = glm::vec2(static_cast<float>(j) / sectors, t);
            vert.Norm = glm::vec3(sideNormals[static_cast<size_t>(k)], sideNormals[static_cast<size_t>(k) + 1], sideNormals[static_cast<size_t>(k) + 2]);
            cylinderVertices.push_back(vert);
        }
    }

    int baseCapStart = (int)cylinderVertices.size();

    // Base cap.
    int numberOfCapVertices = sectors + 2;

    glm::vec3 capCenter = glm::vec3(0.f, 0.f, -height / 2.f);

    for (int i = 0; i < numberOfCapVertices; i++)
    {
        Renderer::Vertex1Pos1UV1Norm v;
        v.Pos.x = capCenter.x + (baseRadius * glm::cos((float)i * glm::two_pi<float>() / (float)sectors));
        v.Pos.y = capCenter.y + (baseRadius * glm::sin((float)i * glm::two_pi<float>() / (float)sectors));
        v.Pos.z = capCenter.z;
        v.UV.x = 0.5f - (v.Pos.x) / (2.f * baseRadius);
        v.UV.y = 0.5f - (v.Pos.y) / (2.f * baseRadius);
        v.Norm = { 0.0f, 0.0f, -1.0f };
        cylinderVertices.push_back(v);
    }

    {
        Renderer::Vertex1Pos1UV1Norm v;
        v.Pos.x = capCenter.x;
        v.Pos.y = capCenter.y;
        v.Pos.z = capCenter.z;
        v.UV.x = 0.5f;
        v.UV.y = 0.5f;
        v.Norm = { 0.0f, 0.0f, -1.0f };
        cylinderVertices.push_back(v);
    }

    int topCapStart = (int)cylinderVertices.size();

    // Top cap.
    capCenter = glm::vec3(0.f, 0.f, height / 2.f);

    for (int i = numberOfCapVertices; i > 0; i--)
    {
        Renderer::Vertex1Pos1UV1Norm v;
        v.Pos.x = capCenter.x + (topRadius * glm::cos((float)i * glm::two_pi<float>() / (float)sectors));
        v.Pos.y = capCenter.y + (topRadius * glm::sin((float)i * glm::two_pi<float>() / (float)sectors));
        v.Pos.z = capCenter.z;
        v.UV.x = 0.5f + (v.Pos.x) / (2.f * topRadius);
        v.UV.y = 0.5f + (v.Pos.y) / (2.f * topRadius);
        v.Norm = { 0.0f, 0.0f, 1.0f };
        cylinderVertices.push_back(v);
    }

    {
        Renderer::Vertex1Pos1UV1Norm v;
        v.Pos.x = capCenter.x;
        v.Pos.y = capCenter.y;
        v.Pos.z = capCenter.z;
        v.UV.x = 0.5f;
        v.UV.y = 0.5f;
        v.Norm = { 0.0f, 0.0f, 1.0f };
        cylinderVertices.push_back(v);
    }

    // Indices.
    std::vector<uint32_t> cylinderIndices;

    // Base cap.
    for (int32_t i = baseCapStart + 2; i < baseCapStart + sectors; i++)
    {
        cylinderIndices.push_back(baseCapStart);
        cylinderIndices.push_back(i - 1);
        cylinderIndices.push_back(i);
    }

    // Top cap.
    for (int32_t i = topCapStart + 2; i < topCapStart + sectors; i++)
    {
        cylinderIndices.push_back(topCapStart);
        cylinderIndices.push_back(i - 1);
        cylinderIndices.push_back(i);
    }

    // Side.
    unsigned int k1, k2;
    for (int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            cylinderIndices.push_back(k1);
            cylinderIndices.push_back(k1 + 1);
            cylinderIndices.push_back(k2);
            cylinderIndices.push_back(k2);
            cylinderIndices.push_back(k1 + 1);
            cylinderIndices.push_back(k2 + 1);
        }
    }

    return Renderer::LoadGeometry(cylinderVertices.data(), static_cast<uint32_t>(cylinderVertices.size()),
        cylinderIndices.data(), static_cast<uint32_t>(cylinderIndices.size()),
        pID);
}

bool Renderer::LoadConeGeometryPrimitive(const float baseRadius, const float height, const int32_t sectors, const int32_t stacks, uint32_t* pID)
{
    // Vertices.
    std::vector<Renderer::Vertex1Pos1UV1Norm> coneVertices;

    // Side.
    float z = 0.f;
    float radius = 0.f;

    // Get normals for cylinder sides.
    std::vector<float> sideNormals;
    float sectorStep = glm::two_pi<float>() / sectors;
    float sectorAngle = 0.f;

    float zAngle = atan2(baseRadius - 0.f, height);
    float x0 = cos(zAngle);
    float y0 = 0;
    float z0 = sin(zAngle);

    for (int i = 0; i <= sectors; ++i)
    {
        sectorAngle = i * sectorStep;
        sideNormals.push_back(cos(sectorAngle) * x0 - sin(sectorAngle) * y0);
        sideNormals.push_back(sin(sectorAngle) * x0 + cos(sectorAngle) * y0);
        sideNormals.push_back(z0);
    }

    // Get unit circle vertices.
    std::vector<float> unitCircleVertices;

    for (int i = 0; i <= sectors; ++i)
    {
        sectorAngle = i * sectorStep;
        unitCircleVertices.push_back(cos(sectorAngle));
        unitCircleVertices.push_back(sin(sectorAngle));
        unitCircleVertices.push_back(0);
    }

    for (int i = stacks; i >= 0; i--)
    {
        z = -(height * 0.5f) + (float)i / stacks * height;
        radius = baseRadius + (float)i / stacks * (0.f - baseRadius);
        float t = 1.0f - (float)i / stacks;

        for (int32_t j = 0, k = 0; j <= sectors; ++j, k += 3)
        {
            Renderer::Vertex1Pos1UV1Norm vert;
            vert.Pos = glm::vec3(unitCircleVertices[static_cast<size_t>(k)] * radius, unitCircleVertices[static_cast<size_t>(k) + 1] * radius, z);
            vert.UV = glm::vec2(static_cast<float>(j) / sectors, t);
            vert.Norm = glm::vec3(sideNormals[static_cast<size_t>(k)], sideNormals[static_cast<size_t>(k) + 1], sideNormals[static_cast<size_t>(k) + 2]);
            coneVertices.push_back(vert);
        }
    }

    int baseCapStart = (int)coneVertices.size();

    // Base cap.
    int numberOfCapVertices = sectors + 2;

    glm::vec3 capCenter = glm::vec3(0.f, 0.f, -height / 2.f);

    for (int i = 0; i < numberOfCapVertices; i++)
    {
        Renderer::Vertex1Pos1UV1Norm v;
        v.Pos.x = capCenter.x + (baseRadius * glm::cos((float)i * glm::two_pi<float>() / (float)sectors));
        v.Pos.y = capCenter.y + (baseRadius * glm::sin((float)i * glm::two_pi<float>() / (float)sectors));
        v.Pos.z = capCenter.z;
        v.UV.x = 0.5f - (v.Pos.x) / (2.f * baseRadius);
        v.UV.y = 0.5f - (v.Pos.y) / (2.f * baseRadius);
        v.Norm = { 0.0f, 0.0f, -1.0f };
        coneVertices.push_back(v);
    }

    {
        Renderer::Vertex1Pos1UV1Norm v;
        v.Pos.x = capCenter.x;
        v.Pos.y = capCenter.y;
        v.Pos.z = capCenter.z;
        v.UV.x = 0.5f;
        v.UV.y = 0.5f;
        v.Norm = { 0.0f, 0.0f, -1.0f };
        coneVertices.push_back(v);
    }

    // Indices.
    std::vector<uint32_t> coneIndices;

    // Base cap.
    for (int32_t i = baseCapStart + 2; i < baseCapStart + sectors; i++)
    {
        coneIndices.push_back(baseCapStart);
        coneIndices.push_back(i - 1);
        coneIndices.push_back(i);
    }

    // Side.
    unsigned int k1, k2;
    for (int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            coneIndices.push_back(k1);
            coneIndices.push_back(k1 + 1);
            coneIndices.push_back(k2);
            coneIndices.push_back(k2);
            coneIndices.push_back(k1 + 1);
            coneIndices.push_back(k2 + 1);
        }
    }

    return Renderer::LoadGeometry(coneVertices.data(), static_cast<uint32_t>(coneVertices.size()),
        coneIndices.data(), static_cast<uint32_t>(coneIndices.size()),
        pID);
}

bool Renderer::LoadTexture(const std::string& textureAssetFilepath, const bool generateMipmaps, uint32_t* pID)
{
    // Load the pixels from the texture
    int32_t textureWidth;
    int32_t textureHeight;
    int32_t textureChannels;
    stbi_uc* texturePixels = stbi_load(textureAssetFilepath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

    // Check the pixels loaded
    if (texturePixels == nullptr)
    {
        return false;
    }

    uint32_t mipLevels{ 1 };
    if (generateMipmaps)
    {
        // Calculate the number of mip levels to generate from the texture
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;
    }

    // Calculate the size of the image
    const int32_t channelCount{ 4 };
    auto imageSize{ static_cast<VkDeviceSize>(textureWidth) * 
        static_cast<VkDeviceSize>(textureHeight) * 
        static_cast<VkDeviceSize>(channelCount) };

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    if (!CreateBuffer(
        gDevice,
        gPhysicalDevice,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        &stagingBuffer,
        &stagingBufferMemory
    ))
    {
        return false;
    }

    // Copy pixels into staging buffer
    void* pData;
    if (vkMapMemory(gDevice, stagingBufferMemory, 0, imageSize, 0, &pData) != VK_SUCCESS)
    {
        return false;
    }
    memcpy(pData, texturePixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(gDevice, stagingBufferMemory);

    // Free pixel data
    stbi_image_free(texturePixels);

    // Assert max loaded texture count has not been reached
    assert(!gAvailableTextureIDs.empty() && "Max loaded texture count reached.");

    // Get an available texture instance
    *pID = gAvailableTextureIDs.front();
    gAvailableTextureIDs.pop();
    auto& texture = gLoadedTextures[*pID];
    gUsedTextureIDs.push_back(*pID);

    // Create image and memory for the texture
    if (!CreateImage(static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight), mipLevels, &texture.GetImage(), &texture.GetImageMemory()))
    {
        return false;
    }
 
    // Begin single submit command buffer
    VkCommandBuffer commandBuffer;
    if (!BeginSingleSubmitCommandBuffer(gDevice, gGraphicsCommandPool, &commandBuffer))
    {
        return false;
    }

    // Transition the image to transfer destination layout
    TransitionImageLayout(commandBuffer, texture.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy staging buffer data into the image
    CopyBufferToImage(commandBuffer, stagingBuffer, texture.GetImage(), static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

    // Check if mipmaps should be generated for the texture
    if (generateMipmaps)
    {
        // Generate mipmaps
        GenerateMipmaps(commandBuffer, texture.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight, mipLevels);
    }
    else
    {
        // Transition the image to shader read only
        TransitionImageLayout(commandBuffer, texture.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, mipLevels, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // End single submit command buffer
    if (!EndAndSubmitSingleSubmitCommandBuffer(gDevice, gGraphicsCommandPool, gGraphicsQueue, commandBuffer))
    {
        return false;
    }

    // Cleanup staging buffer
    DestroyBuffer(stagingBuffer, stagingBufferMemory);

    // Create texture image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.GetImage();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    if (vkCreateImageView(gDevice, &viewInfo, nullptr, &texture.GetImageView()) != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

void Renderer::DestroyGeometry(const uint32_t id)
{
    // Wait for all queues to finish work
    vkQueueWaitIdle(gGraphicsQueue);
    vkQueueWaitIdle(gTransferQueue);

    auto& destroyedGeometry = gLoadedGeometry[id];
    DestroyBuffer(destroyedGeometry.GetVertexBuffer(), destroyedGeometry.GetVertexBufferMemory());
    DestroyBuffer(destroyedGeometry.GetIndexBuffer(), destroyedGeometry.GetIndexBufferMemory());
    destroyedGeometry.Reset();
    gUsedGeometryIDs.erase(std::find(gUsedGeometryIDs.begin(), gUsedGeometryIDs.end(), id));
    gAvailableGeometryIDs.push(id);
}

void Renderer::DestroyTexture(const uint32_t id)
{
    // Wait for all queues to finish work
    vkQueueWaitIdle(gGraphicsQueue) != VK_SUCCESS && vkQueueWaitIdle(gTransferQueue);

    auto& destroyedTexture = gLoadedTextures[id];
    DestroyImage(destroyedTexture.GetImage(), destroyedTexture.GetImageMemory());
    vkDestroyImageView(gDevice, destroyedTexture.GetImageView(), nullptr);
    destroyedTexture.Reset();
    gUsedTextureIDs.erase(std::find(gUsedTextureIDs.begin(), gUsedTextureIDs.end(), id));
    gAvailableTextureIDs.push(id);
}

bool Renderer::WaitForIdle()
{
    // Wait for all queues to finish work
    return (vkQueueWaitIdle(gGraphicsQueue) == VK_SUCCESS) && (vkQueueWaitIdle(gTransferQueue) == VK_SUCCESS);
}
