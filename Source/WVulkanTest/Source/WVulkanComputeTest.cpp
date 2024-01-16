#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <random>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const uint32_t PARTICLE_COUNT = 8192;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, 
        "vkCreateDebugUtilsMessengerEXT"
    );
    if (func != nullptr)
    {
        return func(
            instance, 
            pCreateInfo, 
            pAllocator, 
            pDebugMessenger
        );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance Instance,
    VkDebugUtilsMessengerEXT DebugMessenger,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        Instance,
        "vkDestroyDebugUtilsMessengerEXT"
    );
    if (func != nullptr)
    {
        func(
            Instance,
            DebugMessenger,
            pAllocator
        );
    }
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsAndComputeFamily;
    std::optional<uint32_t> PresentFamily;

    bool IsComplete()
    {
        return GraphicsAndComputeFamily.has_value() && PresentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

struct UniformBufferObject
{
    float DeltaTime = 1.f;
};

struct Particle
{
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Particle);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> AttributeDescriptions = {};

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(Particle, Position);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(Particle, Color);

        return AttributeDescriptions;
    }
};

class ComputeShaderApplication
{
public:
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    GLFWwindow* Window;

    VkInstance Instance;
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkSurfaceKHR Surface;

    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    VkDevice Device;

    VkQueue GraphicsQueue;
    VkQueue ComputeQueue;
    VkQueue PresentQueue;

    VkSwapchainKHR SwapChain;
    std::vector<VkImage> SwapChainImages;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    std::vector<VkImageView> SwapChainImageViews;
    std::vector<VkFramebuffer> SwapChainFramebuffers;

    VkRenderPass RenderPass;
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;

    VkDescriptorSetLayout ComputeDescriptorSetLayout;
    VkPipelineLayout ComputePipelineLayout;
    VkPipeline ComputePipeline;

    VkCommandPool CommandPool;

    std::vector<VkBuffer> ShaderStorageBuffers;
    std::vector<VkDeviceMemory> ShaderStorageBuffersMemory;

    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBuffersMemory;
    std::vector<void*> UniformBuffersMapped;

    VkDescriptorPool DescriptionPool;
    std::vector<VkDescriptorSet> computeDescriptorSets;

    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkCommandBuffer> ComputeCommandBuffers;

    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkSemaphore> ComputeFinishedSemaphores;
    std::vector<VkFence> InFlightFences;
    std::vector<VkFence> CompueInFlightFences;
    uint32_t CurrentFrame = 0;

    float LastFrameTime = 0.f;
    bool FramebufferResized = false;
    double LastTime = 0.f;

    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(Window, this);
        glfwSetFramebufferSizeCallback(Window, FramebufferResizeCallback);

        LastTime = glfwGetTime();
    }

    static void FramebufferResizeCallback(GLFWwindow* Window, int Width, int Height)
    {
        auto App = reinterpret_cast<ComputeShaderApplication*>(glfwGetWindowUserPointer(Window));
        App->FramebufferResized = true;
    }

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();

    }

    void MainLoop()
    {
        while(!glfwWindowShouldClose(Window))
        {
            glfwPollEvents();
            DrawFrame();

            double CurrentTime = glfwGetTime();
            LastFrameTime = (CurrentTime - LastTime) * 1000.f;
            LastTime = CurrentTime;
        }

        vkDeviceWaitIdle(Device);
    }

    void CleanupSwapChain()
    {
        for (auto Framebuffer : SwapChainFramebuffers)
        {
            vkDestroyFramebuffer(Device, Framebuffer, nullptr);
        }
        for (auto ImageView : SwapChainImageViews)
        {
            vkDestroyImageView(Device, ImageView, nullptr);
        }

        vkDestroySwapchainKHR(Device, SwapChain, nullptr);
    }

    void Cleanup()
    {
        CleanupSwapChain();

        vkDestroyPipeline(Device, ComputePipeline, nullptr);
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);

        vkDestroyPipeline(Device, ComputePipeline, nullptr);
        vkDestroyPipelineLayout(Device, ComputePipelineLayout, nullptr);

        vkDestroyRenderPass(Device, RenderPass, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(Device, UniformBuffers[i], nullptr);
            vkFreeMemory(Device, UniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(Device, DescriptionPool, nullptr);
        vkDestroyDescriptorSetLayout(Device, ComputeDescriptorSetLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(Device, ShaderStorageBuffers[i], nullptr);
            vkFreeMemory(Device, ShaderStorageBuffersMemory[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(Device, RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(Device, ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(Device, ComputeFinishedSemaphores[i], nullptr);
            vkDestroyFence(Device, InFlightFences[i], nullptr);
            vkDestroyFence(Device, CompueInFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(Device, CommandPool, nullptr);

        vkDestroyDevice(Device, nullptr);

        if (EnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(Instance, Surface, nullptr);
        vkDestroyInstance(Instance, nullptr);

        glfwDestroyWindow(Window);

        glfwTerminate();
    }

    void RecreateSwapChain()
    {
        int Width=0, Height=0;
        glfwGetFramebufferSize(Window, &Width, &Height);
        while (Width == 0 || Height == 0)
        {
            glfwGetFramebufferSize(Window, &Width, &Height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(Device);

        CleanupSwapChain();

        CreateSwapChain();
        CreateImageViews();
        CreateFramebuffers();
    }

    void CreateInstance()
    {
        if (EnableValidationLayers && !CheckValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        VkApplicationInfo AppInfo = {};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = "Vulkan Compute Test";
        AppInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        AppInfo.pEngineName = "No Engine";
        AppInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        AppInfo.apiVersion = VK_API_VERSION_1_3; // check if this is correct

        VkInstanceCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        CreateInfo.pApplicationInfo = &AppInfo;

        auto Extensions = GetRequiredExtensions();
        CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
        CreateInfo.ppEnabledExtensionNames = Extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};
        if (EnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();

            PopulateDebugMessengerCreateInfo(DebugCreateInfo);
            CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&DebugCreateInfo;
        }
        else
        {
            CreateInfo.enabledLayerCount = 0;
            CreateInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create instance!");
        }
    }

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
    {
        CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    }

    void SetupDebugMessenger()
    {
        if (!EnableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT CreateInfo;
        PopulateDebugMessengerCreateInfo(CreateInfo);

        if (CreateDebugUtilsMessengerEXT(Instance, &CreateInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    void CreateSurface()
    {
        if (glfwCreateWindowSurface(Instance, Window, nullptr, &Surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    void PickPhysicalDevice()
    {
        uint32_t DeviceCount = 0;
        vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

        if (DeviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> Devices(DeviceCount);
        vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

        for (const auto& Device : Devices)
        {
            if (IsDeviceSuitable(Device))
            {
                PhysicalDevice = Device;
                break;
            }
        }

        if (PhysicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    void CreateLogicalDevice()
    {
        QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
        std::set<uint32_t> UniqueQueueFamilies = {
            Indices.GraphicsAndComputeFamily.value(),
            Indices.PresentFamily.value()
        };
        
        float QueuePriority = 1.f;
        for (uint32_t QueueFamily : UniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo QueueCreateInfo{};
            QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueueCreateInfo.queueFamilyIndex = QueueFamily;
            QueueCreateInfo.queueCount = 1;
            QueueCreateInfo.pQueuePriorities = &QueuePriority;
            QueueCreateInfos.push_back(QueueCreateInfo);
        }

        VkPhysicalDeviceFeatures DeviceFeatures{};

        VkDeviceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
        CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();

        CreateInfo.pEnabledFeatures = &DeviceFeatures;

        CreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
        CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

        if (EnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        else
        {
            CreateInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(Device, Indices.GraphicsAndComputeFamily.value(), 0, &GraphicsQueue);
        vkGetDeviceQueue(Device, Indices.GraphicsAndComputeFamily.value(), 0, &ComputeQueue);
        vkGetDeviceQueue(Device, Indices.PresentFamily.value(), 0, &PresentQueue);
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice Device)
    {
        SwapChainSupportDetails Details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Details.Capabilities);

        uint32_t FormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);

        if (FormatCount != 0)
        {
            Details.Formats.resize(FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                Device, 
                Surface, 
                &FormatCount, 
                Details.Formats.data()
            );
        }

        uint32_t PresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            Device, 
            Surface, 
            &PresentModeCount, 
            nullptr
        );

        if (PresentModeCount != 0)
        {
            Details.PresentModes.resize(PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                Device,
                Surface,
                &PresentModeCount,
                Details.PresentModes.data()
            );
        }
        return Details;
    }

    bool IsDeviceSuitable(const VkPhysicalDevice& InDevice)
    {
        QueueFamilyIndices Indices = FindQueueFamilies(InDevice);

        bool ExtensionsSupported = CheckDeviceExtensionSupport(InDevice);

        bool SwapChainAdequate = false;
        if (ExtensionsSupported)
        {
            SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(InDevice);
            SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
        }
        return Indices.IsComplete() && ExtensionsSupported && SwapChainAdequate;
    }

    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& InDevice)
    {
        uint32_t ExtensionCount;
        vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &ExtensionCount, AvailableExtensions.data());

        std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

        for (const auto& Extension : AvailableExtensions)
        {
            RequiredExtensions.erase(Extension.extensionName);
        }

        return RequiredExtensions.empty();
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device)
    {
        QueueFamilyIndices Indices;

        uint32_t QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

        int i = 0;
        for (const auto& QueueFamily : QueueFamilies)
        {
            if ((QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                Indices.GraphicsAndComputeFamily = i;
            }

            VkBool32 PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &PresentSupport);

            if (PresentSupport)
            {
                Indices.PresentFamily = i;
            }

            if (Indices.IsComplete())
            {
                break;
            }
            i++;
        }
        return Indices;
    }

    std::vector<const char*> GetRequiredExtensions()
    {
        uint32_t GLFWExtensionCount = 0;
        const char** GLFWExtensions;
        GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

        std::vector<const char*> Extensions(GLFWExtensions, GLFWExtensions + GLFWExtensionCount);

        if (EnableValidationLayers)
        {
            Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return Extensions;
    }

    bool CheckValidationLayerSupport()
    {
        uint32_t LayerCount;
        vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

        std::vector<VkLayerProperties> AvailableLayers(LayerCount);
        vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

        for (const char* LayerName : ValidationLayers)
        {
            bool LayerFound = false;

            for (const auto& LayerProperties : AvailableLayers)
            {
                if (strcmp(LayerName, LayerProperties.layerName) == 0)
                {
                    LayerFound = true;
                    break;
                }
            }

            if (!LayerFound)
            {
                return false;
            }
        }
        return true;
    }

};