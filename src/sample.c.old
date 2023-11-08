
#include "apollo.h"
#include "logger.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <vulkan/vulkan.h>

#define LS SECTOR_MAIN_APOLLO
#define PF "\033[32m%f\033[0m"

// static const char *ENABLED_LAYERS[] = {
//     "VK_LAYER_KHRONOS_validation",
//     // "VK_LAYER_LUNARG_api_dump"
// };

static const char *vk_result_string(VkResult result);

VkInstance instance;
VkSurfaceKHR surface;
VkResult result;
VkDevice device;
VkPhysicalDevice physical_device;

VkDeviceMemory in_memory;
VkDeviceMemory out_memory;

VkBuffer in_buffer;
VkBuffer out_buffer;

VkShaderModule shader_module;

VkDescriptorSetLayout descriptor_set_layout;
VkPipelineLayout pipeline_layout;
VkPipelineCache pipeline_cache;
VkDescriptorPool descriptor_pool;
VkDescriptorSet descriptor_set;
VkCommandPool cmd_pool;
VkCommandBuffer cmd_buffer;

VkPipeline compute_pipeline;

VkFence fence;

void cleanup(void) {
    vkResetCommandPool(device, cmd_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    vkDestroyFence(device, fence, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);

    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyPipelineCache(device, pipeline_cache, NULL);

    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);

    vkDestroyShaderModule(device, shader_module, NULL);
    vkDestroyPipeline(device, compute_pipeline, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyCommandPool(device, cmd_pool, NULL);

    vkDestroyBuffer(device, in_buffer, NULL);
    vkDestroyBuffer(device, out_buffer, NULL);

    vkFreeMemory(device, in_memory, NULL);
    vkFreeMemory(device, out_memory, NULL);



    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
}

int main(void) {
    log_info("Starting Apollo Valkan");

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Apollo",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    if ((result = vkCreateInstance(&create_info, NULL, &instance))) {
        log_error("instance creation faild: %s", vk_result_string(result));
    }

    uint32_t vk_dev_count = 1;
    vkEnumeratePhysicalDevices(instance, &vk_dev_count, &physical_device);
    log_info("%d devices", vk_dev_count);

    if (!vk_dev_count) {
        log_error("no physical device was found");
        cleanup();
        return 1;
    }

    uint32_t compute_queue_family = 0;
    uint32_t queue_family_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_family_list = malloc(
        sizeof(VkQueueFamilyProperties) * queue_family_count
    );
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_list);

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_family_list[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            compute_queue_family = i;
            break;
        }
    }

    log_info("compute_queue_family: %d", compute_queue_family);


    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .queueCount = 1,
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = compute_queue_family,
        .pQueuePriorities = &queue_priority,
    };


    VkPhysicalDeviceFeatures device_features;
    memset(&device_features, 0, sizeof(device_features));

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &device_features,
    };

    if ((result = vkCreateDevice(physical_device, &device_create_info, NULL, &device))) {
        log_error("device creation faild: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    // VkQueue queue;
    // vkGetDeviceQueue(device, queue_create_info.queueFamilyIndex, 0, &queue);


    uint32_t number_of_elements = 10;
    uint32_t buffer_size = number_of_elements * sizeof(uint32_t);

    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .pQueueFamilyIndices = &compute_queue_family,
        .queueFamilyIndexCount = 1
    };


    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_props);

    uint32_t memory_type_index = 0;
    uint64_t memory_heap_size = 0;

    for (uint32_t i = 0; i < memory_props.memoryTypeCount; ++i) {
        VkMemoryType memory_type = memory_props.memoryTypes[i];

        if (
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memory_type.propertyFlags) &&
            (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & memory_type.propertyFlags)
        ) {
            memory_heap_size = memory_props.memoryHeaps[memory_type.heapIndex].size;
            memory_type_index = i;
            break;
        }
    }

    log_info(
        "memory type index: %d\nmemory heap size: %ldGB",
        memory_type_index, memory_heap_size / 1024 / 1024 / 1024
    );

    // input memory
    if ((result = vkCreateBuffer(device, &buffer_create_info, NULL, &in_buffer))) {
        log_error("input buffer creation failded: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkMemoryRequirements in_memory_requirements;
    vkGetBufferMemoryRequirements(device, in_buffer, &in_memory_requirements);

    VkMemoryAllocateInfo in_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = in_memory_requirements.size,
        .memoryTypeIndex = memory_type_index
    };

    if ((result = vkAllocateMemory(device, &in_allocate_info, NULL, &in_memory))) {
        log_error("input memory allocation failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    // write some stuff in input memory
    uint32_t *input_data;
    vkMapMemory(device, in_memory, 0, buffer_size, 0, (void *)&input_data);
    for (uint32_t i = 0; i < number_of_elements; ++i) {
        input_data[i] = i;
    }
    vkUnmapMemory(device, in_memory);

    if ((result = vkBindBufferMemory(device, in_buffer, in_memory, 0))) {
        log_error("input bind faild: %s", vk_result_string(result));
        cleanup();
        return 1;
    }
    // end of input memory

    // output memory
    if ((result = vkCreateBuffer(device, &buffer_create_info, NULL, &out_buffer))) {
        log_error("output buffer creation failded: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkMemoryRequirements out_memory_requirements;
    vkGetBufferMemoryRequirements(device, in_buffer, &out_memory_requirements);

    VkMemoryAllocateInfo out_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = out_memory_requirements.size,
        .memoryTypeIndex = memory_type_index
    };

    if ((result = vkAllocateMemory(device, &out_allocate_info, NULL, &out_memory))) {
        log_error("output memory allocation failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    if ((result = vkBindBufferMemory(device, out_buffer, out_memory, 0))) {
        log_error("output bind faild: %s", vk_result_string(result));
        cleanup();
        return 1;
    }
    // end of output memory

    int fd = open("./shader/spv/compute.spv", O_RDONLY);
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *shader_data = malloc(file_size);
    read(fd, shader_data, file_size);

    VkShaderModuleCreateInfo shader_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = file_size,
        .pCode = (uint32_t *)shader_data,
    };

    if ((result = vkCreateShaderModule(device, &shader_create_info, NULL, &shader_module))) {
        log_error("shader creation faild: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkDescriptorSetLayoutBinding layout_binding[] = {
        [0] = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
        },
        [1] = {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
        },
    };

    VkDescriptorSetLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = layout_binding,
    };

    if ((result = vkCreateDescriptorSetLayout(device, &layout_create_info, NULL, &descriptor_set_layout))) {
        log_error("descriptor layout creation faild: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout
    };

    if ((result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout))) {
        log_error("pipeline layout creation: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkPipelineCacheCreateInfo pipeline_cache_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    };

    if ((result = vkCreatePipelineCache(device, &pipeline_cache_create_info, NULL, &pipeline_cache))) {
        log_error("pipeline cache creation: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_module,
        .pName = "main"
    };

    VkComputePipelineCreateInfo compute_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = pipeline_shader_stage_create_info,
        .layout = pipeline_layout,
    };

    if ((result = vkCreateComputePipelines(device, pipeline_cache, 1, &compute_pipeline_create_info, NULL, &compute_pipeline))) {
        log_error("compute pipeline creation faild: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkDescriptorPoolSize descriptor_pool_size = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 2
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptor_pool_size
    };

    if ((result = vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &descriptor_pool))) {
        log_error("descriptor pool creation failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout
    };

    if ((result = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set))) {
        log_error("allocating descriptor sets failded: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkDescriptorBufferInfo in_buffer_info = {
        .buffer = in_buffer,
        .offset = 0,
        .range = buffer_size,
    };

    VkDescriptorBufferInfo out_buffer_info = {
        .buffer = out_buffer,
        .offset = 0,
        .range = buffer_size,
    };

    VkWriteDescriptorSet write_descriptor_set[] = {
        [0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = &in_buffer_info
        },
        [1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = &out_buffer_info
        },

    };

    vkUpdateDescriptorSets(device, 2, write_descriptor_set, 0, NULL);


    VkCommandPoolCreateInfo cmd_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = compute_queue_family,
    };

    vkCreateCommandPool(device, &cmd_pool_create_info, NULL, &cmd_pool);
    VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
    // VkStructureType         sType;
    // const void*             pNext;
    // VkCommandPool           commandPool;
    // VkCommandBufferLevel    level;
    // uint32_t                commandBufferCount;
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if ((result = vkAllocateCommandBuffers(device, &cmd_buffer_allocate_info, &cmd_buffer))) {
        log_error("allocating cmd buffer failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkCommandBufferBeginInfo cmd_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    if ((result = vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info))) {
        log_error("begin cmd buffer failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
    vkCmdBindDescriptorSets(
        cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout,
        0, 1, &descriptor_set, 0, NULL
    );

    vkCmdDispatch(cmd_buffer, number_of_elements, 1, 1);

    if ((result = vkEndCommandBuffer(cmd_buffer))) {
        log_error("end cmd buffer failded: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    VkQueue queue;
    vkGetDeviceQueue(device, compute_queue_family, 0, &queue);

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    vkCreateFence(device, &fence_create_info, NULL, &fence);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL
    };

    if ((result = vkQueueSubmit(queue, 1, &submit_info, fence))) {
        log_error("queue submit failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    if ((result = vkWaitForFences(device, 1, &fence, true, -1))) {
        log_error("waiting for fence failed: %s", vk_result_string(result));
        cleanup();
        return 1;
    }

    vkMapMemory(device, in_memory, 0, buffer_size, 0, (void *)&input_data);
    uint32_t *output_data;
    vkMapMemory(device, out_memory, 0, buffer_size, 0, (void *)&output_data);

    for (uint32_t i = 0; i < number_of_elements; ++i) {
        log_info("[%d] %d -> %d", i, input_data[i], output_data[i]);
    }

    vkUnmapMemory(device, in_memory);
    vkUnmapMemory(device, out_memory);

    cleanup();
    return 0;
}

static const char *vk_result_string(VkResult result) {
#define CASE(x) case VK_##x: return #x;
    switch (result) {
        CASE(SUCCESS) CASE(NOT_READY) CASE(TIMEOUT) CASE(EVENT_SET)
        CASE(EVENT_RESET) CASE(INCOMPLETE) CASE(ERROR_OUT_OF_HOST_MEMORY)
        CASE(ERROR_OUT_OF_DEVICE_MEMORY) CASE(ERROR_INITIALIZATION_FAILED)
        CASE(ERROR_DEVICE_LOST) CASE(ERROR_MEMORY_MAP_FAILED)
        CASE(ERROR_LAYER_NOT_PRESENT) CASE(ERROR_EXTENSION_NOT_PRESENT)
        CASE(ERROR_FEATURE_NOT_PRESENT) CASE(ERROR_INCOMPATIBLE_DRIVER)
        CASE(ERROR_TOO_MANY_OBJECTS) CASE(ERROR_FORMAT_NOT_SUPPORTED)
        CASE(ERROR_FRAGMENTED_POOL) CASE(ERROR_UNKNOWN)
        CASE(ERROR_OUT_OF_POOL_MEMORY) CASE(ERROR_INVALID_EXTERNAL_HANDLE)
        CASE(ERROR_FRAGMENTATION) CASE(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        CASE(PIPELINE_COMPILE_REQUIRED) CASE(ERROR_SURFACE_LOST_KHR)
        CASE(ERROR_NATIVE_WINDOW_IN_USE_KHR) CASE(SUBOPTIMAL_KHR)
        CASE(ERROR_OUT_OF_DATE_KHR) CASE(ERROR_INCOMPATIBLE_DISPLAY_KHR)
        CASE(ERROR_VALIDATION_FAILED_EXT) CASE(ERROR_INVALID_SHADER_NV)
        CASE(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
        CASE(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
        CASE(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
        CASE(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
        CASE(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
        CASE(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
        CASE(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        CASE(ERROR_NOT_PERMITTED_KHR)
        CASE(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT) CASE(THREAD_IDLE_KHR)
        CASE(THREAD_DONE_KHR) CASE(OPERATION_DEFERRED_KHR)
        CASE(OPERATION_NOT_DEFERRED_KHR)
    #ifdef VK_ENABLE_BETA_EXTENSIONS
        CASE(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
    #endif
        CASE(ERROR_COMPRESSION_EXHAUSTED_EXT)
        CASE(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT) CASE(RESULT_MAX_ENUM)
        default: return "unknown";
    }
#undef CASE
}
