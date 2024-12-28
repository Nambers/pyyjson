{
  description = "pyyjson flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } (
      { lib, ... }:
      {
        systems = [
          "aarch64-linux"
          "x86_64-linux"
          "riscv64-linux"
          "x86_64-darwin"
          "aarch64-darwin"
        ];
        perSystem =
          {
            config,
            pkgs,
            self',
            ...
          }:
          let
            defaultShell = pkgs.callPackage ./dev_tools/shell.nix { };
            _drvs = pkgs.callPackage ./dev_tools/_drvs.nix { };
            use_minor_ver = import ./dev_tools/pyver.nix;
            inputDerivation = defaultShell.inputDerivation;
          in
          {
            devShells.internal = defaultShell;
            devShells.default = defaultShell.overrideAttrs {
              shellHook = pkgs.callPackage ./dev_tools/shellhook.nix {
                inherit inputDerivation;
                inherit (_drvs) pyenvs;
                nix_pyenv_directory = ".nix-pyenv";
                pyenv = builtins.elemAt _drvs.pyenvs (use_minor_ver - 9);
                using_python = builtins.elemAt _drvs.using_pythons (use_minor_ver - 9);
              };
            };
          };
      }
    );
}
