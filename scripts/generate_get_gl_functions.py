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

import xml.etree.ElementTree as ET

from pathlib import Path

doc = """
Generates a function that produces a populated struct of functions from the gles2 API
"""


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

    parser.add_argument(
        "output_file",
        help="Path to the source file that should be generated",
        type=Path,
    )
    
    parsed_args = parser.parse_args(args[1:])

    tree = ET.parse(parsed_args.xml)
    registry = tree.getroot()
    required_command_names = set()
    commands = None
    for child in registry:  # type: ET.Element
        if child.tag == 'commands':
            assert not commands
            commands = child
        if child.tag == 'feature' and child.attrib['api'] == 'gles2':
            for api_child in child:
                if api_child.tag == 'require':
                    for requirement in api_child:
                        if requirement.tag == 'command':
                            required_command_names.add(requirement.attrib['name'])

    get_gl_functions = 'shadertrap::GlFunctions GetGlFunctions() {\n'
    get_gl_functions += '  shadertrap::GlFunctions result{};\n'
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
        assert name != None
        if name.text in required_command_names:
            get_gl_functions += '  result.' + name.text + '_ = ' + name.text + ';\n'

    get_gl_functions += '  // clang-format on\n'
    get_gl_functions += '  return result;\n'
    get_gl_functions += '}\n'

    PROLOGUE = """// Copyright 2021 The ShaderTrap Project Authors
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

#include <glad/glad.h>

#include "shadertrap/get_gl_functions.h"

namespace shadertrap {

"""

    EPILOGUE = """

}  // namespace shadertrap
"""
    
    with open(parsed_args.output_file, 'w') as outfile:
        outfile.write(PROLOGUE)
        outfile.write(get_gl_functions)
        outfile.write(EPILOGUE)

if __name__ == "__main__":
    main(sys.argv)
