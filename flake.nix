{
  description = "pyyjson flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-24-05.url = "github:NixOS/nixpkgs/nixos-24.05";
  };

  outputs =
    inputs@{
      self,
      nixpkgs,
      nixpkgs-24-05,
      ...
    }:
    let
      forAllSystems =
        function:
        nixpkgs.lib.genAttrs
          [
            "x86_64-linux"
            "aarch64-linux"
            "x86_64-darwin"
            "aarch64-darwin"
          ]
          (
            system:
            function (
              import nixpkgs {
                inherit system;
              }
            )
          );

    in
    {
      devShells = forAllSystems (
        pkgs:
        let
          pkgs-24-05 = import nixpkgs-24-05 { inherit (pkgs) system; };
          defaultShell = pkgs.callPackage ./dev_tools/shell.nix {
            inherit pkgs-24-05;
            debugLLVM = false;
          };
          debugLLVMInternal = pkgs.callPackage ./dev_tools/shell.nix {
            inherit pkgs-24-05;
            debugLLVM = true;
          };
          _drvs = pkgs.callPackage ./dev_tools/_drvs.nix { inherit pkgs-24-05; };
          pythonVerConfig = pkgs.lib.importJSON ./dev_tools/pyver.json;
          curVer = pythonVerConfig.curVer;
          leastVer = pythonVerConfig.minSupportVer;
          verLength = curVer - leastVer;
          mkMyShell =
            { shell, ... }:
            (
              (shell.overrideAttrs {
                shellHook = pkgs.callPackage ./dev_tools/shellhook.nix {
                  parentShell = shell;
                  inherit (shell) inputDerivation;
                  inherit (_drvs) pyenvs;
                  nix_pyenv_directory = if shell.debugLLVM then ".nix-pyenv-llvm" else ".nix-pyenv";
                  pyenv = builtins.elemAt _drvs.pyenvs verLength;
                  using_python = builtins.elemAt _drvs.using_pythons verLength;
                };
              })
              // {
                super = shell;
              }
            );
        in
        {
          internal = defaultShell;
          default = mkMyShell { shell = defaultShell; };
          inherit debugLLVMInternal;
          debugLLVM = mkMyShell { shell = debugLLVMInternal; };
          packShell39 = pkgs.callPackage ./dev_tools/pack_shell.nix { py = pkgs-24-05.python39; };
          packShell310 = pkgs.callPackage ./dev_tools/pack_shell.nix { py = pkgs-24-05.python310; };
          packShell311 = pkgs.callPackage ./dev_tools/pack_shell.nix { py = pkgs-24-05.python311; };
          packShell312 = pkgs.callPackage ./dev_tools/pack_shell.nix { py = pkgs-24-05.python312; };
          packShell313 = pkgs.callPackage ./dev_tools/pack_shell.nix { py = pkgs.python313; };
          packShell314 = pkgs.callPackage ./dev_tools/pack_shell.nix { py = pkgs.python314; };
        }
      );
      packages = forAllSystems (
        pkgs:
        let
          pkgs-24-05 = import nixpkgs-24-05 { inherit (pkgs) system; };
          pythonVerConfig = pkgs.lib.importJSON ./dev_tools/pyver.json;
        in
        rec {
          pyyjson-py39 = pkgs.callPackage ./dev_tools/build_package.nix { python = pkgs-24-05.python39; };
          pyyjson-py310 = pkgs.callPackage ./dev_tools/build_package.nix { python = pkgs-24-05.python310; };
          pyyjson-py311 = pkgs.callPackage ./dev_tools/build_package.nix { python = pkgs-24-05.python311; };
          pyyjson-py312 = pkgs.callPackage ./dev_tools/build_package.nix { python = pkgs-24-05.python312; };
          pyyjson-py313 = pkgs.callPackage ./dev_tools/build_package.nix { python = pkgs.python313; };
          pyyjson-py314 = pkgs.callPackage ./dev_tools/build_package.nix { python = pkgs.python314; };
          default = pyyjson-py313;
        }
      );
    };
}
