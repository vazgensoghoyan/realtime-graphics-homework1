struct Immediates {
    model: mat4x4f,
    color: vec4f,
}

var<immediate> immediates: Immediates;

struct CameraUniforms {
    viewProjection: mat4x4f,
}

@group(0) @binding(0) var<uniform> camera: CameraUniforms;

@vertex
fn vertexMain(@location(0) position: vec3f, @location(1) normal: vec3f) -> @builtin(position) vec4f {
    return camera.viewProjection * immediates.model * vec4f(position, 1.0);
}

@fragment
fn fragmentMain() -> @location(0) vec4f {
    return immediates.color;
}
