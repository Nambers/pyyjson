{
  pkgs ? import <nixpkgs> { },
}:
let
  #   python314 =
  #     (pkgs.python313.override (oldAttr: {
  #       self = python314;
  #       noldconfigPatch = "${pkgs.path}/pkgs/development/interpreters/python/cpython/3.13/no-ldconfig.patch";
  #       sourceVersion = {
  #         major = "3";
  #         minor = "14";
  #         patch = "0";
  #         suffix = "a1";
  #       };
  #       pythonAttr = "python313";
  #       hash = "sha256-PkZLDLt1NeLbNCYv0ZoKOT0OYr4PQ7FRPtmDebBU6tQ=";
  #     })).overrideAttrs
  #       (oldAttrs: {
  #         src = pkgs.fetchFromGitHub {
  #           owner = "python";
  #           repo = "cpython";
  #           rev = "v3.14.0a1";
  #           hash = "sha256-6UnX96n5XTTrXgP0a7oRI6eunfdEJbtyN2e7m7bW2tI=";
  #         };
  #       });
  using_pythons_map =
    py:
    let
      x = (
        (pkgs.enableDebugging py).override {
          self = x;
          packageOverrides = (
            self: super:
            if !py.isPy39 then
              { }
            else
              {
                # for py39 there is an upstream bug
                # https://github.com/NixOS/nixpkgs/issues/353830
                setuptools = import ./py39setuptools.nix { inherit super pkgs; };
              }
          );
        }
      );
    in
    x;
  using_pythons = (
    builtins.map using_pythons_map (
      with pkgs;
      [
        python39
        python310
        python311
        python312
        python313
        python314
      ]
    )
  )
  # ++ [ python314 ]
  ;
  # import required python packages
  required_python_packages = import ./py_requirements.nix;
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
in
{
  inherit pyenvs; # list
  inherit using_pythons; # list
  inherit (pkgs)
    cmake
    gdb
    valgrind
    clang
    gcc
    python-launcher
    ; # packages
}
