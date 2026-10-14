struct Immediates {
    model: mat4x4f,
    color: vec4f,
    isSun: u32,
}

var<immediate> immediates: Immediates;

struct CameraUniforms {
    viewProjection: mat4x4f,
}

@group(0) @binding(0) var<uniform> camera: CameraUniforms;

struct VertexOutput {
    @builtin(position) clipPosition: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) worldNormal: vec3f,
}

@vertex
fn vertexMain(@location(0) position: vec3f, @location(1) normal: vec3f) -> VertexOutput {
    let worldPosition = immediates.model * vec4f(position, 1.0);
    var output: VertexOutput;
    output.clipPosition = camera.viewProjection * worldPosition;
    output.worldPosition = worldPosition.xyz;
    // Valid for the bodies' uniform scales; w = 0 excludes translation.
    output.worldNormal = (immediates.model * vec4f(normal, 0.0)).xyz;
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    if immediates.isSun == 1 {
        return immediates.color;
    }

    let ambient = 0.05;
    let normal = normalize(input.worldNormal);
    let directionToSun = normalize(-input.worldPosition);

    let diffuse = max(dot(normal, directionToSun), 0.0);

    return vec4f(
        immediates.color.rgb * (ambient + diffuse),
        immediates.color.a
    );
}
