{
  description = "C++ Development Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";

      pkgs = nixpkgs.legacyPackages.${system};
      llvm = pkgs.llvmPackages_22;
    in
    {
      devShells.${system}.default =
        pkgs.mkShell.override
          {
            stdenv = llvm.stdenv;
          }
          {
            hardeningDisable = [ "all" ];

            nativeBuildInputs = with pkgs; [
              llvm.lldb
              llvm.clang-tools
              mold
              cmake
              ninja
              doxygen
            ];

            buildInputs = [
              llvm.libcxx
            ];
          };
    };
}
