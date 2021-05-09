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
import os
import re
import sys

import xml.etree.ElementTree as ET

from pathlib import Path
from typing import List, Any, Optional

doc = """
Generates a struct of std::functions for all OpenGL ES functions
"""


def tidy_type(type_text: str) -> str:
    return re.sub(' \*', '*', type_text).strip()


def main(args) -> None:

    raw_help_formatter: Any = argparse.RawDescriptionHelpFormatter

    parser = argparse.ArgumentParser(
        description=doc,
        formatter_class=raw_help_formatter,
    )

    parser.add_argument(
        "--check_only",
        help="Don't change any files; just check that they would not change. If the files would have changed then "
             "exits with error status 2.",
        action="store_true",
    )

    parsed_args = parser.parse_args(args[1:])

    check_only: bool = parsed_args.check_only

    os.chdir(os.environ["SHADERTRAP_REPO_ROOT"])

    xml_path = Path() / "third_party" / "OpenGL-Registry" / "OpenGL-Registry" / "xml" / "gl.xml"

    tree = ET.parse(xml_path)
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

    structure = 'struct GlFunctions {\n'
    initialization = 'shadertrap::GlFunctions functions{};\n'
                            
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
            return_type = ''
            if proto.text is not None:
               return_type = proto.text
            return_ptype = proto.find('ptype') 
            if return_ptype is not None:
                return_type += return_ptype.text
                if return_ptype.tail is not None:
                    return_type += return_ptype.tail
            param_types = []
            for param in command.findall('param'):
                param_type = ''
                if param.text is not None:
                    param_type += param.text
                param_ptype = param.find('ptype')
                if param_ptype is not None:
                    param_type += param_ptype.text
                    if param_ptype.tail is not None:
                        param_type += param_ptype.tail
                param_types.append(tidy_type(param_type))
                
            structure += '  std::function<' + tidy_type(return_type) + '(' + ', '.join(param_types) + ')> ' + name.text + '_;\n'
            initialization += '  functions.' + name.text + '_ = ' + name.text + ';\n'

    structure += '};\n'
    print(structure)
    print()
    print(initialization)

if __name__ == "__main__":
    main(sys.argv)
