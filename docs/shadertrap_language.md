# The ShaderTrap language

ShaderTrap is a scripting language that aims to make it easy to write tests for OpenGL and OpenGL ES shader compilers, in the form of graphics pipelines that perform off screen rendering using vertex and fragment shaders, and compute pipelines. The language exposes a variety of features common to OpenGL and OpenGL ES but does not aim to expose them in their full generality. In particular the range of formats that can be used in textures and renderbuffers is limited. The aim is to provide enough control that a wide range of shader compiler tests can be written, without providing so much control that the language becomes as complex as the underlying API itself.

## Specifying an API version

A ShaderTrap script starts with a statement of the form:

```
API MAJOR.MINOR
```

where:

- `API` is either `GL`, specifying that the OpenGL API should be targeted, or `GLES`, specifying that the OpenGL ES API should be targeted
- `MAJOR` and `MINOR` specify the version of the desired API

Examples:

- `GL 4.0` specifies that OpenGL 4.0 should be targeted
- `GLES 3.1` specifies that OpenGL ES 3.1 should be targeted

ShaderTrap aims to support OpenGL 4.0 through 4.6, and OpenGL ES 2.0, 3.0, 3.1 and 3.2. If you would value support for older API versions please open an issue.

## Commands

Every ShaderTrap command starts with a name, in all-caps `SNAKE_CASE`. Examples are `CREATE_PROGRAM`, `SET_UNIFORM` and `CREATE_RENDERBUFFER`.

Some commands produce a *result*, e.g. `COMPILE_SHADER` yields a compiled shader, while `CREATE_RENDERBUFFER` yields a renderbuffer. A result identifier always directly follows the command name, and should use no-caps `snake_case`. Here is an example of such a command:

```
# Compiles "shader" and returns the compiled shader in "result"
COMPILE_SHADER result SHADER shader
```

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

This command has two forms.

```
ASSERT_EQUAL BUFFERS buffer_1 buffer_2 [FORMAT format_entry_1 ... format_entry_n]
```

Checks whether `buffer_1` and `buffer_2`, which must be buffers produced by `CREATE_BUFFER`, contain equal contents. Performs a byte-level comparison of the buffers. Yields an error if the buffers have different sizes, or if they have the same size but differ at any single byte.

- `buffer_1` and `buffer_2` are the buffers to be compared
- Each `format_entry_i` is either:
    - `SKIP_BYTES count` for some positive integer `count` that must be a multiple of 4
    - `type count` for some positive integer `count`, where `type` is one of `byte`, `int`, `uint` or `float`, and where `count` is a multiple of 4 if `type` is `byte`

`FORMAT` is an optional parameter that specifies the format of the data that will be compared. If `FORMAT` is specified, it should be followed by at least 1 `format_entry_i` component. The data is then  formatted by processing the `format_entry_i` components as follows, with respect to `offset`, a byte offset into the buffer, initialised to 0:

- If `format_entry_i` is `SKIP_BYTES count` then `offset` is incremented by `count`. This allows padding or irrelevant data to be ignored.

- If `format_entry_i` is byte count then count successive bytes from the buffers are compared, starting from position offset, with any byte-level differences reported, after which offset is incremented by `count`.

- If `format_entry_i` is int count then count successive 4-byte sequences from the buffers are compared, starting from position offset, after which offset is incremented by 4* `count`. Any mismatches between 4-byte sequences are reported as differences between 32-bit signed integers.

- If format_entry_i is `uint count` then the effect is the same as for the `int` case, except that mismatches are reported as differences between 32-bit unsigned integers.

- If `format_entry_i` is `float count` then the effect is the same as for the `int` case, except that mismatches are reported as differences between 32-bit floating-point values. Note that this does not involve comparing floating-point numbers: comparisons are made at the byte level.

The sum of `count` for all `byte` and `SKIP_BYTES` entries, plus the sum of 4*`count` for all `int`, `uint` and `float` entries, must equal the buffer size in bytes - i.e., every byte in the buffer must be accounted for.

If `FORMAT` is not explicitly specified in the command we assumed it to be `FORMAT byte n` where `n` is the size of `buffer_1` and `buffer_2`.
```
ASSERT_EQUAL RENDERBUFFERS renderbuffer_1 renderbuffer_2
```

Checks whether `renderbuffer_1` and `renderbuffer_2`, which must be renderbuffers produced by `CREATE_RENDERBUFFER`, contain equal contents. Performs a pixel-by-pixel comparison of the renderbuffers. Yields an error if the renderbuffers have different widths or different heights, or if they have the same dimensions but differ at any single pixel.

### ASSERT_PIXELS

```
ASSERT_PIXELS RENDERBUFFER renderbuffer RECTANGLE x y w h EXPECTED r g b a
```

Checks whether every pixel in a particular rectangular region of a renderbuffer has a specific value.
- `renderbuffer` must be a renderbuffer produced by `CREATE_RENDERBUFFER`
- `x`, `y`, `w`, `h` are non-negative integers that define a rectangle with top-left coordinate (`x`, `y`), width `w` and height `h` that is required to be within the bounds of `renderbuffer`
- `r`, `g`, `b` and `a` are integer values in the range [0, 255] that define the expected value for every pixel in the rectangular region

### ASSERT_SIMILAR_EMD_HISTOGRAM

```
ASSERT_SIMILAR_EMD_HISTOGRAM RENDERBUFFERS renderbuffer_1 renderbuffer_2 TOLERANCE value
```

Checks whether two renderbuffers are similar according to the [Earth Mover's Distance](https://en.wikipedia.org/wiki/Earth_mover%27s_distance) metric.

- `renderbuffer_1` and `renderbuffer_2` must be renderbuffers produced by `CREATE_RENDERBUFFER`
- `value` must be a floating-point number in the range [0, 1] specifying the tolerance to be used for the comparison. It is a unit-less number.

The following description of Earth Mover's Distance is taken from the [Amber source code](https://github.com/google/amber/blob/dabae26164714abf951c6815a2b4513260f7c6a4/src/buffer.cc#L230):

  >Earth movers's distance: Calculate the minimal cost of moving "earth" to
  >transform the first histogram into the second, where each bin of the
  >histogram can be thought of as a column of units of earth. The cost is the
  >amount of earth moved times the distance carried (the distance is the
  >number of adjacent bins over which the earth is carried). Calculate this
  >using the cumulative difference of the bins, which works as long as both
  >histograms have the same amount of earth. Sum the absolute values of the
  >cumulative difference to get the final cost of how much (and how far) the
  >earth was moved.

### BIND_SAMPLER

```
BIND_SAMPLER SAMPLER sampler TEXTURE_UNIT unit
```

Binds a sampler to a texture unit.
- `sampler` is a sampler produced by `CREATE_SAMPLER`
- `unit` is a non-negative integer specifying which texture unit `sampler` should be bound to

To make a sampler uniform in a shader use the texture unit `unit` to which this sampler has been bound, use `SET_UNIFORM` (supplying the value of `unit` as the value of the uniform).

### BIND_SHADER_STORAGE_BUFFER

Requires API level to be at least OpenGL 4.3 or OpenGL ES 3.1.

```
BIND_STORAGE_BUFFER BUFFER buffer BINDING binding
```

Binds a buffer to a storage buffer binding point.
- `buffer` is a buffer produced by `CREATE_BUFFER`
- `binding` is a non-negative integer specifying a shader storage buffer binding point

### BIND_TEXTURE

```
BIND_TEXTURE TEXTURE texture TEXTURE_UNIT unit
```

Binds a texture to a texture unit.
- `texture` is a texture produced by `CREATE_EMPTY_TEXTURE_2D`
- `unit` is a non-negative integer specifying which texture unit `texture` should be bound to

To make a sampler uniform in a shader use the texture unit `unit` to which this texture has been bound, use `SET_UNIFORM` (supplying the value of `unit` as the value of the uniform).

### BIND_UNIFORM_BUFFER

```
BIND_UNIFORM_BUFFER BUFFER buffer BINDING binding
```

Binds a buffer to a uniform buffer binding point.
- `buffer` is a buffer produced by `CREATE_BUFFER`
- `binding` is a non-negative integer specifying a uniform buffer binding point

### COMPILE_SHADER

```
COMPILE_SHADER result SHADER shader
```

Compiles a shader.

- The compiled shader is identified by `result`
- `shader` is the shader to be compiled, and must be produced by `DECLARE_SHADER`

### CREATE_BUFFER

```
CREATE_BUFFER result SIZE_BYTES size INIT_VALUES
  type_1 value_sequence_1
  type_2 value_sequence_2
  ...
  type_n value_sequence_n
```

Creates a buffer of bytes that can be subsequently accessed by the GPU for various purposes, e.g. as a shader storage buffer, uniform buffer, vertex buffer or index buffer. The creation command does not specify for which purpose the buffer will later be used, and the buffer could in fact be used for multiple purposes, e.g. a compute shader could be used to modify the contents of the buffer, after which it could be fed to a graphics pipeline as vertex data.

- The created buffer is identified by `result`
- `size` is the size of the buffer in bytes
- `INIT_VALUES` specifies the initial contents of the buffer as follows:
  - Each of `type_1`, ..., `type_n` is one of `float`, `int`, `uint` or `byte`
  - If `type_i` is `float` then `value_sequence_i` must be a sequence of floating-point literals.
  - If `type_i` is `int` then `value_sequence_i` must be a sequence of 32-bit signed integer literals.
  - If `type_i` is `uint` then `value_sequence_i` must be a sequence of 32-bit unsigned integer literals.
  - If `type_i` is `byte` then `value_sequence_i` must be a sequence of byte literals (values in the range [0, 255]), and the length of `value_sequence_i` must be a multiple of 4

The buffer is populated with the values provided by the value sequence in order. Each `float`, `int` and `uint` value occupies 4 bytes. Each `byte` value occupies 1 byte. There is no padding between elements.

The total number of bytes occupied by the value sequences combined must match `size`. While this makes the `size` parameter technically redundant, requiring the expected size to be specified allows it to be cross-checked against the combined size of the provided values.

### CREATE_EMPTY_TEXTURE_2D

```
CREATE_EMPTY_TEXTURE_2D result WIDTH w HEIGHT h
```

Creates and empty 2D texture, with RGBA pixel format.

- The created texture is identified by `result`
- The texture has width `w`
- The texture has height `h`

Before sampling from the texture during rendering it is necessary to populate the texture, and set texture parameters appropriately.

Populating the texture should be done by rendering to it using `RUN_GRAPHICS`, supplying the texture as a framebuffer attachment via the `FRAMEBUFFER_ATTACHMENTS` parameter of that command.

Setting parameters for the texture can be done using `SET_TEXTURE_PARAMETER`. The texture can be bound to a texture unit via `BIND_TEXTURE`.

There is currently no way to load a texture from a file or to explicitly specify the initial contents of the texture.

### CREATE_PROGRAM

```
CREATE_PROGRAM result SHADERS compiled_shader_1 compiled_shader_2?
```

Creates a program by linking a compiled shader or pair of compiled shaders, each produced by `COMPILE_SHADER`.

- The created program is identified by `result`
- If `compiled_shader_1` was obtained by compiling a compute shader then `compiled_shader_2` must not be present. In this case `result` is a *compute* program.
- If `compiled_shader_1` was obtained by compiling a vertex or fragment shader then `compiled_shader_2` must be present and collectively the two compiled shaders must have been obtained by compiling a vertex and a fragment shader. In this case `result` is a *graphics* program.

### CREATE_RENDERBUFFER

```
CREATE_RENDERBUFFER result WIDTH w HEIGHT h
```

Creates a renderbuffer with undefined contents, using the RGBA8 format.

- The created renderbuffer is identified by `result`
- The renderbuffer has width `w`
- The renderbuffer has height `r`

The renderbuffer can be rendered to by supplying it as a framebuffer attachment to `RUN_GRAPHICS`, via the `FRAMEBUFFER_ATTACHMENTS` parameter of that command. It can be dumped to a file using `DUMP_RENDERBUFFER`.


### CREATE_SAMPLER

```
CREATE_SAMPLER result
```

Creates a sampler identified via `result`. Parameters of the the sampler can then be set via `SET_SAMPLER_PARAMETER`. The sampler can be bound to a texture unit via `BIND_SAMPLER`.

### DECLARE_SHADER

The `COMPUTE` shader kind requires API level to be at least OpenGL 4.3 or OpenGL ES 3.1.

```
DECLARE_SHADER result KIND kind
// GLSL code
END
```

Declares a shader, which can subsequently be compiled. The shader will be validated using [`glslang`](https://github.com/KhronosGroup/glslang).

- The declared shader is identified by `result`
- `kind` specifies the kind of shader this is, and must be one of `VERTEX`, `FRAGMENT` or `COMPUTE`, for a vertex, fragment or compute shader, respectively. Other shader kinds are not supported.
- The shader text is taken from the lines following `DECLARE_SHADER`, up to but not including a line starting with `END`

The reason for separating the declaration and compilation of shaders into two separate commands is that it allows writing a test that compiles the same shader multiple times without having to duplicate the text of the shader.

### DUMP_BUFFER_BINARY

```
DUMP_BUFFER_BINARY BUFFER buffer FILE file
```

Dumps all bytes of a buffer to a binary file.

- `buffer` is the buffer to be dumped, and must be produced by `CREATE_BUFFER`
- `file` is the file to which the buffer data will be written

### DUMP_BUFFER_TEXT

```
DUMP_BUFFER_TEXT BUFFER buffer FILE file FORMAT format_entry_1 ... format_entry_n
```

Dumps a buffer to a text file in a formatted manner.

- `buffer` is the buffer to be dumped, and must be produced by `CREATE_BUFFER`
- `file` is the file to which the formatted buffer data will be written
- Each `format_entry_i` is either:
    - A string literal
    - `SKIP_BYTES count` for some positive integer `count` that must be a multiple of 4
    - `type count` for some positive integer `count`, where `type` is one of `byte`, `int`, `uint` or `float`, and where `count` is a multiple of 4 if `type` is `byte`

The data is formatted by processing the `format_entry_i` components as follows, with respect to `offset`, a byte offset into the buffer, initialised to 0:

- If `format_entry_i` is a string literal, the string literal is output to the file. This is useful for including descriptive text in the output.

- If `format_entry_i` is `SKIP_BYTES count` then offset is incremented by `count`. This allows padding or irrelevant data to be skipped.

- If `format_entry_i` is `byte count` then `count` bytes from the buffer are output as non-negative integers, separated by spaces, starting from position `offset`, after which `offset` is incremented by `count`.

- If `format_entry_i` is `int count` then `count` 32-bit signed integers are output, each constructed from the next 4 bytes in the buffer, separated by spaces, starting from position `offset`, after which `offset` is incremented by 4*`count`.

- If `format_entry_i` is `uint count` then the effect is the same as for the `int` case, except that 32-bit unsigned integers are output.

- If `format_entry_i` is `float count` then the effect is the same as for the `int` case, except that 32-bit floating-point values are output.

The sum of `count` for all `byte` and `SKIP_BYTES` entries, plus the sum of 4*`count` for all `int`, `uint` and `float` entries, must equal the buffer size in bytes - i.e., every byte in the buffer must be accounted for.

### DUMP_RENDERBUFFER

```
DUMP_RENDERBUFFER RENDERBUFFER renderbuffer FILE file
```

Dumps a renderbuffer to a file, in PNG format.

- `renderbuffer` is the renderbuffer to be dumped, and must be produced by `CREATE_RENDERBUFFER`
- `file` is the file to which the PNG will be written

### RUN_COMPUTE

Requires API level to be at least OpenGL 4.3 or OpenGL ES 3.1.

```
RUN_COMPUTE PROGRAM compute_program NUM_GROUPS x y z
```

- `compute_program` must be a *compute* program produced by `CREATE_PROGRAM`
- `x`, `y` and `z` must be non-negative integers specifying the number of work groups in the *x*, *y* and *z* dimensions, respectively, that should execute the compute program

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

Runs a graphics workload.

- `graphics_program` must be a *graphics* program produced by `CREATE_PROGRAM`.
- `VERTEX_DATA` is a mapping with one entry for each vertex attribute associated with the vertex shader of `graphics_program`. If the vertex shader declares an `in` variable with `layout(location = i)` then `VERTEX_DATA` must provide an entry of the form `i -> ...`.  In this entry:
  - `vertex_buffer_i` specifies a buffer from which vertex data for vertex attribute `i` will be taken. This must be produced by `CREATE_BUFFER`. The elements of this buffer will be interpreted as 32-bit floating-point numbers - i.e. the `GL_FLOAT` type is used for vertex data.
  - `offset_i` is a non-negative integer specifying the byte offset into `vertex_buffer_i` at which vertex data begins
  - `stride_i` is a non-negative integer specifying the distance in bytes between successive pieces of vertex data in `vertex_buffer_i`
  - `dimension_i` is a positive integer specifying the dimensionality of vertices associated with vertex attribute `i`
- `index_buffer` specifies a buffer of index data, which must be produced by `CREATE_BUFFER`. It will be interpreted as a buffer of unsigned 32-bit integers - i.e. the `GL_UNSIGNED_INT` type is used for index data.
- `count` is a non-negative integer specifying how many vertices should be processed. Thus `index_buffer` should contain at least this many unsigned integers, and for each unsigned integer `index` in `index_buffer` the vertex buffers associated with each vertex attribute should contain suitable data.
- `topology` specifies the kind of primitive to be drawn using the vertex data. At present only `TRIANGLES` is supported. [More kinds of primitive should be supported](https://github.com/google/shadertrap/issues/25).
- For every `out` variable in the fragment shader associated with `graphics_program` there should be a corresponding entry in the `FRAMEBUFFER_ATTACHMENTS` parameter. If the fragment shader has a declaration of the form `out layout(location = l)` then `FRAMEBUFFER_ATTACHMENTS` should have an entry `location_i -> attachment_i` such that `location_i` = `l`, and `attachment_i` is a renderbuffer produced by `CREATE_RENDERBUFFER` or a texture produced by `CREATE_EMPTY_TEXTURE_2D`. This supports off-screen rendering to both renderbuffers and textures. On-screen rendering is not supported.

TODO(afd): The `RUN_GRAPHICS` command is complex, so it would be useful to have an illustrative example to accompany the description.

### SET_SAMPLER_PARAMETER

```
SET_SAMPLER_PARAMETER SAMPLER sampler PARAMETER parameter VALUE value
```

Sets a parameter of a sampler.

- `sampler` is the sampler for which the parameter should be set, and must be produced by `CREATE_SAMPLER`
- `parameter` is the name of the parameter to be set, and must be one of:
  - `TEXTURE_MAG_FILTER`
  - `TEXTURE_MIN_FILTER`
- `value` is the value to which the parameter should be set, and must be one of:
  - `NEAREST`
  - `NONE`
- [Other parameters and values should be supported in future as needed](https://github.com/google/shadertrap/issues/26)

### SET_TEXTURE_PARAMETER

```
SET_TEXTURE_PARAMETER TEXTURE texture PARAMETER parameter VALUE value
```

Sets a parameter of a sampler.

See the description of `SET_SAMPLER_PARAMETER`; this command does the same thing except that the parameter is set for the texture identified by `texture`, which must be produced by `CREATE_EMPTY_TEXTURE_2D`.

### SET_UNIFORM

This command has two forms, both of which set a uniform in a program's default uniform block to a value.

```
SET_UNIFORM PROGRAM program LOCATION location TYPE type VALUES value+
```

Sets a uniform according to its location.

- `program` is the program in which the uniform is to be set, and must be produced by `CREATE_PROGRAM`

- `location` is a non-negative integer corresponding to the location of the uniform to be set; if the uniform declaration in the shader(s) associated with `program` features the layout qualifier `layout(location = l)` then `l` should be provided as the value of `location`

- `type` is the type of the uniform, and should match the type declaration for the uniform in the shader(s) of `program`. This can be `sampler2D`, any scalar, vector or matrix type, or an array of any of these types (e.g., `vec2[5]`).

- `value+` should be a whitespace-separated sequence of values that collectively provide the uniform's value.
  - If `type` is `sampler2D` then `value+` should be a single non-negative integer corresponding to the texture unit the sampler uniform should sample from
  - If `type` is `float` then `value+` should be a single floating-point value
  - If `type` is `vecn` then `value+` should be a sequence of `n` floating-point values, one per component of the vector
  - If `type` is `matmxn` then `value+` should be a sequence of `m` x `n` floating-point values, one per element of the matrix, in column-major order
  - If `type` is `int` then `value+` should be a single integer value
  - If `type` is `ivecn` then `value+` should be a sequence of `n` integer values
  - If `type` is `uint` then `value+` should be a single non-negative integer value
  - If `type` is `uvecn` then `value+` should be a sequence of `n` non-negative integer values
  - If `type` is `element_type[n]` then `value+` should be the concatenation of `n` sequences of values, where each sequence is suitable for an element of type `element_type`

```
SET_UNIFORM PROGRAM program NAME name TYPE type VALUES value+
```

Sets a uniform according to its name. The parameters have the same meaning as in the previous version of `SET_UNIFORM`, except that instead of the `location`, `name` is a string specifying which uniform should be set. This must correspond to the name of a uniform declared in the shader(s) associated with `program`.

For example, suppose a shader contains the following declarations:

```
struct S {
  ivec2 a[3];
  int b;
};

struct T {
  S c[2];
  int d;
};

uniform T e[2];
```

Then we can use the following command to set `e[0].c[1].b` to the value 42 (assuming that `prog` is a compiled program):

```
SET_UNIFORM PROGRAM prog NAME "e[0].c[1].b" TYPE int VALUES 42
```

and the following command to set `e[1].c[1].a` to the array of vectors `[(1, 2), (3, 4), (5, 6)]`:

```
SET_UNIFORM PROGRAM prog NAME "e[1].c[1].a" TYPE ivec2[3] VALUES 1 2 3 4 5 6
```
