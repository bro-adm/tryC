{
  description = "Development environment with CMake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # propagted dpdenecies for pkg-config tool to work in cmake
        # criterionFixed = pkgs.criterion.overrideAttrs (oldAttrs: {
        #     propagatedBuildInputs = (oldAttrs.propagatedBuildInputs or []) ++ (with pkgs; [
        #       boxfort 
        #       libffi 
        #       libgit2 
        #       nanomsg
        #     ]);
        # });
        # criterionFixed = pkgs.criterion.overrideAttrs (old: {
        #     propagatedBuildInputs = (old.propagatedBuildInputs or []) ++ old.buildInputs;
        # });
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.cmake
            pkgs.pkg-config

            # 2. The Compiler (Pick the stable wrapper)
            pkgs.llvmPackages_20.clang

            # 3. Testsing - Criterion
            # criterionFixed
            pkgs.criterion

            # 4. The LSP (For autocomplete/diagnostics in your editor)
            pkgs.llvmPackages_20.clang-tools # This gives you 'clangd'

            # 5. Debugger
            pkgs.lldb

            pkgs.llvmPackages.llvm
          ];

          shellHook = ''
            echo "Development environment loaded"
            echo ""
            echo "CMake version:"
            cmake --version
            echo ""
            echo "Clang version:"
            clang --version
            echo ""
            echo "Clangd version:"
            clangd --version
            echo ""
            echo "LLDB version:"
            lldb --version
          '';
        };
      }
    );
}
