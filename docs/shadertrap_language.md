# The ShaderTrap language

ShaderTrap is a scripting language that aims to make it easy to write tests for OpenGL ES shader compilers, in the form of graphics pipelines that perform off screen rendering using vertex and fragment shaders, and compute pipelines. The language exposes a variety of OpenGL ES features but does not aim to expose them in their full generality. In particular the range of formats that can be used in textures and renderbuffers is limited. The aim is to provide enough control that a wide range of shader compiler tests can be written, without providing so much control that the language becomes as complex as the underlying API itself.

The current prototype is hard-coded to use OpenGL ES 3.2, but future versions may support some older OpenGL ES versions, and possibly some OpenGL versions.

## Commands

Every ShaderTrap command starts with a name, in all-caps `SNAKE_CASE`. Examples are `CREATE_PROGRAM`, `SET_UNIFORM` and `CREATE_RENDERBUFFER`.

Some commands produce a *result*, e.g. `CREATE_PROGRAM` yields a compiled shader, while `CREATE_RENDERBUFFER` yields a renderbuffer. A result identifier always directly follow the command name, and should use no-caps `snake_case`.

There is a single global scope for result identifiers, and the same result identifier cannot be produced by more than one command - i.e. ShaderTrap uses static single assignment form.

Most commands take one or more parameters. Each parameter has a name, in all-caps `SNAKE_CASE`, and is followed by an appropriate sequence of values, the format of which depend on the parameter. For example, `CREATE_PROGRAM` has a `SHADERS` parameter, which is followed by the result identifiers of the compiled shader or shaders from which a program should be created. When a command accepts multiple parameters it does not matter in which order the parameters appear. For example, `SET_UNIFORM` has parameters `PROGRAM`, `LOCATION`, `TYPE` and `VALUES` to specify the program in which the uniform should be set, the location of the uniform within that program, the type of the uniform, and the series of values used to populate that uniform. The following two instances of this command are equivalent:

```
# Sets the vec2 uniform at location 1 in my_prog to (1.0, 2.0)
SET_UNIFORM LOCATION 1 TYPE vec2 VALUES 1.0 2.0 PROGRAM my_prog

# Also sets the vec2 uniform at location 1 in my_prog to (1.0, 2.0)
SET_UNIFORM PROGRAM my_prog VALUES 1.0 2.0 TYPE vec2 LOCATION 1
```

A number of complete ShaderTrap programs are provided in the `examples` directory.

A brief description of each ShaderTrap command now follows. The commands are presented in alphabetical order.

### ASSERT_EQUAL

Checks whether two buffers or two renderbuffers are identical.

The form for buffers is:

```
ASSERT_EQUAL BUFFERS buffer1 buffer2
```

where `buffer1` and `buffer2` must be buffers produced by `CREATE_BUFFER`. Performs a byte-level comparison of the buffers. Yields an error if the buffers have different sizes, or if they have the same size but differ at any single byte.

```
ASSERT_EQUAL RENDERBUFFERS renderbuffer1 renderbuffer2
```

where `renderbuffer1` and `renderbuffer2` must be renderbuffers produced by `CREATE_RENDERBUFFER`. Performs a pixel-by-pixel comparison of the renderbuffers. Yields an error if the renderbuffers have different widths or different heights, or if they have the same dimensions but differ at any single pixel.

### ASSERT_PIXELS

```
ASSERT_PIXELS RENDERBUFFER renderbuffer RECTANGLE x y w h EXPECTED r g b a
```

TODO(afd)

### ASSERT_SIMILAR_EMD_HISTOGRAM

```
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS renderbuffer1 renderbuffer2 TOLERANCE value
```

TODO(afd)

### BIND_SAMPLER

```
BIND_SAMPLER SAMPLER sampler TEXTURE_UNIT unit
```

TODO(afd)

### BIND_STORAGE_BUFFER

```
BIND_STORAGE_BUFFER BUFFER buffer BINDING binding
```

TODO(afd)

### BIND_TEXTURE

```
BIND_TEXTURE TEXTURE texture TEXTURE_UNIT unit
```

TODO(afd)

### BIND_UNIFORM_BUFFER

```
BIND_UNIFORM_BUFFER BUFFER buffer BINDING binding
```

TODO(afd)

### COMPILE_SHADER

```
COMPILE_SHADER result SHADER shader
```

TODO(afd)

### CREATE_BUFFER

```
CREATE_BUFFER result SIZE_BYTES size INIT_TYPE type INIT VALUES value+
```

TODO(afd)

### CREATE_EMPTY_TEXTURE_2D

```
CREATE_EMPTY_TEXTURE_2D result WIDTH w HEIGHT h
```

TODO(afd)

### CREATE_PROGRAM

```
CREATE_PROGRAM result SHADERS shader1 shader2?
```

TODO(afd)

### CREATE_RENDERBUFFER

```
CREATE_RENDERBUFFER result WIDTH w HEIGHT h
```

TODO(afd)

### CREATE_SAMPLER

```
CREATE_SAMPLER result
```

TODO(afd)

### DECLARE_SHADER

```
DECLARE_SHADER result VERTEX|FRAGMENT|COMPUTE
// GLSL code
END
```

TODO(afd)

### DUMP_RENDERBUFFER

```
DUMP_RENDERBUFFER RENDERBUFFER renderbuffer FILE file
```

TODO(afd)

### RUN_COMPUTE

```
RUN_COMPUTE PROGRAM compute_program NUM_GROUPS x y z
```

TODO(afd)

### RUN_GRAPHICS

```
RUN_GRAPHICS
  PROGRAM graphics_program
  VERTEX_DATA
    [ attribute_index_1 -> BUFFER vertex_buffer_1 OFFSET_BYTES offset_1 STRIDE_BYTES stride_1 DIMENSION dimension_1,
      attribute_index_2 -> BUFFER vertex_buffer_2 OFFSET_BYTES offset_2 STRIDE_BYTES stride_2 DIMENSION dimension_2,
      ...
      attribute_index_n -> BUFFER vertex_buffer_n OFFSET_BYTES offset_n STRIDE_BYTES stride_n DIMENSION dimension_n,
    ]
  INDEX_DATA index_buffer
  VERTEX_COUNT count
  TOPOLOGY topology
  FRAMEBUFFER_ATTACHMENTS
    [ location_1 -> attachment_1,
      location_2 -> attachment_2,
      ...
      location_n -> attachment_n
    ]
```

TODO(afd)

### SET_SAMPLER_PARAMETER

```
SET_SAMPLER_PARAMETER SAMPLER sampler PARAMETER parameter VALUE value
```

TODO(afd)

### SET_TEXTURE_PARAMETER

```
SET_TEXTURE_PARAMETER TEXTURE texture PARAMETER parameter VALUE value
```

TODO(afd)

### SET_UNIFORM

```
SET_UNIFORM PROGRAM program LOCATION location TYPE type VALUES value+
```

```
SET_UNIFORM PROGRAM program NAME name TYPE type VALUES value+
```

TODO(afd)
