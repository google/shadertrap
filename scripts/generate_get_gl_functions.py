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


import argparse
import sys

import xml.etree.ElementTree as ElT

from pathlib import Path
from typing import Any

doc = """
Generates a function that produces a populated struct of functions from the gles2 API
"""


def gen_functions(xml_file: Path) -> str:
    tree = ElT.parse(xml_file)
    registry = tree.getroot()
    required_command_names = set()
    commands = None
    for child in registry:  # type: ElT.Element
        if child.tag == 'commands':
            assert not commands
            commands = child
        if child.tag == 'feature' and child.attrib['api'] == 'gles2':
            for api_child in child:
                if api_child.tag == 'require':
                    for requirement in api_child:
                        if requirement.tag == 'command':
                            required_command_names.add(requirement.attrib['name'])

    get_gl_functions = 'GlFunctions GetGlFunctions() {\n'
    get_gl_functions += '  GlFunctions result{};\n'
    get_gl_functions += '  // clang-format off\n'
    for command in commands:
        assert command.tag == 'command'
        proto = command[0]
        assert proto.tag == 'proto'
        name = None
        for child in proto:
            if child.tag == 'name':
                name = child
                break
        assert name is not None
        if name.text in required_command_names:
            get_gl_functions += '  result.' + name.text + '_ = ' + name.text + ';\n'

    get_gl_functions += '  // clang-format on\n'
    get_gl_functions += '  return result;\n'
    get_gl_functions += '}\n'

    prologue = """// Copyright 2021 The ShaderTrap Project Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Automatically-generated file - DO NOT EDIT

// clang-format off
// GLAD needs to be included here because it must be included before
// GLES3/gl32.h, which will be transitively included by get_gl_functions.h.
#include "glad/glad.h"
// clang-format on

#include "shadertrap/get_gl_functions.h"

#include <functional>

#include "libshadertrap/gl_functions.h"

namespace shadertrap {

"""

    epilogue = """
}  // namespace shadertrap
"""

    return prologue + get_gl_functions + epilogue


def main(args) -> None:
    raw_help_formatter: Any = argparse.RawDescriptionHelpFormatter
    parser = argparse.ArgumentParser(
        description=doc,
        formatter_class=raw_help_formatter,
    )
    parser.add_argument(
        "xml",
        help="Path to gl.xml",
        type=Path,
    )
    parsed_args = parser.parse_args(args[1:])
    print(gen_functions(parsed_args.xml), end='')


if __name__ == "__main__":
    main(sys.argv)
