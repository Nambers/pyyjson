{
  pkgs ? import <nixpkgs> { },
}:
let
  nix_pyenv_directory = ".nix-pyenv";
  # define version
  use_minor_ver = import ./pyver.nix;
  drvs = import ./_drvs.nix { inherit pkgs; };
  using_pythons = drvs.using_pythons;
  using_python = builtins.elemAt using_pythons (use_minor_ver - 9);
  pyenvs = drvs.pyenvs;
  pyenv = builtins.elemAt pyenvs (use_minor_ver - 9);
in
pkgs.mkShell {
  packages = import ./packages.nix { inherit pkgs; };
}
