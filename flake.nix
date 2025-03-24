{
  description = "pyyjson flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-24-05.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
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
          defaultShell = pkgs.callPackage ./dev_tools/shell.nix { inherit pkgs-24-05; };
          _drvs = pkgs.callPackage ./dev_tools/_drvs.nix { inherit pkgs-24-05; };
          pythonVerConfig = pkgs.lib.importJSON ./dev_tools/pyver.json;
          curVer = pythonVerConfig.curVer;
          leastVer = pythonVerConfig.minSupportVer;
          verLength = curVer - leastVer;
        in
        # inputDerivation = defaultShell.inputDerivation;
        rec {
          internal = defaultShell;
          default = defaultShell.overrideAttrs {
            shellHook = pkgs.callPackage ./dev_tools/shellhook.nix {
              inherit (defaultShell) inputDerivation;
              inherit (_drvs) pyenvs;
              nix_pyenv_directory = ".nix-pyenv";
              pyenv = builtins.elemAt _drvs.pyenvs verLength;
              using_python = builtins.elemAt _drvs.using_pythons verLength;
            };
          };
        }
      );
      # devShells.internal = defaultShell;
      # devShells.default = defaultShell.overrideAttrs {
      #   shellHook = pkgs.callPackage ./dev_tools/shellhook.nix {
      #     inherit inputDerivation;
      #     inherit (_drvs) pyenvs;
      #     nix_pyenv_directory = ".nix-pyenv";
      #     pyenv = builtins.elemAt _drvs.pyenvs (curVer - leastVer);
      #     using_python = builtins.elemAt _drvs.using_pythons (curVer - leastVer);
      #   };
      # };
      # packages.gevent = pkgs.python39.withPackages (p: [
      #   (p.gevent.overrideAttrs {
      #     # src = pkgs.fetchFromGitHub {
      #     #     owner = "hukkin";
      #     #     repo = p.tomli.pname;
      #     #     rev = "2.0.2";
      #     #     hash = "";
      #     #   };
      #   })
      # ]);
    };
}
