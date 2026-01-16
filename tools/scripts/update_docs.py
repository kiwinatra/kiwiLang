#!/usr/bin/env python3

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple, Any
import xml.etree.ElementTree as ET

class DocumentationUpdater:
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.docs_dir = project_root / "docs"
        self.include_dir = project_root / "include"
        self.src_dir = project_root / "src"
        
    def update_all(self, force: bool = False) -> bool:
        """Update all documentation"""
        success = True
        
        print("Updating kiwiLang documentation...")
        
        if not self.docs_dir.exists():
            self.docs_dir.mkdir(parents=True)
        
        success &= self.generate_api_documentation()
        success &= self.update_readme()
        success &= self.update_changelog()
        success &= self.generate_architecture_diagram()
        success &= self.build_doxygen_docs()
        success &= self.validate_documentation()
        
        return success
    
    def generate_api_documentation(self) -> bool:
        """Generate API documentation from source files"""
        print("  Generating API documentation...")
        
        api_dir = self.docs_dir / "api"
        api_dir.mkdir(exist_ok=True)
        
        stdlib_file = api_dir / "standard_lib.md"
        ffi_file = api_dir / "ffi.md"
        
        stdlib_content = self._generate_stdlib_docs()
        ffi_content = self._generate_ffi_docs()
        
        stdlib_file.write_text(stdlib_content, encoding='utf-8')
        ffi_file.write_text(ffi_content, encoding='utf-8')
        
        return True
    
    def _generate_stdlib_docs(self) -> str:
        """Generate standard library documentation"""
        content = [
            "# kiwiLang Standard Library",
            "",
            "This document describes the kiwiLang standard library.",
            "",
            "## Core Modules",
            "",
            "### `io`",
            "```kiwi",
            "module io",
            "",
            "fn print(value: any) -> void",
            "fn println(value: any) -> void",
            "fn readLine() -> string",
            "fn readChar() -> char",
            "fn open(path: string, mode: string) -> File",
            "fn close(file: File) -> void",
            "```",
            "",
            "### `math`",
            "```kiwi",
            "module math",
            "",
            "fn sin(x: float) -> float",
            "fn cos(x: float) -> float",
            "fn tan(x: float) -> float",
            "fn sqrt(x: float) -> float",
            "fn pow(base: float, exponent: float) -> float",
            "fn log(x: float) -> float",
            "fn log10(x: float) -> float",
            "```",
            "",
            "### `string`",
            "```kiwi",
            "module string",
            "",
            "fn length(str: string) -> int",
            "fn substring(str: string, start: int, end: int) -> string",
            "fn contains(str: string, substr: string) -> bool",
            "fn split(str: string, delimiter: string) -> []string",
            "fn join(strings: []string, delimiter: string) -> string",
            "fn trim(str: string) -> string",
            "fn toUpper(str: string) -> string",
            "fn toLower(str: string) -> string",
            "```",
            "",
            "### `collections`",
            "```kiwi",
            "module collections",
            "",
            "struct Array<T>",
            "struct List<T>",
            "struct Map<K, V>",
            "struct Set<T>",
            "struct Queue<T>",
            "struct Stack<T>",
            "```",
            "",
            "### `time`",
            "```kiwi",
            "module time",
            "",
            "fn now() -> DateTime",
            "fn sleep(milliseconds: int) -> void",
            "struct Duration",
            "struct DateTime",
            "```",
            "",
            "### `system`",
            "```kiwi",
            "module system",
            "",
            "fn exit(code: int) -> void",
            "fn getEnv(key: string) -> string?",
            "fn setEnv(key: string, value: string) -> void",
            "fn currentDir() -> string",
            "fn changeDir(path: string) -> void",
            "```",
        ]
        
        return '\n'.join(content)
    
    def _generate_ffi_docs(self) -> str:
        """Generate Foreign Function Interface documentation"""
        content = [
            "# Foreign Function Interface",
            "",
            "kiwiLang provides a Foreign Function Interface (FFI) for calling",
            "functions from other languages.",
            "",
            "## C Interoperability",
            "",
            "### Basic Usage",
            "```kiwi",
            "extern \"C\" {",
            "    fn printf(format: *char, ...) -> int;",
            "    fn malloc(size: size_t) -> *void;",
            "    fn free(ptr: *void) -> void;",
            "}",
            "",
            "fn main() {",
            "    let message = \"Hello from kiwiLang!\\n\";",
            "    printf(message);",
            "}",
            "```",
            "",
            "### Type Mappings",
            "",
            "| kiwiLang Type | C Type |",
            "|------------|--------|",
            "| `i8`       | `int8_t` |",
            "| `i16`      | `int16_t` |",
            "| `i32`      | `int32_t` |",
            "| `i64`      | `int64_t` |",
            "| `u8`       | `uint8_t` |",
            "| `u16`      | `uint16_t` |",
            "| `u32`      | `uint32_t` |",
            "| `u64`      | `uint64_t` |",
            "| `f32`      | `float`   |",
            "| `f64`      | `double`  |",
            "| `bool`     | `bool`    |",
            "| `*T`       | `T*`      |",
            "| `[]T`      | `T*`, `size_t` |",
            "",
            "### Calling Conventions",
            "",
            "```kiwi",
            "extern \"stdcall\" fn Win32Function() -> void;",
            "extern \"fastcall\" fn FastFunction() -> void;",
            "extern \"system\" fn SystemFunction() -> void;",
            "```",
            "",
            "## Memory Management",
            "",
            "When using FFI, you are responsible for memory management:",
            "",
            "```kiwi",
            "extern \"C\" {",
            "    fn create_buffer(size: size_t) -> *u8;",
            "    fn destroy_buffer(ptr: *u8) -> void;",
            "}",
            "",
            "fn use_buffer() {",
            "    let buffer = create_buffer(1024);",
            "    // use buffer...",
            "    destroy_buffer(buffer);",
            "}",
            "```",
            "",
            "## Error Handling",
            "",
            "FFI calls can return error codes:",
            "",
            "```kiwi",
            "extern \"C\" {",
            "    fn open_file(path: *char) -> i32;  // returns 0 on success",
            "}",
            "",
            "fn try_open() -> Result<i32, string> {",
            "    let result = open_file(\"file.txt\");",
            "    if result == 0 {",
            "        return Ok(result);",
            "    } else {",
            "        return Err(\"Failed to open file\");",
            "    }",
            "}",
            "```",
        ]
        
        return '\n'.join(content)
    
    def update_readme(self) -> bool:
        """Update README.md with current project information"""
        print("  Updating README.md...")
        
        readme_path = self.project_root / "README.md"
        
        version = self._get_project_version()
        build_status = self._get_build_status()
        
        content = [
            f"# kiwiLang - The Kiwi Programming Language",
            "",
            f"![Build Status]({build_status})",
            "![License](https://img.shields.io/badge/license-MIT-blue.svg)",
            f"![Version](https://img.shields.io/badge/version-{version}-green.svg)",
            "",
            "kiwiLang is a modern, statically-typed programming language designed for",
            "performance, safety, and developer productivity.",
            "",
            "## Features",
            "",
            "- **Statically Typed** with type inference",
            "- **Memory Safe** without garbage collection",
            "- **Zero-cost Abstractions**",
            "- **Modern Syntax** inspired by Rust and Swift",
            "- **C Interoperability**",
            "- **Cross-platform** (Windows, Linux, macOS)",
            "- **Package Manager** built-in",
            "- **Great Tooling** (LSP, formatter, debugger)",
            "",
            "## Quick Start",
            "",
            "### Installation",
            "",
            "```bash",
            "# Clone the repository",
            "git clone https://github.com/yourusername/kiwiLang.git",
            "cd kiwiLang",
            "",
            "# Build from source",
            "mkdir build && cd build",
            "cmake ..",
            "cmake --build .",
            "```",
            "",
            "### Hello World",
            "",
            "Create `hello.kiwi`:",
            "",
            "```kiwi",
            "fn main() {",
            '    println("Hello, World!");',
            "}",
            "```",
            "",
            "Compile and run:",
            "",
            "```bash",
            "kiwiLang compile hello.kiwi",
            "./hello",
            "```",
            "",
            "## Documentation",
            "",
            "- [Language Specification](docs/spec/)",
            "- [Standard Library](docs/api/standard_lib.md)",
            "- [Tutorials](docs/tutorials/)",
            "- [Internals](docs/internals/)",
            "",
            "## Building from Source",
            "",
            "### Prerequisites",
            "",
            "- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)",
            "- CMake 3.15+",
            "- LLVM 13+ (optional, for JIT)",
            "",
            "### Build Commands",
            "",
            "```bash",
            "mkdir build && cd build",
            "cmake -DCMAKE_BUILD_TYPE=Release ..",
            "cmake --build . --parallel",
            "ctest  # Run tests",
            "```",
            "",
            "## Contributing",
            "",
            "We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md)",
            "for details.",
            "",
            "## License",
            "",
            "kiwiLang is licensed under the MIT License. See [LICENSE](LICENSE) for details.",
            "",
            "## Community",
            "",
            "- [Discord](https://discord.gg/kiwiLang)",
            "- [Twitter](https://twitter.com/kiwiLanglang)",
            "- [GitHub Discussions](https://github.com/yourusername/kiwiLang/discussions)",
        ]
        
        readme_path.write_text('\n'.join(content), encoding='utf-8')
        return True
    
    def _get_project_version(self) -> str:
        """Get project version from CMakeLists.txt or version file"""
        cmake_path = self.project_root / "CMakeLists.txt"
        if cmake_path.exists():
            content = cmake_path.read_text(encoding='utf-8')
            match = re.search(r'project\(.*VERSION\s+([\d.]+)', content)
            if match:
                return match.group(1)
        return "0.1.0"
    
    def _get_build_status(self) -> str:
        """Get build status badge URL"""
        return "https://github.com/yourusername/kiwiLang/actions/workflows/ci.yml/badge.svg"
    
    def update_changelog(self) -> bool:
        """Update CHANGELOG.md with recent commits"""
        print("  Updating CHANGELOG.md...")
        
        changelog_path = self.project_root / "CHANGELOG.md"
        
        if not changelog_path.exists():
            content = [
                "# Changelog",
                "",
                "All notable changes to kiwiLang will be documented in this file.",
                "",
                "The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),",
                "and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).",
                "",
                "## [Unreleased]",
                "",
                "### Added",
                "- Initial project structure",
                "- Basic lexer and parser",
                "- CMake build system",
                "",
                "### Changed",
                "",
                "### Fixed",
                "",
                "### Removed",
                "",
            ]
            changelog_path.write_text('\n'.join(content), encoding='utf-8')
        
        return True
    
    def generate_architecture_diagram(self) -> bool:
        """Generate architecture diagram in PlantUML format"""
        print("  Generating architecture diagram...")
        
        images_dir = self.docs_dir / "images"
        images_dir.mkdir(exist_ok=True)
        
        plantuml_path = images_dir / "architecture.puml"
        diagram = self._generate_plantuml_diagram()
        
        plantuml_path.write_text(diagram, encoding='utf-8')
        
        try:
            self._render_plantuml(plantuml_path)
        except Exception as e:
            print(f"    Warning: Could not render PlantUML diagram: {e}")
        
        return True
    
    def _generate_plantuml_diagram(self) -> str:
        """Generate PlantUML diagram content"""
        return """@startuml
!pragma layout smetana

title kiwiLang Architecture

package "Frontend" {
  [Lexer] as LEXER
  [Parser] as PARSER
  [AST Builder] as AST
  [Semantic Analyzer] as SEMANTIC
}

package "Middle-end" {
  [IR Generator] as IR
  [Optimizer] as OPT
}

package "Backend" {
  [Code Generator] as CODEGEN
  [JIT Compiler] as JIT
  [Linker] as LINKER
}

package "Runtime" {
  [Virtual Machine] as VM
  [Garbage Collector] as GC
  [Standard Library] as STDLIB
}

LEXER -> PARSER : Tokens
PARSER -> AST : Parse Tree
AST -> SEMANTIC : Abstract Syntax Tree
SEMANTIC -> IR : Validated AST
IR -> OPT : Intermediate Representation
OPT -> CODEGEN : Optimized IR
CODEGEN -> LINKER : Object Files
CODEGEN -> JIT : JIT Code
LINKER -> VM : Executable
JIT -> VM : JIT Functions
VM -> GC : Memory Management
VM -> STDLIB : Library Calls

@enduml
"""
    
    def _render_plantuml(self, plantuml_path: Path) -> bool:
        """Render PlantUML diagram to PNG"""
        try:
            subprocess.run([
                "plantuml", "-tpng", str(plantuml_path)
            ], check=True, capture_output=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    
    def build_doxygen_docs(self) -> bool:
        """Build Doxygen documentation for C++ API"""
        print("  Building Doxygen documentation...")
        
        doxygen_dir = self.project_root / "docs" / "doxygen"
        doxygen_dir.mkdir(exist_ok=True)
        
        doxyfile = self._generate_doxyfile()
        doxyfile_path = doxygen_dir / "Doxyfile"
        
        doxyfile_path.write_text(doxyfile, encoding='utf-8')
        
        try:
            subprocess.run([
                "doxygen", str(doxyfile_path)
            ], check=True, capture_output=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("    Warning: Doxygen not found or failed")
            return False
    
    def _generate_doxyfile(self) -> str:
        """Generate Doxygen configuration"""
        return f"""PROJECT_NAME           = "kiwiLang"
PROJECT_NUMBER         = {self._get_project_version()}
PROJECT_BRIEF          = "The Kiwi Programming Language"
OUTPUT_DIRECTORY       = {self.docs_dir / "doxygen"}
CREATE_SUBDIRS         = NO
ALLOW_UNICODE_NAMES    = YES
OUTPUT_LANGUAGE        = English
BRIEF_MEMBER_DESC      = YES
REPEAT_BRIEF           = YES
ALWAYS_DETAILED_SEC    = NO
INLINE_INHERITED_MEMB  = NO
FULL_PATH_NAMES        = YES
STRIP_FROM_PATH        = {self.project_root}
SHORT_NAMES            = NO
JAVADOC_AUTOBRIEF      = YES
QT_AUTOBRIEF           = NO
MULTILINE_CPP_IS_BRIEF = NO
INHERIT_DOCS           = YES
SEPARATE_MEMBER_PAGES  = NO
TAB_SIZE               = 4
ALIASES                = 
OPTIMIZE_OUTPUT_FOR_C  = NO
OPTIMIZE_OUTPUT_JAVA   = NO
OPTIMIZE_FOR_FORTRAN   = NO
OPTIMIZE_OUTPUT_VHDL   = NO
MARKDOWN_SUPPORT       = YES
TOC_INCLUDE_HEADINGS   = 2
AUTOLINK_SUPPORT       = YES
BUILTIN_STL_SUPPORT    = YES
CPP_CLI_SUPPORT        = NO
SIP_SUPPORT            = NO
IDL_PROPERTY_SUPPORT   = YES
DISTRIBUTE_GROUP_DOC   = NO
SUBGROUPING            = YES
INLINE_GROUPED_CLASSES = NO
INLINE_SIMPLE_STRUCTS  = NO
TYPEDEF_HIDES_STRUCT   = NO
LOOKUP_CACHE_SIZE      = 0
NUM_PROC_THREADS       = 0
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_PACKAGE        = NO
EXTRACT_STATIC         = YES
EXTRACT_LOCAL_CLASSES  = YES
EXTRACT_LOCAL_METHODS  = NO
EXTRACT_ANON_NSPACES   = NO
HIDE_UNDOC_MEMBERS     = NO
HIDE_UNDOC_CLASSES     = NO
HIDE_FRIEND_COMPOUNDS  = NO
HIDE_IN_BODY_DOCS      = NO
INTERNAL_DOCS          = NO
CASE_SENSE_NAMES       = NO
HIDE_SCOPE_NAMES       = NO
HIDE_COMPOUND_REFERENCE= NO
SHOW_INCLUDE_FILES     = YES
SHOW_GROUPED_MEMB_INC  = NO
FORCE_LOCAL_INCLUDES   = NO
INLINE_INFO            = YES
SORT_MEMBER_DOCS       = YES
SORT_BRIEF_DOCS        = NO
SORT_MEMBERS_CTORS_1ST = NO
SORT_GROUP_NAMES       = NO
SORT_BY_SCOPE_NAME     = NO
STRICT_PROTO_MATCHING  = NO
GENERATE_TODOLIST      = YES
GENERATE_TESTLIST      = YES
GENERATE_BUGLIST       = YES
GENERATE_DEPRECATEDLIST= YES
ENABLED_SECTIONS       = 
MAX_INITIALIZER_LINES  = 30
SHOW_USED_FILES        = YES
SHOW_FILES             = YES
SHOW_NAMESPACES        = YES
FILE_VERSION_FILTER    = 
LAYOUT_FILE            = 
CITE_BIB_FILES         = 
QUIET                  = NO
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_NO_PARAMDOC       = NO
WARN_AS_ERROR          = NO
WARN_FORMAT            = "$file:$line: $text"
INPUT                  = {self.include_dir} {self.src_dir}
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.h *.hpp *.c *.cpp *.cc *.cxx *.ixx
RECURSIVE              = YES
EXCLUDE                = 
EXCLUDE_SYMLINKS       = NO
EXCLUDE_PATTERNS       = 
EXCLUDE_SYMBOLS        = 
EXAMPLE_PATH           = 
EXAMPLE_PATTERNS       = *
EXAMPLE_RECURSIVE      = NO
IMAGE_PATH             = 
INPUT_FILTER           = 
FILTER_PATTERNS        = 
FILTER_SOURCE_FILES    = NO
FILTER_SOURCE_PATTERNS = 
USE_MDFILE_AS_MAINPAGE = 
SOURCE_BROWSER         = YES
INLINE_SOURCES         = NO
STRIP_CODE_COMMENTS    = YES
REFERENCED_BY_RELATION = YES
REFERENCES_RELATION    = YES
REFERENCES_LINK_SOURCE = YES
SOURCE_TOOLTIPS        = YES
USE_HTAGS              = NO
VERBATIM_HEADERS       = YES
ALPHABETICAL_INDEX     = YES
COLS_IN_ALPHA_INDEX    = 5
IGNORE_PREFIX          = 
GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
HTML_HEADER            = 
HTML_FOOTER            = 
HTML_STYLESHEET        = 
HTML_EXTRA_STYLESHEET  = 
HTML_EXTRA_FILES       = 
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80
HTML_TIMESTAMP         = YES
HTML_DYNAMIC_MENUS     = YES
HTML_DYNAMIC_SECTIONS  = NO
HTML_INDEX_NUM_ENTRIES = 100
GENERATE_DOCSET        = NO
DOCSET_FEEDNAME        = "Doxygen generated docs"
DOCSET_BUNDLE_ID       = org.doxygen.Project
DOCSET_PUBLISHER_ID    = org.doxygen.Publisher
DOCSET_PUBLISHER_NAME  = Publisher
GENERATE_HTMLHELP      = NO
GENERATE_CHI           = NO
GENERATE_QHP           = NO
GENERATE_ECLIPSEHELP   = NO
ECLIPSE_DOC_ID         = org.doxygen.Project
DISABLE_INDEX          = NO
GENERATE_TREEVIEW      = NO
ENUM_VALUES_PER_LINE   = 4
TREEVIEW_WIDTH         = 250
EXT_LINKS_IN_WINDOW    = NO
FORMULA_FONTSIZE       = 10
FORMULA_TRANSPARENT    = YES
USE_MATHJAX            = NO
MATHJAX_FORMAT         = HTML-CSS
MATHJAX_RELPATH        = 
MATHJAX_EXTENSIONS     = 
MATHJAX_CODEFILE       = 
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO
EXTERNAL_SEARCH        = NO
SEARCHDATA_FILE        = searchdata.xml
GENERATE_LATEX         = NO
GENERATE_RTF           = NO
GENERATE_MAN           = NO
GENERATE_XML           = YES
XML_OUTPUT             = xml
XML_PROGRAMLISTING     = YES
XML_NS_MEMB_FILE_SCOPE = YES
GENERATE_DOCBOOK       = NO
DOCBOOK_OUTPUT         = docbook
GENERATE_AUTOGEN_DEF   = NO
GENERATE_PERLMOD       = NO
ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = NO
EXPAND_ONLY_PREDEF     = NO
SEARCH_INCLUDES        = YES
INCLUDE_PATH           = 
INCLUDE_FILE_PATTERNS  = 
PREDEFINED             = 
EXPAND_AS_DEFINED      = 
SKIP_FUNCTION_MACROS   = YES
TAGFILES               = 
GENERATE_TAGFILE       = 
ALLEXTERNALS           = NO
EXTERNAL_GROUPS        = YES
EXTERNAL_PAGES         = YES
PERL_PATH              = /usr/bin/perl
CLASS_DIAGRAMS         = YES
MSCGEN_PATH            = 
DIA_PATH               = 
HIDE_UNDOC_RELATIONS   = YES
HAVE_DOT               = YES
DOT_NUM_THREADS        = 0
DOT_FONTNAME           = Helvetica
DOT_FONTSIZE           = 10
DOT_FONTPATH           = 
CLASS_GRAPH            = YES
COLLABORATION_GRAPH    = YES
GROUP_GRAPHS           = YES
UML_LOOK               = NO
UML_LIMIT_NUM_FIELDS   = 10
TEMPLATE_RELATIONS     = NO
INCLUDE_GRAPH          = YES
INCLUDED_BY_GRAPH      = YES
CALL_GRAPH             = NO
CALLER_GRAPH           = NO
GRAPHICAL_HIERARCHY    = YES
DIRECTORY_GRAPH        = YES
DOT_IMAGE_FORMAT       = png
INTERACTIVE_SVG        = NO
DOT_PATH               = 
DOTFILE_DIRS           = 
MSCFILE_DIRS           = 
DIAFILE_DIRS           = 
PLANTUML_JAR_PATH      = 
PLANTUML_CFG_FILE      = 
PLANTUML_INCLUDE_PATH  = 
DOT_GRAPH_MAX_NODES    = 50
MAX_DOT_GRAPH_DEPTH    = 0
DOT_TRANSPARENT        = YES
DOT_MULTI_TARGETS      = NO
GENERATE_LEGEND        = YES
DOT_CLEANUP            = YES
"""
    
    def validate_documentation(self) -> bool:
        """Validate documentation for broken links and formatting"""
        print("  Validating documentation...")
        
        broken_links = []
        
        for doc_file in self.docs_dir.rglob("*.md"):
            content = doc_file.read_text(encoding='utf-8')
            
            link_pattern = r'\[([^\]]+)\]\(([^)]+)\)'
            matches = re.finditer(link_pattern, content)
            
            for match in matches:
                link_text = match.group(1)
                link_url = match.group(2)
                
                if link_url.startswith('http'):
                    continue
                
                if link_url.startswith('#'):
                    continue
                
                target_path = doc_file.parent / link_url
                if not target_path.exists():
                    broken_links.append((doc_file, link_url))
        
        if broken_links:
            print("    Found broken links:")
            for file, link in broken_links:
                print(f"      {file}: {link}")
            return False
        
        print("    Documentation validation passed")
        return True

def main():
    parser = argparse.ArgumentParser(
        description="Update kiwiLang documentation"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force update all documentation"
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Only check documentation, don't update"
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path.cwd(),
        help="Project root directory"
    )
    
    args = parser.parse_args()
    
    updater = DocumentationUpdater(args.project_root)
    
    if args.check_only:
        print("Checking documentation...")
        success = updater.validate_documentation()
        return 0 if success else 1
    else:
        success = updater.update_all(args.force)
        if success:
            print("Documentation update completed successfully")
            return 0
        else:
            print("Documentation update completed with warnings")
            return 1

if __name__ == "__main__":
    sys.exit(main())