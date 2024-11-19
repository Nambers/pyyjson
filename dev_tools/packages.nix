{
  pkgs ? import <nixpkgs> { },
}:
let
  use_minor_ver = import ./pyver.nix;
  drvs = (import ./_drvs.nix { inherit pkgs; });
  pyenv = builtins.elemAt drvs.pyenvs (use_minor_ver - 9);
in
[ pyenv ]
++ (with drvs; [
  cmake
  gdb
  valgrind
  clang
  gcc
  python-launcher # use python-launcher to use other versions
])
++ (if (use_minor_ver != 14) then drvs.pyenvs else [ (builtins.elemAt drvs.pyenvs (14 - 9)) ])
