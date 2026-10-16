#include <wgpu_app.hpp>
#include <file_utils.hpp>
#include <obj_loader.hpp>

#include "stars.hpp"
#include "scene.hpp"
#include "colors.hpp"
#include "sphere.hpp"
#include "ring.hpp"
#include "camera.hpp"

#include <webgpu.h>
#include <math/affine.hpp>

#include <chrono>
#include <cmath>
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
    float time;
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

WGPURenderPipeline createStarPipeline(WGPUDevice device, WGPUShaderModule shaderModule, WGPUTextureFormat surfaceFormat) {
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDescriptor.immediateSize = sizeof(math::matrix4f);

    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUColorTargetState colorTargetState = WGPU_COLOR_TARGET_STATE_INIT;
    colorTargetState.format = surfaceFormat;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = {"fragmentMain", WGPU_STRLEN};
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTargetState;

    WGPUVertexAttribute positionAttribute = WGPU_VERTEX_ATTRIBUTE_INIT;
    positionAttribute.offset = offsetof(ObjVertex, position);
    positionAttribute.shaderLocation = 0;
    positionAttribute.format = WGPUVertexFormat_Float32x3;

    WGPUVertexAttribute instanceAttributes[2];
    instanceAttributes[0] = WGPU_VERTEX_ATTRIBUTE_INIT;
    instanceAttributes[0].offset = offsetof(StarInstance, radius);
    instanceAttributes[0].shaderLocation = 1;
    instanceAttributes[0].format = WGPUVertexFormat_Float32;
    instanceAttributes[1] = WGPU_VERTEX_ATTRIBUTE_INIT;
    instanceAttributes[1].offset = offsetof(StarInstance, center);
    instanceAttributes[1].shaderLocation = 2;
    instanceAttributes[1].format = WGPUVertexFormat_Float32x3;

    WGPUVertexBufferLayout vertexLayouts[2];
    vertexLayouts[0] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    vertexLayouts[0].arrayStride = sizeof(ObjVertex);
    vertexLayouts[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexLayouts[0].attributeCount = 1;
    vertexLayouts[0].attributes = &positionAttribute;
    vertexLayouts[1] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    vertexLayouts[1].arrayStride = sizeof(StarInstance);
    vertexLayouts[1].stepMode = WGPUVertexStepMode_Instance;
    vertexLayouts[1].attributeCount = 2;
    vertexLayouts[1].attributes = instanceAttributes;

    WGPURenderPipelineDescriptor renderPipelineDescriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    renderPipelineDescriptor.layout = pipelineLayout;
    renderPipelineDescriptor.vertex.module = shaderModule;
    renderPipelineDescriptor.vertex.entryPoint = {"vertexMain", WGPU_STRLEN};
    renderPipelineDescriptor.vertex.bufferCount = 2;
    renderPipelineDescriptor.vertex.buffers = vertexLayouts;
    renderPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    renderPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
    renderPipelineDescriptor.primitive.cullMode = WGPUCullMode_Back;
    renderPipelineDescriptor.fragment = &fragmentState;

    WGPURenderPipeline renderPipeline = wgpuDeviceCreateRenderPipeline(device, &renderPipelineDescriptor);
    wgpuPipelineLayoutRelease(pipelineLayout);

    return renderPipeline;
}

WGPURenderPipeline createPipeline(WGPUDevice device, WGPUShaderModule shaderModule,
                                  WGPUTextureFormat surfaceFormat, WGPUBindGroupLayout cameraLayout,
                                  bool ringFragment = false,
                                  WGPUCullMode cullMode = WGPUCullMode_Back,
                                  bool alphaBlend = false,
                                  bool vertexColors = true) {
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDescriptor.immediateSize = sizeof(Immediates);
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &cameraLayout;

    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;
    blend.color = {WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha};
    blend.alpha = {WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};

    WGPUColorTargetState colorTargetState = WGPU_COLOR_TARGET_STATE_INIT;
    colorTargetState.format = surfaceFormat;
    colorTargetState.writeMask = WGPUColorWriteMask_All;
    colorTargetState.blend = alphaBlend ? &blend : nullptr;

    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = {ringFragment ? "ringFragmentMain" : "fragmentMain", WGPU_STRLEN};
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

    WGPUVertexAttribute colorAttribute = WGPU_VERTEX_ATTRIBUTE_INIT;
    colorAttribute.shaderLocation = 2;
    colorAttribute.format = WGPUVertexFormat_Float32x4;

    WGPUVertexBufferLayout vertexLayouts[2];
    vertexLayouts[0] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    vertexLayouts[0].arrayStride = sizeof(ObjVertex);
    vertexLayouts[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexLayouts[0].attributeCount = 2;
    vertexLayouts[0].attributes = attributes;
    vertexLayouts[1] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    vertexLayouts[1].arrayStride = sizeof(math::vector4f);
    vertexLayouts[1].stepMode = WGPUVertexStepMode_Vertex;
    vertexLayouts[1].attributeCount = 1;
    vertexLayouts[1].attributes = &colorAttribute;

    WGPUDepthStencilState depthState = WGPU_DEPTH_STENCIL_STATE_INIT;
    depthState.format = WGPUTextureFormat_Depth24Plus;
    depthState.depthWriteEnabled = WGPUOptionalBool_True;
    depthState.depthCompare = WGPUCompareFunction_Less;

    WGPURenderPipelineDescriptor renderPipelineDescriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    renderPipelineDescriptor.layout = pipelineLayout;
    renderPipelineDescriptor.vertex.module = shaderModule;
    renderPipelineDescriptor.vertex.entryPoint = {vertexColors ? "vertexMain" : "uncoloredVertexMain", WGPU_STRLEN};
    renderPipelineDescriptor.vertex.bufferCount = vertexColors ? 2 : 1;
    renderPipelineDescriptor.vertex.buffers = vertexLayouts;
    renderPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    renderPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
    renderPipelineDescriptor.primitive.cullMode = cullMode;
    renderPipelineDescriptor.fragment = &fragmentState;
    renderPipelineDescriptor.depthStencil = &depthState;

    WGPURenderPipeline renderPipeline = wgpuDeviceCreateRenderPipeline(device, &renderPipelineDescriptor);
    wgpuPipelineLayoutRelease(pipelineLayout);

    return renderPipeline;
}

math::matrix4f glowModelMatrix(const math::vector3f &cameraPosition, const math::vector3f &center, float radius) {
    const auto toCamera = cameraPosition - center;
    const auto y = math::length_squared(toCamera) > 1e-8f
                 ? math::normalized(toCamera) : math::vector3f{0.f, 0.f, 1.f};
    const auto reference = std::abs(y[1]) > 0.999f
                         ? math::vector3f{0.f, 0.f, 1.f} : math::vector3f{0.f, 1.f, 0.f};
    const auto x = math::normalized(math::cross(reference, y));
    const auto z = math::cross(x, y);

    return {
        radius * x[0], radius * y[0], radius * z[0], center[0],
        radius * x[1], radius * y[1], radius * z[1], center[1],
        radius * x[2], radius * y[2], radius * z[2], center[2],
        0.f, 0.f, 0.f, 1.f,
    };
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

    const auto seed = std::random_device{}();
    std::cout << "Scene seed: " << seed << std::endl;

    // initialization for stars
    const auto stars = generateStars(seed);

    WGPUShaderModule starsShaderModule = createShaderModule(app.device(), projectRoot / "stars.wgsl");
    WGPURenderPipeline starsRenderPipeline = createStarPipeline(app.device(), starsShaderModule, app.surfaceFormat());

    const ObjMesh starSphere = generateSphere(8, 12);
    WGPUBuffer starVertexBuffer = loadBuffer(app.device(), app.queue(), starSphere.vertices, WGPUBufferUsage_Vertex);
    WGPUBuffer starIndexBuffer = loadBuffer(app.device(), app.queue(), starSphere.indices, WGPUBufferUsage_Index);
    WGPUBuffer starInstanceBuffer = loadBuffer(app.device(), app.queue(), stars, WGPUBufferUsage_Vertex);

    // pipeline of drawing planets and sun + separate for rings
    WGPUShaderModule shaderModule = createShaderModule(app.device(), projectRoot / "shader.wgsl");
    WGPUBindGroupLayout cameraLayout = createCameraBindGroupLayout(app.device());
    WGPURenderPipeline renderPipeline = createPipeline(app.device(), shaderModule, app.surfaceFormat(), cameraLayout);

    WGPURenderPipeline sunRenderPipeline = createPipeline(app.device(), shaderModule, app.surfaceFormat(),
                                                          cameraLayout, false, WGPUCullMode_Back, false, false);

    WGPURenderPipeline ringRenderPipeline = createPipeline(app.device(), shaderModule, app.surfaceFormat(),
                                                           cameraLayout, true, WGPUCullMode_None, true, false);

    const ObjMesh ringMesh = generateRing(1.4f, 2.4f);
    WGPUBuffer ringVertexBuffer = loadBuffer(app.device(), app.queue(), ringMesh.vertices, WGPUBufferUsage_Vertex);
    WGPUBuffer ringIndexBuffer = loadBuffer(app.device(), app.queue(), ringMesh.indices, WGPUBufferUsage_Index);

    const ObjMesh glowMesh = generateRing(0.95f, 1.2f, ringMesh.vertices.size() / 2);
    WGPUBuffer glowVertexBuffer = loadBuffer(app.device(), app.queue(), glowMesh.vertices, WGPUBufferUsage_Vertex);

    const ObjMesh celestialSphere = generateSphere(32, 16);
    WGPUBuffer celestialVertexBuffer = loadBuffer(app.device(), app.queue(), celestialSphere.vertices, WGPUBufferUsage_Vertex);
    WGPUBuffer celestialIndexBuffer = loadBuffer(app.device(), app.queue(), celestialSphere.indices, WGPUBufferUsage_Index);

    WGPUBuffer cameraMatrixBuffer = createUniformBuffer(app.device());
    WGPUBindGroup cameraBindGroup = createBindGroup(app.device(), cameraLayout, cameraMatrixBuffer);

    const auto planets = generateScene(seed);

    // sun we color in shader procedural
    std::vector<WGPUBuffer> colorBuffers(planets.size(), nullptr);
    for (std::size_t i = 1; i < planets.size(); ++i) {
        std::vector<math::vector4f> colors;
        colors.reserve(celestialSphere.vertices.size());
        for (const auto &vertex : celestialSphere.vertices) {
            colors.push_back(bodyVertexColor(vertex.position, planets[i].color, planets[i].materialSeed));
        }
        colorBuffers[i] = loadBuffer(app.device(), app.queue(), colors, WGPUBufferUsage_Vertex);
    }

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

        // STARS PIPELINE

        {
            WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            colorAttachment.view = targetView;
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.storeOp = WGPUStoreOp_Store;
            colorAttachment.clearValue = {0.005, 0.01, 0.015, 1.0}; // celestial color

            WGPURenderPassDescriptor renderPassDescriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
            renderPassDescriptor.colorAttachmentCount = 1;
            renderPassDescriptor.colorAttachments = &colorAttachment;

            WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);

            wgpuRenderPassEncoderSetPipeline(renderPass, starsRenderPipeline);

            // Removing camera translation keeps the background infinitely distant.
            auto starView = view;
            starView[0][3] = 0.f;
            starView[1][3] = 0.f;
            starView[2][3] = 0.f;
            const auto starViewProjection = math::transpose(projection * starView);
            wgpuRenderPassEncoderSetImmediates(renderPass, 0, &starViewProjection, sizeof(starViewProjection));

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, starVertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, starIndexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, starInstanceBuffer, 0, WGPU_WHOLE_SIZE);

            wgpuRenderPassEncoderDrawIndexed(renderPass, starSphere.indices.size(), stars.size(), 0, 0, 0);

            wgpuRenderPassEncoderEnd(renderPass);
            wgpuRenderPassEncoderRelease(renderPass);
        }

        // MAIN PIPELINE

        {
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

            WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            colorAttachment.view = targetView;
            colorAttachment.loadOp = WGPULoadOp_Load;
            colorAttachment.storeOp = WGPUStoreOp_Store;

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

            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, cameraBindGroup, 0, nullptr);

            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, celestialVertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, celestialIndexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);

            for (std::size_t i = 0; i < planets.size(); ++i) {
                const auto &planet = planets[i];

                Immediates const immediates{
                    .model = math::transpose(planet.modelMatrix(time, planets)),
                    .color = planet.color,
                    .isSun = i == 0 ? 1u : 0u,
                    .time = time,
                };
                wgpuRenderPassEncoderSetPipeline(renderPass, i == 0 ? sunRenderPipeline : renderPipeline);
                if (i != 0) wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, colorBuffers[i], 0, WGPU_WHOLE_SIZE);
                wgpuRenderPassEncoderSetImmediates(renderPass, 0, &immediates, sizeof(immediates));
                wgpuRenderPassEncoderDrawIndexed(renderPass, celestialSphere.indices.size(), 1, 0, 0, 0);
            }

            {
                const auto &ringPlanet = planets.at(7);
                const auto ringModel = math::translation(ringPlanet.position(time, planets))
                                     * math::rotation_yz(0.25f)
                                     * math::scale(math::vector3f{ringPlanet.radius, ringPlanet.radius, ringPlanet.radius});
                Immediates const ringImmediates{
                    .model = math::transpose(ringModel),
                    .color = ringPlanet.color * 0.65f + math::vector4f{0.35f, 0.35f, 0.35f, 0.35f},
                    .isSun = 0,
                };
                wgpuRenderPassEncoderSetPipeline(renderPass, ringRenderPipeline);
                wgpuRenderPassEncoderSetBindGroup(renderPass, 0, cameraBindGroup, 0, nullptr);
                wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, ringVertexBuffer, 0, WGPU_WHOLE_SIZE);
                wgpuRenderPassEncoderSetIndexBuffer(renderPass, ringIndexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
                wgpuRenderPassEncoderSetImmediates(renderPass, 0, &ringImmediates, sizeof(ringImmediates));
                wgpuRenderPassEncoderDrawIndexed(renderPass, ringMesh.indices.size(), 1, 0, 0, 0);
            }

            const auto& sun = planets.at(0);
            const Immediates glowImmediates{
                .model = math::transpose(glowModelMatrix(camera.position(), sun.position(time, planets), sun.radius)),
                .color = {1.f, 0.68f, 0.36f, 1.f},
                .isSun = 1,
            };
            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, glowVertexBuffer, 0, WGPU_WHOLE_SIZE);
            wgpuRenderPassEncoderSetImmediates(renderPass, 0, &glowImmediates, sizeof(glowImmediates));
            wgpuRenderPassEncoderDrawIndexed(renderPass, ringMesh.indices.size(), 1, 0, 0, 0);

            wgpuRenderPassEncoderEnd(renderPass);
            wgpuRenderPassEncoderRelease(renderPass);
        }

        // SUBMITTING

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

    wgpuBufferRelease(celestialVertexBuffer);
    wgpuBufferRelease(celestialIndexBuffer);
    for (auto buffer : colorBuffers) {
        if (buffer) wgpuBufferRelease(buffer);
    }

    wgpuBufferRelease(ringVertexBuffer);
    wgpuBufferRelease(ringIndexBuffer);
    wgpuRenderPipelineRelease(ringRenderPipeline);

    wgpuBufferRelease(glowVertexBuffer);

    wgpuBufferRelease(starInstanceBuffer);
    wgpuBufferRelease(starVertexBuffer);
    wgpuBufferRelease(starIndexBuffer);
    wgpuRenderPipelineRelease(starsRenderPipeline);
    wgpuShaderModuleRelease(starsShaderModule);

    wgpuRenderPipelineRelease(renderPipeline);
    wgpuRenderPipelineRelease(sunRenderPipeline);
    wgpuShaderModuleRelease(shaderModule);

} catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
