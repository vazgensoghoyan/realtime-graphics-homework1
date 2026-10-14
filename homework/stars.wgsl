struct Immediates {
    viewProjection: mat4x4f,
}

var<immediate> immediates: Immediates;

@vertex
fn vertexMain(@location(0) position: vec3f,
              @location(1) radius: f32,
              @location(2) center: vec3f) -> @builtin(position) vec4f
{
    return immediates.viewProjection * vec4f(center + position * radius, 1.0);
}

@fragment
fn fragmentMain() -> @location(0) vec4f {
    return vec4f(1.0, 1.0, 1.0, 1.0);
}
