

struct Immediates {
    viewProjection: mat4x4f,
    color: vec4f,
}

var<immediate> immediates: Immediates;

@vertex
fn vertexMain(@location(0) position: vec3f) -> @builtin(position) vec4f {
    return immediates.viewProjection * vec4f(position, 1.0);
}

@fragment
fn fragmentMain() -> @location(0) vec4f {
    return immediates.color;
}
