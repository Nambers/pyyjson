{
  pkgs ? import <nixpkgs> { },
}:
let
  nix_pyenv_directory = ".nix-pyenv";
  # define version
  use_minor_ver = import ./pyver.nix;
  drvs = pkgs.callPackage ./_drvs.nix { };
  using_pythons = drvs.using_pythons;
  using_python = builtins.elemAt using_pythons (use_minor_ver - 9);
  pyenvs = drvs.pyenvs;
  pyenv = builtins.elemAt pyenvs (use_minor_ver - 9);
in
pkgs.mkShell {
  packages = pkgs.callPackage ./packages.nix { };
}
