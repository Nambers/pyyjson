{
  pkgs ? import <nixpkgs> { },
  pkgs-24-05,
  fetchFromGitHub,
  ...
}:
let
  lib = pkgs.lib;
  pythonVerConfig = lib.importJSON ./pyver.json;
  maxSupportVer = pythonVerConfig.maxSupportVer;
  minSupportVer = pythonVerConfig.minSupportVer;
  latestStableVer = pythonVerConfig.latestStableVer;
  supportedVers = builtins.genList (x: minSupportVer + x) (maxSupportVer - minSupportVer + 1);
  using_pythons_map =
    py:
    let
      x = (
        (pkgs.enableDebugging py).override {
          self = x;
          packageOverrides = (
            self: super:
            {
              orjson = pkgs.callPackage ./orjson_fixed.nix { inherit super; };
              pytest-benchmark = pkgs.callPackage ./pytest-benchmark-fixed.nix { inherit super; };
            }
            // (lib.optionalAttrs (py.pythonVersion == "3.14") {
              pytest-random-order =
                (super.pytest-random-order.override {
                  pytest-xdist = null;
                }).overrideAttrs
                  {
                    pytestCheckPhase = ":";
                  };
            })
            // (lib.optionalAttrs (py.pythonOlder "3.11") {
              # tomli =
              #   assert (lib.versionAtLeast super.tomli.version "2.0.3");
              #   (super.tomli.overrideAttrs {
              #     src = fetchFromGitHub {
              #       owner = "hukkin";
              #       repo = super.tomli.pname;
              #       rev = "2.0.2";
              #       hash = "sha256-YduGLNprrW1yFQ2gUNuueHTtQ+bXH43hVFzDR6rKtFI=";
              #     };
              #   });
            })
          );
        }
      );
    in
    x;
  using_pythons = (
    builtins.map using_pythons_map (
      builtins.map (
        supportedVer:
        builtins.getAttr ("python3" + (builtins.toString supportedVer)) (
          if (supportedVer >= latestStableVer) then pkgs else pkgs-24-05
        )
      ) supportedVers
    )
  );
  # import required python packages
  required_python_packages = import ./py_requirements.nix;
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  sde = pkgs.callPackage ./sde.nix { };
in
{
  inherit pyenvs; # list
  inherit using_pythons; # list
  inherit (pkgs)
    clang
    cmake
    gcc
    gdb
    python-launcher
    valgrind
    ; # packages
}
// lib.optionalAttrs (pkgs.system == "x86_64-linux") {
  inherit sde;
}
