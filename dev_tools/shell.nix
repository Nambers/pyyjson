{
  pkgs ? import <nixpkgs> { },
}:
let
  python314 =
    (pkgs.python313.override (oldAttr: {
      self = python314;
      noldconfigPatch = "${pkgs.path}/pkgs/development/interpreters/python/cpython/3.13/no-ldconfig.patch";
      sourceVersion = {
        major = "3";
        minor = "14";
        patch = "0";
        suffix = "a1";
      };
      pythonAttr = "python313";
      hash = "sha256-PkZLDLt1NeLbNCYv0ZoKOT0OYr4PQ7FRPtmDebBU6tQ=";
    })).overrideAttrs
      (oldAttrs: {
        src = pkgs.fetchFromGitHub {
          owner = "python";
          repo = "cpython";
          rev = "v3.14.0a1";
          hash = "sha256-6UnX96n5XTTrXgP0a7oRI6eunfdEJbtyN2e7m7bW2tI=";
        };
      });
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
                setuptools = super.setuptools.overridePythonAttrs (superAttr: rec {
                  version = "75.1.1";
                  src = pkgs.fetchFromGitHub {
                    owner = "pypa";
                    repo = "setuptools";
                    rev = "refs/tags/v${version}";
                    hash = "sha256-b8O/DrDWAbD6ht9M762fFN6kPtV8hAbn1gAN9SS7H5g=";
                  };
                  # patches = superAttr.patches ++ [
                  #   ./fix.patch
                  # ];
                });
              }
          );
        }
      );
    in
    x;
  using_pythons =
    (builtins.map using_pythons_map (
      with pkgs;
      [
        python39
        python310
        python311
        python312
        python313
      ]
    ))
    ++ [ python314 ];
  # import required python packages
  required_python_packages = import ./py_requirements.nix;
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  #
  nix_pyenv_directory = ".nix-pyenv";
  # define version
  use_minor_ver = 12;
  using_python = builtins.elemAt using_pythons (use_minor_ver - 9);
  pyenv = builtins.elemAt pyenvs (use_minor_ver - 9);
in
pkgs.mkShell {
  # this defines the order in PATH.
  # make sure pyenv selected by use_minor_ver is the first one
  packages =
    [ pyenv ]
    ++ (with pkgs; [
      cmake
      gdb
      valgrind
      clang
      gcc
      python-launcher # use python-launcher to use other versions
    ])
    ++ (if (use_minor_ver != 14) then pyenvs else [ (builtins.elemAt pyenvs (14 - 9)) ]);
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
