#!/usr/bin/env python3

# Copyright 2021 The ShaderTrap Project Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import generate_get_gl_functions
import generate_gl_struct
import sys
from pathlib import Path


def go():
    gl_xml_file: Path = Path('third_party', 'OpenGL-Registry', 'OpenGL-Registry', 'xml', 'gl.xml')
    header_file_path: Path = Path('src', 'libshadertrap', 'include', 'libshadertrap', 'gl_functions.h')
    generated_struct_header: str = generate_gl_struct.gen_struct(gl_xml_file)
    existing_struct_header: str = open(header_file_path, 'r').read()
    if generated_struct_header != existing_struct_header:
        print('Header file ' + str(header_file_path) + ' needs to be re-generated')
        sys.exit(1)
    source_file_path: Path = Path('src', 'shadertrap', 'src', 'get_gl_functions.cc')
    generated_source: str = generate_get_gl_functions.gen_functions(gl_xml_file)
    existing_source: str = open(source_file_path, 'r').read()
    if generated_source != existing_source:
        print('Source file ' + str(source_file_path) + ' needs to be re-generated')
        sys.exit(1)
    

if __name__ == "__main__":
    go()
