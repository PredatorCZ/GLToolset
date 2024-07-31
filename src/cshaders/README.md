# New rules

|gc++|glsl|
|-|-|
|operator `^`|operator `^^`|
|`constexpr`|`const`|
|`const`|ignored|
|`static const <type> <name>;`|`uniform <type> <name>;`|
|`static <type> <name>;`|`out <type> <name>;`|
|`static const <type> <name> = <number>;`|`layout(location=<number>) in <type> <name>;`|
|`static <type> <name> = <number>;`|`layout(location=<number>) out <type> <name>;`|

## Attributes

### `locked`

Treat structure as interface block (UBO or SSBO)

### Eaxample 1 (UBO)

gc++:

```cpp
struct [[locked]] {
    float var0;
    vec4 var1;
} const lockedData;
```

glsl output:

```glsl
uniform LockedData {
    float var0;
    vec4 var1;
} lockedData;
```

c++ output:

```cpp
struct LockedData {
    float var0;
    vec4 var1;
};
```

### Eaxample 2 (SSBO)

gc++:

```cpp
struct [[locked]] {
    float var0;
    vec4 var1[]; // Open array, consider SSBO
} const lockedData;
```

glsl output:

```glsl
readonly buffer LockedData {
    float var0;
    vec4 var1[];
} lockedData;
```

c++ output:

```cpp
struct LockedData {
    float var0;
    vec4 var1[1];
};
```

### `binding(<bind index>)`

Implies `locked` attribute.

gc++:

```cpp
struct [[binding(0)]] {
  mat4 projection;
  vec4 view[2];
} const camera;
```

glsl output:

```glsl
layout(binding = 0) uniform Camera {
  mat4 projection;
  vec4 view[2];
} camera;
```

### `disable(<constexpr>)` or `enable(<constexpr>)`

gc++:

```cpp
constexpr bool instance = false;

struct Transforms {
    vec4 tm[2];
};

struct [[locked]] {
  Transforms [[disable(instance)]] transform;
  Transforms [[enable(instance)]] transforms[];
} const instanceTransforms;
```

glsl output:

```glsl
struct Transforms {
    vec4 tm[2];
};

uniform TnstanceTransforms {
    Transforms transform;
} instanceTransforms;
```

c++ output:

```cpp
struct Transforms {
    vec4 tm[2];
};

struct TnstanceTransforms {
    Transforms transform;
};
```

## Uniform gather

Active `static` variables are automatically gathered and single uniform buffer is built.

Gather ordering is based on type alignment, then the order of appearance.

```glsl
uniform {
    mat4,
    mat3,
    vec4,
    vec3, [float, int, uint, sampler],
    vec2,
    float
    int, uint
    sampler
};
```

### Example 1

gc++:

```cpp
constexpr bool useGlow = false;

void fragment() {
  static const sampler2D smAlbedo;
  static const vec2 psTexCoord;

  vec4 albedo = texture(smAlbedo, psTexCoord);
  vec3 normal(0.f, 0.f, 1.f);

  static const float specLevel;
  static const float specPower;
  static const vec3 ambientColor;
  static vec4 fragColor = 0;

  PointLightData lightData =
      FragmentPointLight(GetTSNormal(normal), specLevel, specPower);
  fragColor =
      vec4(lightData.color + lightData.specColor + ambientColor, 1.f) * albedo;

  if constexpr (useGlow) {
    static const sampler2D smGlow;
    static const float glowLevel;
    static vec4 glowColor = 1;

    vec4 glow = texture(smGlow, psTexCoord);
    glowColor = glow * vec4(glowLevel, glowLevel, glowLevel, 1) * glow.x;
  }
}
```

glsl output (in case `useGlow = false`):

```glsl
uniform FragmentProperties {
  vec3 ambientColor;
  float specPower;
  float specLevel;
  sampler2D smAlbedo;
};

layout(location=0) out vec4 fragColor;
in vec2 psTexCoord;

// rest of the code
// ...
```

c++ output:

```cpp
struct FragmentProperties {
  vec3 ambientColor;
  float specPower;
  float specLevel;
  uint32_t smAlbedo;
};
```

glsl output (in case `useGlow = true`):

```glsl
uniform FragmentProperties {
  vec3 ambientColor;
  float specPower;
  float specLevel;
  float glowLevel;
  sampler2D smAlbedo;
  sampler2D smGlow;
};

layout(location=0) out vec4 fragColor;
layout(location=1) out vec4 glowColor;
in vec2 psTexCoord;

// rest of the code
// ...
```

c++ output:

```cpp
struct FragmentProperties {
  vec3 ambientColor;
  float specPower;
  float specLevel;
  float glowLevel;
  uint32_t smAlbedo;
  uint32_t smGlow;
};
