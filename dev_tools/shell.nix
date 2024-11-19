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
  # this defines the order in PATH.
  # make sure pyenv selected by use_minor_ver is the first one
  packages = import ./packages.nix { inherit pkgs; };
  shellHook = import ./shellhook.nix {
    inherit
      nix_pyenv_directory
      pyenv
      using_python
      pkgs
      pyenvs
      ;
  };
}
