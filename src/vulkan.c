
#include "hera.h"
#include "logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <vulkan/vulkan.h>

#include <glslang/Include/glslang_c_interface.h>
// #include <spirv-tools/libspirv.h>
// #include <glslang/Public/ResourceLimits.h>

#define LS SECTOR_VULKAN

// static const char *vk_result_string(VkResult result);

uint64_t input_size = 8192 * 8192 * 4;
uint64_t output_size = 1920 * 1080 * 4;
uint64_t uniform_size = sizeof(ScaleData);

VkInstance instance;
VkSurfaceKHR surface;
VkResult result;
VkDevice device;
VkPhysicalDevice physical_device;

VkDeviceMemory in_memory;
VkDeviceMemory out_memory;
VkDeviceMemory uniform_memory;

VkBuffer in_buffer;
VkBuffer out_buffer;
VkBuffer uniform_buffer;

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
VkQueue queue;

VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = NULL,
    .pWaitDstStageMask = NULL,
    .commandBufferCount = 0,
    .pCommandBuffers = NULL,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores = NULL
};

const glslang_resource_t DEFAULT_BUILTIN_RESOURCE = {
    .max_lights = 32,
    .max_clip_planes = 6,
    .max_texture_units = 32,
    .max_texture_coords = 32,
    .max_vertex_attribs = 64,
    .max_vertex_uniform_components = 4096,
    .max_varying_floats = 64,
    .max_vertex_texture_image_units = 32,
    .max_combined_texture_image_units = 80,
    .max_texture_image_units = 32,
    .max_fragment_uniform_components = 4096,
    .max_draw_buffers = 32,
    .max_vertex_uniform_vectors = 128,
    .max_varying_vectors = 8,
    .max_fragment_uniform_vectors = 16,
    .max_vertex_output_vectors = 16,
    .max_fragment_input_vectors = 15,
    .min_program_texel_offset = -8,
    .max_program_texel_offset = 7,
    .max_clip_distances = 8,
    .max_compute_work_group_count_x = 65535,
    .max_compute_work_group_count_y = 65535,
    .max_compute_work_group_count_z = 65535,
    .max_compute_work_group_size_x = 1024,
    .max_compute_work_group_size_y = 1024,
    .max_compute_work_group_size_z = 64,
    .max_compute_uniform_components = 1024,
    .max_compute_texture_image_units = 16,
    .max_compute_image_uniforms = 8,
    .max_compute_atomic_counters = 8,
    .max_compute_atomic_counter_buffers = 1,
    .max_varying_components = 60,
    .max_vertex_output_components = 64,
    .max_geometry_input_components = 64,
    .max_geometry_output_components = 128,
    .max_fragment_input_components = 128,
    .max_image_units = 8,
    .max_combined_image_units_and_fragment_outputs = 8,
    .max_combined_shader_output_resources = 8,
    .max_image_samples = 0,
    .max_vertex_image_uniforms = 0,
    .max_tess_control_image_uniforms = 0,
    .max_tess_evaluation_image_uniforms = 0,
    .max_geometry_image_uniforms = 0,
    .max_fragment_image_uniforms = 8,
    .max_combined_image_uniforms = 8,
    .max_geometry_texture_image_units = 16,
    .max_geometry_output_vertices = 256,
    .max_geometry_total_output_components = 1024,
    .max_geometry_uniform_components = 1024,
    .max_geometry_varying_components = 64,
    .max_tess_control_input_components = 128,
    .max_tess_control_output_components = 128,
    .max_tess_control_texture_image_units = 16,
    .max_tess_control_uniform_components = 1024,
    .max_tess_control_total_output_components = 4096,
    .max_tess_evaluation_input_components = 128,
    .max_tess_evaluation_output_components = 128,
    .max_tess_evaluation_texture_image_units = 16,
    .max_tess_evaluation_uniform_components = 1024,
    .max_tess_patch_components = 120,
    .max_patch_vertices = 32,
    .max_tess_gen_level = 64,
    .max_viewports = 16,
    .max_vertex_atomic_counters = 0,
    .max_tess_control_atomic_counters = 0,
    .max_tess_evaluation_atomic_counters = 0,
    .max_geometry_atomic_counters = 0,
    .max_fragment_atomic_counters = 8,
    .max_combined_atomic_counters = 8,
    .max_atomic_counter_bindings = 1,
    .max_vertex_atomic_counter_buffers = 0,
    .max_tess_control_atomic_counter_buffers = 0,
    .max_tess_evaluation_atomic_counter_buffers = 0,
    .max_geometry_atomic_counter_buffers = 0,
    .max_fragment_atomic_counter_buffers = 1,
    .max_combined_atomic_counter_buffers = 1,
    .max_atomic_counter_buffer_size = 16384,
    .max_transform_feedback_buffers = 4,
    .max_transform_feedback_interleaved_components = 64,
    .max_cull_distances = 8,
    .max_combined_clip_and_cull_distances = 8,
    .max_samples = 4,
    .max_mesh_output_vertices_nv = 256,
    .max_mesh_output_primitives_nv = 512,
    .max_mesh_work_group_size_x_nv = 32,
    .max_mesh_work_group_size_y_nv = 1,
    .max_mesh_work_group_size_z_nv = 1,
    .max_task_work_group_size_x_nv = 32,
    .max_task_work_group_size_y_nv = 1,
    .max_task_work_group_size_z_nv = 1,
    .max_mesh_view_count_nv = 4,
    .max_mesh_output_vertices_ext = 256,
    .max_mesh_output_primitives_ext = 256,
    .max_mesh_work_group_size_x_ext = 128,
    .max_mesh_work_group_size_y_ext = 128,
    .max_mesh_work_group_size_z_ext = 128,
    .max_task_work_group_size_x_ext = 128,
    .max_task_work_group_size_y_ext = 128,
    .max_task_work_group_size_z_ext = 128,
    .max_mesh_view_count_ext = 4,
    .max_dual_source_draw_buffers_ext = 1,

    .limits = {
        .non_inductive_for_loops = 1,
        .while_loops = 1,
        .do_while_loops = 1,
        .general_uniform_indexing = 1,
        .general_attribute_matrix_vector_indexing = 1,
        .general_varying_indexing = 1,
        .general_sampler_indexing = 1,
        .general_variable_indexing = 1,
        .general_constant_matrix_vector_indexing = 1
    }
};


// /* Callback for local file inclusion */
// typedef glsl_include_result_t* (*glsl_include_local_func)(void* ctx, const char* header_name, const char* includer_name,
//                                                           size_t include_depth);
//
// /* Callback for system file inclusion */
// typedef glsl_include_result_t* (*glsl_include_system_func)(void* ctx, const char* header_name,
//                                                            const char* includer_name, size_t include_depth);
//
// /* Callback for include result destruction */
// typedef int (*glsl_free_include_result_func)(void* ctx, glsl_include_result_t* result);
//


// glsl_include_result_t *glsl_include_local(
//     void *ctx, const char *header_name, const size_t include_depth
// ) {
//     glsl_include_result_t result = {
//         .header_name = header_name,
//         .header_length = 1,
//         .header_data
//     };
// }


void vk_init(uint8_t shader_index) {
    log_info("--- Valkan Init ---");

    int fd = -1;
    if (shader_index % 2) {
        fd = open("./shader/cube.comp.glsl", O_RDONLY);
    } else {
        fd = open("./shader/point.comp.glsl", O_RDONLY);
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *shader_source = malloc(file_size + 1);
    read(fd, shader_source, file_size);
    close(fd);
    shader_source[file_size] = 0;

    // spv_context ctx;
    // spv_binary bin;
    // spv_diagnostic diagnostic;
    //
    // spv_result_t result = spvTextToBinary(
    //     ctx, shader_source, file_size, &bin, &diagnostic
    // );
    // log_info("result: %s", result);

    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = GLSLANG_STAGE_COMPUTE,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = shader_source,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = &DEFAULT_BUILTIN_RESOURCE,
    };

    glslang_initialize_process();

    glslang_shader_t *shader = glslang_shader_create(&input);
    glslang_shader_preprocess(shader, &input);
    glslang_shader_parse(shader, &input);

    glslang_program_t *program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
    glslang_program_SPIRV_generate(program, GLSLANG_STAGE_COMPUTE);
    size_t spirv_size = glslang_program_SPIRV_get_size(program);
    uint32_t *spirv_words = malloc(spirv_size * sizeof(uint32_t));
    glslang_program_SPIRV_get(program, spirv_words);
    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    log_info("spirv message: %s", spirv_messages);

    glslang_program_delete(program);
    glslang_shader_delete(shader);
    glslang_finalize_process();

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hera",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0), 
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    vkCreateInstance(&create_info, NULL, &instance);

    uint32_t vk_dev_count = 1;
    vkEnumeratePhysicalDevices(instance, &vk_dev_count, &physical_device);

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

    vkCreateDevice(physical_device, &device_create_info, NULL, &device);

    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_props);

    uint32_t memory_type_index = 0;

    for (uint32_t i = 0; i < memory_props.memoryTypeCount; ++i) {
        VkMemoryType memory_type = memory_props.memoryTypes[i];

        if (
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memory_type.propertyFlags) &&
            (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & memory_type.propertyFlags)
        ) {
            memory_type_index = i;
            break;
        }
    }

    // input memory
    VkBufferCreateInfo input_buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = input_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .pQueueFamilyIndices = &compute_queue_family,
        .queueFamilyIndexCount = 1
    };
    vkCreateBuffer(device, &input_buffer_create_info, NULL, &in_buffer);
        
    VkMemoryRequirements in_memory_requirements;
    vkGetBufferMemoryRequirements(device, in_buffer, &in_memory_requirements);

    VkMemoryAllocateInfo in_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = in_memory_requirements.size,
        .memoryTypeIndex = memory_type_index
    };

    vkAllocateMemory(device, &in_allocate_info, NULL, &in_memory);
    vkBindBufferMemory(device, in_buffer, in_memory, 0);

    // output memory
    VkBufferCreateInfo output_buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = output_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .pQueueFamilyIndices = &compute_queue_family,
        .queueFamilyIndexCount = 1
    };
    vkCreateBuffer(device, &output_buffer_create_info, NULL, &out_buffer);

    VkMemoryRequirements out_memory_requirements;
    vkGetBufferMemoryRequirements(device, in_buffer, &out_memory_requirements);

    VkMemoryAllocateInfo out_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = out_memory_requirements.size,
        .memoryTypeIndex = memory_type_index
    };

    vkAllocateMemory(device, &out_allocate_info, NULL, &out_memory);
    vkBindBufferMemory(device, out_buffer, out_memory, 0);
    // end of output memory

    VkBufferCreateInfo uniform_buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = uniform_size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .pQueueFamilyIndices = &compute_queue_family,
        .queueFamilyIndexCount = 1
    };
    vkCreateBuffer(device, &uniform_buffer_create_info, NULL, &uniform_buffer);
        
    VkMemoryRequirements uniform_memory_requirements;
    vkGetBufferMemoryRequirements(device, uniform_buffer, &uniform_memory_requirements);

    VkMemoryAllocateInfo uniform_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = uniform_memory_requirements.size,
        .memoryTypeIndex = memory_type_index
    };

    vkAllocateMemory(device, &uniform_allocate_info, NULL, &uniform_memory);
    vkBindBufferMemory(device, uniform_buffer, uniform_memory, 0);

    // fd = open("./shader/spv/cube.comp.spv", O_RDONLY);
    // file_size = lseek(fd, 0, SEEK_END);
    // lseek(fd, 0, SEEK_SET);
    // char *shader_data = malloc(file_size);
    // read(fd, shader_data, file_size);
    // close(fd);

    VkShaderModuleCreateInfo shader_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv_size * sizeof(uint32_t),
        .pCode = spirv_words
    };

    vkCreateShaderModule(device, &shader_create_info, NULL, &shader_module);

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
        [2] = {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL
        },
    };

    VkDescriptorSetLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 3,
        .pBindings = layout_binding,
    };

    vkCreateDescriptorSetLayout(device, &layout_create_info, NULL, &descriptor_set_layout);
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout
    };

    vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout);
    VkPipelineCacheCreateInfo pipeline_cache_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    };

    vkCreatePipelineCache(device, &pipeline_cache_create_info, NULL, &pipeline_cache);
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_module,
        .pName = "main",
    };

    VkComputePipelineCreateInfo compute_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = pipeline_shader_stage_create_info,
        .layout = pipeline_layout,
    };

    vkCreateComputePipelines(
        device, pipeline_cache, 1, &compute_pipeline_create_info,
        NULL, &compute_pipeline
    );

    VkDescriptorPoolSize descriptor_pool_size = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 3
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptor_pool_size
    };

    vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &descriptor_pool);
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout
    };

    vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);

    VkDescriptorBufferInfo in_buffer_info = {
        .buffer = in_buffer,
        .offset = 0,
        .range = input_size,
    };

    VkDescriptorBufferInfo out_buffer_info = {
        .buffer = out_buffer,
        .offset = 0,
        .range = output_size,
    };

    VkDescriptorBufferInfo uniform_buffer_info = {
        .buffer = uniform_buffer,
        .offset = 0,
        .range = uniform_size,
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
        [2] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = &uniform_buffer_info
        },
    };

    vkUpdateDescriptorSets(device, 3, write_descriptor_set, 0, NULL);

    VkCommandPoolCreateInfo cmd_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = compute_queue_family,
    };

    vkCreateCommandPool(device, &cmd_pool_create_info, NULL, &cmd_pool);
    VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    vkAllocateCommandBuffers(device, &cmd_buffer_allocate_info, &cmd_buffer);
    VkCommandBufferBeginInfo cmd_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info);
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
    vkCmdBindDescriptorSets(
        cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout,
        0, 1, &descriptor_set, 0, NULL
    );

    vkCmdDispatch(cmd_buffer, 1920, 1080, 1);

    vkEndCommandBuffer(cmd_buffer);

    vkGetDeviceQueue(device, compute_queue_family, 0, &queue);

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    vkCreateFence(device, &fence_create_info, NULL, &fence);


    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;

    log_info("end of vk_init");
}

void vk_set_input(uint8_t *data, uint64_t size) {
    uint8_t *input_data;
    vkMapMemory(device, in_memory, 0, input_size, 0, (void *)&input_data);
    memcpy(input_data, data, size);
    vkUnmapMemory(device, in_memory);
    log_info("end of vk_set_input");
}

void vk_update_scale(ScaleData *input) {
    input->wr = (float)input->ow / input->iw;
    input->hr = (float)input->oh / input->ih;
    input->r = input->wr > input->hr ? input->hr : input->wr;
    input->imaxw = input->r * input->iw;
    input->imaxh = input->r * input->ih;
    input->offset_x = (input->ow - input->imaxw) / 2;
    input->offset_y = (input->oh - input->imaxh) / 2;

    log_info(
        "\nScaleData {\n"
        "  .zoom: %f\n"
        "  .iw: %u\n"
        "  .ih: %u\n"
        "  .ow: %u\n"
        "  .oh: %u\n"
        "  .wr: %f\n"
        "  .hr: %f\n"
        "  .r: %f\n"
        "  .imaxw: %u\n"
        "  .imaxh: %u\n"
        "  .offset_x: %u\n"
        "  .offset_y: %u\n"
        "}",
        input->zoom, input->iw, input->ih, input->ow, input->oh,
        input->wr, input->hr, input->r, input->imaxw, input->imaxh,
        input->offset_x, input->offset_y
    );
    ScaleData *data;
    vkMapMemory(device, uniform_memory, 0, uniform_size, 0, (void *)&data);
    memcpy(data, input, sizeof(ScaleData));
    vkUnmapMemory(device, uniform_memory);
    log_info("end of updating scale");
}

void vk_get_output(uint8_t *output) {
    vkResetFences(device, 1, &fence);
    vkQueueSubmit(queue, 1, &submit_info, fence);

    vkWaitForFences(device, 1, &fence, true, -1);

    uint32_t *output_data;
    vkMapMemory(device, out_memory, 0, output_size, 0, (void *)&output_data);
    memcpy(output, output_data, output_size);
    vkUnmapMemory(device, out_memory);
}

void vk_cleanup(void) {
    
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
    vkDestroyBuffer(device, uniform_buffer, NULL);

    vkFreeMemory(device, in_memory, NULL);
    vkFreeMemory(device, out_memory, NULL);
    vkFreeMemory(device, uniform_memory, NULL);

    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
}

// static const char *vk_result_string(VkResult result) {
// #define CASE(x) case VK_##x: return #x;
//     switch (result) {
//         CASE(SUCCESS) CASE(NOT_READY) CASE(TIMEOUT) CASE(EVENT_SET)
//         CASE(EVENT_RESET) CASE(INCOMPLETE) CASE(ERROR_OUT_OF_HOST_MEMORY)
//         CASE(ERROR_OUT_OF_DEVICE_MEMORY) CASE(ERROR_INITIALIZATION_FAILED)
//         CASE(ERROR_DEVICE_LOST) CASE(ERROR_MEMORY_MAP_FAILED)
//         CASE(ERROR_LAYER_NOT_PRESENT) CASE(ERROR_EXTENSION_NOT_PRESENT)
//         CASE(ERROR_FEATURE_NOT_PRESENT) CASE(ERROR_INCOMPATIBLE_DRIVER)
//         CASE(ERROR_TOO_MANY_OBJECTS) CASE(ERROR_FORMAT_NOT_SUPPORTED)
//         CASE(ERROR_FRAGMENTED_POOL) CASE(ERROR_UNKNOWN)
//         CASE(ERROR_OUT_OF_POOL_MEMORY) CASE(ERROR_INVALID_EXTERNAL_HANDLE)
//         CASE(ERROR_FRAGMENTATION) CASE(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
//         CASE(PIPELINE_COMPILE_REQUIRED) CASE(ERROR_SURFACE_LOST_KHR)
//         CASE(ERROR_NATIVE_WINDOW_IN_USE_KHR) CASE(SUBOPTIMAL_KHR)
//         CASE(ERROR_OUT_OF_DATE_KHR) CASE(ERROR_INCOMPATIBLE_DISPLAY_KHR)
//         CASE(ERROR_VALIDATION_FAILED_EXT) CASE(ERROR_INVALID_SHADER_NV)
//         CASE(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
//         CASE(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
//         CASE(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
//         CASE(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
//         CASE(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
//         CASE(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
//         CASE(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
//         CASE(ERROR_NOT_PERMITTED_KHR)
//         CASE(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT) CASE(THREAD_IDLE_KHR)
//         CASE(THREAD_DONE_KHR) CASE(OPERATION_DEFERRED_KHR)
//         CASE(OPERATION_NOT_DEFERRED_KHR)
//     #ifdef VK_ENABLE_BETA_EXTENSIONS
//         CASE(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
//     #endif
//         CASE(ERROR_COMPRESSION_EXHAUSTED_EXT)
//         CASE(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT) CASE(RESULT_MAX_ENUM)
//         default: return "unknown";
//     }
// #undef CASE
// }
