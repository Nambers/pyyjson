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
        }
      );
    };
}
