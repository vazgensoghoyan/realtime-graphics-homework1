struct Immediates {
    model: mat4x4f,
    color: vec4f,
    isSun: u32,
    time: f32,
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
    @location(2) localPosition: vec3f,
    @location(3) color: vec4f,
}

fn transformVertex(position: vec3f, normal: vec3f, color: vec4f) -> VertexOutput {
    var output: VertexOutput;

    let worldPosition = immediates.model * vec4f(position, 1.0);

    output.clipPosition = camera.viewProjection * worldPosition;
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = (immediates.model * vec4f(normal, 0.0)).xyz;

    output.localPosition = position;
    output.color = color;

    return output;
}

@vertex
fn vertexMain(@location(0) position: vec3f, @location(1) normal: vec3f,
              @location(2) color: vec4f) -> VertexOutput {
    return transformVertex(position, normal, color);
}

@vertex
fn uncoloredVertexMain(@location(0) position: vec3f, @location(1) normal: vec3f) -> VertexOutput {
    return transformVertex(position, normal, immediates.color);
}

fn hash3(p: vec3f) -> f32 {
    let h = sin(dot(p, vec3f(127.1, 311.7, 74.7)));
    return fract(h * 43758.5453);
}

fn noise3(p: vec3f) -> f32 {
    let cell = floor(p);
    let f = fract(p);

    let u = f * f * (3.0 - 2.0 * f);

    var result = 0.0;
    for (var x = 0; x < 2; x++) {
        for (var y = 0; y < 2; y++) {
            for (var z = 0; z < 2; z++) {
                let corner = vec3f(f32(x), f32(y), f32(z));
                let weight = mix(vec3f(1.0) - u, u, corner);
                result += hash3(cell + corner) * weight.x * weight.y * weight.z;
            }
        }
    }
    return result;
}

fn sunColor(localPosition: vec3f, time: f32) -> vec3f {
    let p = normalize(localPosition);
    let drift = vec3f(0.12, -0.09, 0.07) * time;
    let warp = noise3(p * 3.0 - drift) * 2.0;
    let q = p * 6.0 + drift + vec3f(warp);

    let heat = 0.57 * noise3(q)
             + 0.28 * noise3(q * 2.0 + vec3f(13.7, 9.2, 5.4))
             + 0.15 * noise3(q * 4.0 + vec3f(3.1, 17.8, 11.6));

    let orange = vec3f(1.0, 0.16, 0.015);
    let yellow = vec3f(1.0, 0.72, 0.08);
    let hot = vec3f(1.0, 0.96, 0.65);

    let base = mix(orange, yellow, smoothstep(0.25, 0.65, heat));
    return mix(base, hot, smoothstep(0.60, 0.82, heat));
}

fn sunlightBrightness(worldPosition: vec3f, normal: vec3f) -> f32 {
    let directionToSun = normalize(-worldPosition);
    return 0.2 + max(dot(normalize(normal), directionToSun), 0.0);
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    if immediates.isSun == 1 {
        return vec4f(sunColor(input.localPosition, immediates.time), 1.0);
    }

    return vec4f(
        input.color.rgb * sunlightBrightness(input.worldPosition, input.worldNormal),
        input.color.a
    );
}

@fragment
fn ringFragmentMain(input: VertexOutput, @builtin(front_facing) frontFacing: bool) -> @location(0) vec4f {
    if immediates.isSun == 1 {
        let radius = length(input.localPosition.xz);
        let direction = normalize(input.localPosition.xz);
        let rays = smoothstep(0.25, 0.75, noise3(vec3f(direction * 10.0, 4.0)));
        let fade = 1.0 - smoothstep(1.0, 1.2, radius);
        let opacity = 0.65 * fade * mix(0.15, 1.0, rays);
        return vec4f(immediates.color.rgb, opacity);
    }

    var normal = input.worldNormal;
    if !frontFacing {
        normal = -normal;
    }

    return vec4f(immediates.color.rgb * sunlightBrightness(input.worldPosition, normal), immediates.color.a);
}
