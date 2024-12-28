{
  pkgs ? import <nixpkgs> { },
}:
let
  use_minor_ver = import ./pyver.nix;
  drvs = (pkgs.callPackage ./_drvs.nix { });
  pyenv = builtins.elemAt drvs.pyenvs (use_minor_ver - 9);
in
# this defines the order in PATH.
# make sure pyenv selected by use_minor_ver is the first one
[ pyenv ]
++ (with drvs; [
  cmake
  gdb
  valgrind
  clang
  gcc
  python-launcher # use python-launcher to use other versions
])
++ drvs.pyenvs
