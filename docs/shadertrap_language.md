# The ShaderTrap language

TODO(afd): overview.

## Commands

A brief description of each ShaderTrap command now follows. The commands are presented in alphabetical order.

### ASSERT_EQUAL

```
ASSERT_EQUAL BUFFERS buffer1 buffer2
```

```
ASSERT_EQUAL RENDERBUFFERS renderbuffer1 renderbuffer2
```

TODO(afd)

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
