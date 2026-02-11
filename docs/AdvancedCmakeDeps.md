Here is the updated Markdown document. I have added the "Onion Problem" section to explain why fixing the propagation is sometimes optional, alongside the technical details we covered.

You can copy-paste this directly into your notes.
Nix C/C++ Development: Dependencies, Wrappers, and LSP
1. Nix Dependency Management: Build vs. Propagated

In standard Linux (Ubuntu/Fedora), installing a library dumps everything into global paths (/usr/lib). In Nix, every package is isolated. This creates a distinction in how dependencies are declared.
The Concepts

    buildInputs (Private): Dependencies required to build this package, but not needed by consumers.

        Example: criterion needs nanomsg to compile itself.

    propagatedBuildInputs (Public): Dependencies that must be "passed down" to anyone using this package.

        Example: criterion headers include nanomsg headers. If I use criterion, I implicitly need nanomsg too.

The Problem: "Package Not Found"

If a Nix package maintainer puts a dependency in buildInputs instead of propagatedBuildInputs, pkg-config will fail.

    Symptom: pkg-config --cflags criterion works, but pkg-config --print-requires --private criterion fails with "Package 'nanomsg' not found".

    Cause: The .pc file says "I need nanomsg", but Nix didn't add nanomsg to the PKG_CONFIG_PATH.

Debugging with nix repl

We can inspect the upstream package definition to confirm if dependencies are missing from propagation.
Bash

$ nix repl
nix-repl> :lf .
nix-repl> pkgs = import inputs.nixpkgs { system = "aarch64-darwin"; }

# Check Public Inputs (Should contain nanomsg, libgit2, etc.)
nix-repl> pkgs.criterion.propagatedBuildInputs
[ ]  <-- EMPTY! This is the bug.

# Check Private Inputs (Found them here instead)
nix-repl> pkgs.criterion.buildInputs
[ «derivation ...nanomsg...» ]

The Fix: overrideAttrs

Instead of polluting our flake.nix with manual dependencies, we patch the package to fix the propagation logic.
Nix

# In flake.nix
criterionFixed = pkgs.criterion.overrideAttrs (old: {
  # Move private inputs to public so pkg-config can see them
  propagatedBuildInputs = (old.propagatedBuildInputs or []) ++ old.buildInputs;
});

2. The "Onion Problem": Why You Might Not Want to Fix It

While the overrideAttrs fix is "correct," applying it can lead to a recursive rabbit hole often called the Onion of Pain.
The Issue: Recursive Brokenness

Fixing one layer often reveals that the next layer is also broken.

    Layer 1: You fix criterion to propagate libgit2.

    Layer 2: pkg-config now reads libgit2.pc. It sees Requires.private: libpcre.

    Layer 3: libpcre is missing from libgit2's propagation. pkg-config complains again.

    Result: You are playing "Whac-a-Mole," manually fixing every library in the chain.

Why It’s Safe to Ignore (Warn != Fail)

You don't have to fix the entire chain if you are Dynamic Linking (standard .so / .dylib).

    Pkg-Config Behavior: It prints warnings to stderr ("Package libpcre not found"), but it successfully prints the CFLAGS to stdout and exits with code 0.

    CMake Behavior: CMake sees Exit Code 0 and proceeds. It ignores the warnings.

    The Build: The linker uses RPATH (hardcoded paths in the binary) to find the missing libraries at runtime anyway.

Conclusion: If pkg-config returns the includes you need (-I...) despite the warnings, you can choose to ignore the noise and skip the overrideAttrs fix.
3. The Wrappers: Nix Ideals vs. CMake Reality

Nix wraps standard tools (clang, gcc, pkg-config) to enforce reproducibility and isolation without requiring build scripts to know about Nix.
The Compiler Wrapper (clang-wrapper)

    What it does: It intercepts calls to clang. It looks at environment variables like NIX_CFLAGS_COMPILE and injects flags (-isystem /nix/store/...) automatically.

    The Benefit: Your CMakeLists.txt remains portable. It doesn't need to know where libraries are; the "compiler" just finds them.

    The Downside: It hides these flags from build systems (see LSP section below).

The Pkg-Config Wrapper

    The Issue: PKG_CONFIG_PATH is usually for the build machine. Nix needs to support cross-compilation (building on x86 for ARM).

    The Solution:

        User/Nix sets: PKG_CONFIG_PATH_FOR_TARGET (Contains the libraries we want to link).

        Wrapper does: sets PKG_CONFIG_PATH = PKG_CONFIG_PATH_FOR_TARGET and runs the real tool.

4. The LSP Issue (Clangd) & compile_commands.json

This is the friction point between "Nix Magic" and "Editor Tooling".
The Chain of Events

    CMake Check: CMake runs a test compile: clang main.c.

    Wrapper Action: The wrapper silently adds -I/nix/store/.../include. The compile succeeds.

    CMake Conclusion: "The compiler found the headers natively. Therefore, these are System Includes. I do not need to add explicit -I flags to the build command."

    JSON Generation: CMake writes compile_commands.json without the include paths.

    LSP Failure: clangd opens the file. It is not wrapped. It sees no -I flags. It cannot find <criterion.h>. Red Squiggles.

The Fix: target_compile_options

We must force CMake to treat these paths as User Options (Raw Strings), not System Includes (Smart Objects).

Don't use: target_include_directories

    Why: CMake optimizes this. It sees a system path and strips the flag to "clean up" the command.

Use: target_compile_options

    Why: This passes raw strings directly to the compiler command line. CMake assumes you know what you are doing and writes them verbatim into compile_commands.json.

CMake

# CMakeLists.txt
find_package(PkgConfig REQUIRED)
pkg_check_modules(CRITERION REQUIRED criterion)

# Force the raw flags into the JSON.
# This ensures clangd sees exactly what pkg-config returned.
target_compile_options(test PRIVATE ${CRITERION_CFLAGS})
target_link_libraries(test PRIVATE ${CRITERION_LIBRARIES})

5. Key Environment Variables

These are the invisible variables Nix manages for us.
Variable	Who sets it?	Purpose	Note
CMAKE_INCLUDE_PATH	Nix (via hook)	Tells CMake where to look for headers (find_path).	Does not add flags to the compiler automatically. Only expands the search scope.
PKG_CONFIG_PATH_FOR_TARGET	Nix (mkShell)	List of directories containing .pc files for the target architecture.	The wrapper reads this and sets PKG_CONFIG_PATH.
NIX_CFLAGS_COMPILE	Nix (mkShell)	List of -isystem flags for all dependencies.	The Clang wrapper injects these silently.
NIX_LDFLAGS	Nix (mkShell)	List of -L flags for all dependencies.	The Linker wrapper injects these silently.
