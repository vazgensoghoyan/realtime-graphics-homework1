#include <wgpu_app.hpp>
#include <file_utils.hpp>
#include <obj_loader.hpp>
#include <sphere.hpp>
#include "camera.hpp"
#include "scene.hpp"

#include <webgpu.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <random>

static std::filesystem::path const projectRoot = PROJECT_ROOT;

struct Immediates {
    math::matrix4f model;
    math::vector4f color;
    uint32_t isSun;
    uint32_t padding[3];
};

template <typename T>
WGPUBuffer loadBuffer(WGPUDevice device, WGPUQueue queue, const std::vector<T>& data, WGPUBufferUsage usage) {
    WGPUBufferDescriptor descriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
    descriptor.size = data.size() * sizeof(T);
    descriptor.usage = usage | WGPUBufferUsage_CopyDst;
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &descriptor);
    wgpuQueueWriteBuffer(queue, buffer, 0, data.data(), descriptor.size);
    return buffer;
}

WGPUShaderModule createShaderModule(WGPUDevice device, std::filesystem::path const &path) {
    auto const source = loadFile(path);

    WGPUShaderSourceWGSL shaderSourceWGSL = WGPU_SHADER_SOURCE_WGSL_INIT;
    shaderSourceWGSL.code = {source.data(), source.size()};

    WGPUShaderModuleDescriptor shaderModuleDescriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderModuleDescriptor.nextInChain = &shaderSourceWGSL.chain;

    return wgpuDeviceCreateShaderModule(device, &shaderModuleDescriptor);
}

WGPURenderPipeline createPipeline(WGPUDevice device, WGPUShaderModule shaderModule,
                                  WGPUTextureFormat surfaceFormat, WGPUBindGroupLayout cameraLayout) {
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDescriptor.immediateSize = sizeof(Immediates);
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &cameraLayout;

    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUColorTargetState colorTargetState = WGPU_COLOR_TARGET_STATE_INIT;
    colorTargetState.format = surfaceFormat;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = {"fragmentMain", WGPU_STRLEN};
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTargetState;

    WGPUVertexAttribute attributes[2];
    attributes[0] = WGPU_VERTEX_ATTRIBUTE_INIT;
    attributes[0].offset = offsetof(ObjVertex, position);
    attributes[0].shaderLocation = 0;
    attributes[0].format = WGPUVertexFormat_Float32x3;
    attributes[1] = WGPU_VERTEX_ATTRIBUTE_INIT;
    attributes[1].offset = offsetof(ObjVertex, normal);
    attributes[1].shaderLocation = 1;
    attributes[1].format = WGPUVertexFormat_Float32x3;

    WGPUVertexBufferLayout vertexLayout = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    vertexLayout.arrayStride = sizeof(ObjVertex);
    vertexLayout.stepMode = WGPUVertexStepMode_Vertex;
    vertexLayout.attributeCount = 2;
    vertexLayout.attributes = attributes;

    WGPUDepthStencilState depthState = WGPU_DEPTH_STENCIL_STATE_INIT;
    depthState.format = WGPUTextureFormat_Depth24Plus;
    depthState.depthWriteEnabled = WGPUOptionalBool_True;
    depthState.depthCompare = WGPUCompareFunction_Less;

    WGPURenderPipelineDescriptor renderPipelineDescriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    renderPipelineDescriptor.layout = pipelineLayout;
    renderPipelineDescriptor.vertex.module = shaderModule;
    renderPipelineDescriptor.vertex.entryPoint = {"vertexMain", WGPU_STRLEN};
    renderPipelineDescriptor.vertex.bufferCount = 1;
    renderPipelineDescriptor.vertex.buffers = &vertexLayout;
    renderPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    renderPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
    renderPipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;
    renderPipelineDescriptor.fragment = &fragmentState;
    renderPipelineDescriptor.depthStencil = &depthState;

    WGPURenderPipeline renderPipeline = wgpuDeviceCreateRenderPipeline(device, &renderPipelineDescriptor);
    wgpuPipelineLayoutRelease(pipelineLayout);

    return renderPipeline;
}

WGPUBuffer createUniformBuffer(WGPUDevice device) {
    WGPUBufferDescriptor descriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
    descriptor.size = sizeof(math::matrix4f);
    descriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    return wgpuDeviceCreateBuffer(device, &descriptor);
}

WGPUBindGroupLayout createCameraBindGroupLayout(WGPUDevice device) {
    WGPUBufferBindingLayout bindingLayout = WGPU_BUFFER_BINDING_LAYOUT_INIT;
    bindingLayout.type = WGPUBufferBindingType_Uniform;
    bindingLayout.minBindingSize = sizeof(math::matrix4f);

    WGPUBindGroupLayoutEntry layoutEntry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Vertex;
    layoutEntry.buffer = bindingLayout;

    WGPUBindGroupLayoutDescriptor layoutDescriptor = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    layoutDescriptor.entryCount = 1;
    layoutDescriptor.entries = &layoutEntry;
    return wgpuDeviceCreateBindGroupLayout(device, &layoutDescriptor);
}

WGPUBindGroup createBindGroup(WGPUDevice device, WGPUBindGroupLayout layout, WGPUBuffer buffer) {
    WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
    entry.binding = 0;
    entry.buffer = buffer;
    entry.size = sizeof(math::matrix4f);

    WGPUBindGroupDescriptor descriptor = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    descriptor.layout = layout;
    descriptor.entryCount = 1;
    descriptor.entries = &entry;
    return wgpuDeviceCreateBindGroup(device, &descriptor);
}

int main() try {
    WgpuApp app("Homework 1", 1280, 720, true);

    // for convenient moving
    app.grab_mouse(true);

    WGPUShaderModule shaderModule = createShaderModule(app.device(), projectRoot / "shader.wgsl");
    WGPUBindGroupLayout cameraLayout = createCameraBindGroupLayout(app.device());
    WGPURenderPipeline renderPipeline = createPipeline(app.device(), shaderModule, app.surfaceFormat(), cameraLayout);

    ObjMesh const sphere = generateSphere(32, 16);
    WGPUBuffer vertexBuffer = loadBuffer(app.device(), app.queue(), sphere.vertices, WGPUBufferUsage_Vertex);
    WGPUBuffer indexBuffer = loadBuffer(app.device(), app.queue(), sphere.indices, WGPUBufferUsage_Index);

    WGPUBuffer cameraMatrixBuffer = createUniformBuffer(app.device());
    WGPUBindGroup cameraBindGroup = createBindGroup(app.device(), cameraLayout, cameraMatrixBuffer);

    const auto seed = std::random_device{}();
    std::cout << "Scene seed: " << seed << std::endl;
    const auto planets = generateScene(seed);

    WGPUTexture depthBuffer = nullptr;
    WGPUTextureView depthView = nullptr;

    auto lastFrameStart = std::chrono::high_resolution_clock::now();
    float time = 0.f;

    Camera camera;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            camera.handleEvent(event);
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                app.resize(event.window.data1, event.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN:
                // way to close the app with cursor pinned
                if (event.key.key == SDLK_ESCAPE) running = false;
                break;
            }
        }

        if (!running) break;

        std::optional<WGPUSurfaceTexture> surfaceTexture = app.beginFrame();
        if (!surfaceTexture) {
            continue;
        }

        const bool resized = !depthBuffer || wgpuTextureGetWidth(depthBuffer) != app.width() || wgpuTextureGetHeight(depthBuffer) != app.height();

        if (resized) {
            if (depthView) wgpuTextureViewRelease(depthView);
            if (depthBuffer) wgpuTextureRelease(depthBuffer);

            WGPUExtent3D extent = WGPU_EXTENT_3D_INIT;
            extent.width = app.width();
            extent.height = app.height();

            WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
            descriptor.dimension = WGPUTextureDimension_2D;
            descriptor.size = extent;
            descriptor.format = WGPUTextureFormat_Depth24Plus;
            descriptor.usage = WGPUTextureUsage_RenderAttachment;

            depthBuffer = wgpuDeviceCreateTexture(app.device(), &descriptor);
            depthView = wgpuTextureCreateView(depthBuffer, nullptr);
        }

        auto const now = std::chrono::high_resolution_clock::now();
        float const dt = std::chrono::duration<float>(now - lastFrameStart).count();
        time += dt;
        lastFrameStart = now;

        camera.update(dt);
        math::matrix4f const view = camera.viewMatrix();
        math::matrix4f const projection = camera.projectionMatrix(static_cast<float>(app.width()) / app.height());

        const auto viewProjection = math::transpose(projection * view);
        wgpuQueueWriteBuffer(app.queue(), cameraMatrixBuffer, 0, &viewProjection, sizeof(viewProjection));

        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture->texture, nullptr);

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(app.device(), nullptr);

        WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        colorAttachment.view = targetView;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.01, 0.02, 0.03, 1.0};

        WGPURenderPassDepthStencilAttachment depthAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
        depthAttachment.view = depthView;
        depthAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment.depthStoreOp = WGPUStoreOp_Discard;
        depthAttachment.depthClearValue = 1.f;

        WGPURenderPassDescriptor renderPassDescriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &colorAttachment;
        renderPassDescriptor.depthStencilAttachment = &depthAttachment;
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);

        wgpuRenderPassEncoderSetPipeline(renderPass, renderPipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, cameraBindGroup, 0, nullptr);

        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);

        for (std::size_t i = 0; i < planets.size(); ++i) {
            const auto &planet = planets[i];

            Immediates const immediates{
                .model = math::transpose(planet.modelMatrix(time)),
                .color = planet.color,
                .isSun = i == 0 ? 1u : 0u,
            };
            wgpuRenderPassEncoderSetImmediates(renderPass, 0, &immediates, sizeof(immediates));
            wgpuRenderPassEncoderDrawIndexed(renderPass, static_cast<uint32_t>(sphere.indices.size()), 1, 0, 0, 0);
        }

        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(app.queue(), 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);

        wgpuSurfacePresent(app.surface());

        wgpuTextureViewRelease(targetView);
        wgpuTextureRelease(surfaceTexture->texture);
    }

    if (depthView) wgpuTextureViewRelease(depthView);
    if (depthBuffer) wgpuTextureRelease(depthBuffer);

    wgpuBindGroupRelease(cameraBindGroup);
    wgpuBufferRelease(cameraMatrixBuffer);
    wgpuBindGroupLayoutRelease(cameraLayout);

    wgpuBufferRelease(vertexBuffer);
    wgpuBufferRelease(indexBuffer);

    wgpuRenderPipelineRelease(renderPipeline);
    wgpuShaderModuleRelease(shaderModule);

} catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
