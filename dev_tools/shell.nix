{
  pkgs ? import <nixpkgs> { },
}:
let
  python314 =
    (pkgs.python313.override (oldAttr: {
      self = python314;
      packageOverrides = (
        self: super: rec {
          flit = super.flit.overridePythonAttrs (superAttr: rec {
            version = "3.10.1";
            src = pkgs.fetchFromGitHub {
              owner = "pypa";
              repo = "flit";
              rev = "refs/tags/${version}";
              hash = "sha256-GOup/iiR0zKM07dFiTFNzBEVBwzNp4ERWp1l4w9hOME=";
            };
          });
        }
      );
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
  use_minor_ver = 13;
  using_python = builtins.elemAt using_pythons (use_minor_ver - 9);
  pyenv = builtins.elemAt pyenvs (use_minor_ver - 9);
  path_concate = x: builtins.toString "${x}";
  env_concate = builtins.map path_concate pyenvs;
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
    ++ pyenvs;
  shellHook = ''
    _SOURCE_ROOT=$(readlink -f ${builtins.toString ./.}/..)
    if [[ $_SOURCE_ROOT == /nix/store* ]]; then
        IN_FLAKE=true
        _SOURCE_ROOT=$(readlink -f .)
    fi
    cd $_SOURCE_ROOT

    # ensure the nix-pyenv directory exists
    if [[ ! -d ${nix_pyenv_directory} ]]; then mkdir ${nix_pyenv_directory}; fi
    if [[ ! -d ${nix_pyenv_directory}/lib ]]; then mkdir ${nix_pyenv_directory}/lib; fi
    if [[ ! -d ${nix_pyenv_directory}/bin ]]; then mkdir ${nix_pyenv_directory}/bin; fi

    ensure_symlink() {
        local link_path="$1"
        local target_path="$2"
        if [[ -L "$link_path" ]] && [[ "$(readlink "$link_path")" = "$target_path" ]]; then
            return 0
        fi
        rm -f "$link_path" > /dev/null 2>&1
        ln -s "$target_path" "$link_path"
    }

    # creating python library symlinks
    for file in ${pyenv}/${using_python.sitePackages}/*; do
        basefile=$(basename $file)
        if [ -d "$file" ]; then
            if [[ "$basefile" != *dist-info && "$basefile" != __pycache__ ]]; then
                ensure_symlink "${nix_pyenv_directory}/lib/$basefile" $file
            fi
        else
            # the typing_extensions.py will make the vscode type checker not working!
            if [[ $basefile == *.so ]] || ([[ $basefile == *.py ]] && [[ $basefile != typing_extensions.py ]]); then
                ensure_symlink "${nix_pyenv_directory}/lib/$basefile" $file
            fi
        fi
    done
    for file in ${nix_pyenv_directory}/lib/*; do
        if [[ -L "$file" ]] && [[ "$(dirname $(readlink "$file"))" != "${pyenv}/${using_python.sitePackages}" ]]; then
            rm -f "$file"
        fi
    done

    # ensure the typing_extensions.py is not in the lib directory
    rm ${nix_pyenv_directory}/lib/typing_extensions.py > /dev/null 2>&1

    # add python executable to the bin directory
    ensure_symlink "${nix_pyenv_directory}/bin/python" ${pyenv}/bin/python
    # export PATH=${using_python}/bin:${nix_pyenv_directory}/bin:$PATH
    export PATH=${nix_pyenv_directory}/bin:$PATH

    # prevent gc
    if [[ -z "$IN_FLAKE" ]]; then
        if command -v nix-build > /dev/null 2>&1; then
            TEMP_NIX_BUILD_COMMAND=nix-build
        else
            TEMP_NIX_BUILD_COMMAND=/run/current-system/sw/bin/nix-build
        fi
        $TEMP_NIX_BUILD_COMMAND shell.nix -A inputDerivation -o ${nix_pyenv_directory}/.nix-shell-inputs
        unset TEMP_NIX_BUILD_COMMAND
    else
        if command -v nix > /dev/null 2>&1; then
            TEMP_NIX_COMMAND=nix
        else
            TEMP_NIX_COMMAND=/run/current-system/sw/bin/nix-build
        fi
        $TEMP_NIX_COMMAND build .#default.inputDerivation -o ${nix_pyenv_directory}/.nix-shell-inputs
        unset TEMP_NIX_COMMAND
    fi

    # custom
    ensure_symlink "${nix_pyenv_directory}/bin/valgrind" ${pkgs.valgrind}/bin/valgrind
    export CC=${pkgs.clang}/bin/clang
    export CXX=${pkgs.clang}/bin/clang++
    ensure_symlink "${nix_pyenv_directory}/bin/clang" $CC
    ensure_symlink "${nix_pyenv_directory}/bin/clang++" $CXX
    ensure_symlink "${nix_pyenv_directory}/bin/cmake" ${pkgs.cmake}/bin/cmake
    # unzip the source
    mkdir -p debug_source
    cd debug_source
    if [[ ! -d Python-${using_python.version} && ${using_python.sourceVersion.minor} != "14" ]]; then
        tar xvf ${using_python.src}
        chmod -R 700 Python-${using_python.version}
    fi
    if [[ ! -d orjson && ${using_python.sourceVersion.minor} != "14" ]]; then
        # this is a directory, not a tarball
        _ORJSON_SOURCE=${
          if using_python.sourceVersion.minor != "14" then
            (builtins.elemAt (builtins.filter (x: x.pname == "orjson") (
              required_python_packages using_python.pkgs
            )) 0).src
          else
            ""
        }
        cp -r $_ORJSON_SOURCE orjson
        chmod -R 700 orjson
        echo "orjson source copied: $_ORJSON_SOURCE"
    fi
    cd ..

    # echo ${builtins.toString env_concate}

    # save env for external use
    echo "PATH=$PATH" > ${nix_pyenv_directory}/.shell-env
    echo "CC=$CC" >> ${nix_pyenv_directory}/.shell-env
    echo "CXX=$CXX" >> ${nix_pyenv_directory}/.shell-env
  '';
}
