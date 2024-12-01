{
  pkgs ? import <nixpkgs> { },
}:
let
  lib = pkgs.lib;
  supportedVers = lib.importJSON ./supported_versions.json;
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
      builtins.map (
        supportedVer: builtins.getAttr ("python3" + (builtins.toString supportedVer)) pkgs
      ) supportedVers
    )
  )
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
