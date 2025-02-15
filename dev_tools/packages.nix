{
  pkgs ? import <nixpkgs> { },
}:
let
  pythonVerConfig = import ./pyver.nix;
  curVer = pythonVerConfig.curVer;
  leastVer = pythonVerConfig.minSupportVer;
  drvs = (pkgs.callPackage ./_drvs.nix { });
  pyenv = builtins.elemAt drvs.pyenvs (curVer - leastVer);
in
# this defines the order in PATH.
# make sure pyenv selected by curVer is the first one
[ pyenv ]
++ (with drvs; [
  clang
  cmake
  gcc
  gdb
  python-launcher
  sde
  valgrind
])
++ drvs.pyenvs
