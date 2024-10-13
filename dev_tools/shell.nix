{ pkgs ? import <nixpkgs> { } }:
let
  using_pythons_map = py:
    let
      x = ((pkgs.enableDebugging py).override {
        self = x;
      });
    in
    x;
  using_pythons = builtins.map using_pythons_map (with pkgs;[
    python39
    python310
    python311
    python312
    python313
  ]);
  # import required python packages
  required_python_packages = import ./py_requirements.nix;
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  #
  nix_pyenv_directory = ".nix-pyenv";
  # define version
  use_minor_ver = 9;
  using_python = builtins.elemAt using_pythons (use_minor_ver - 9);
  pyenv = builtins.elemAt pyenvs (use_minor_ver - 9);
  path_concate = x: builtins.toString "${x}";
  env_concate = builtins.map path_concate pyenvs;
in
pkgs.mkShell {
  # this defines the order in PATH.
  # make sure pyenv selected by use_minor_ver is the first one
  packages = [ pyenv ] ++ (with pkgs;[
    cmake
    gdb
    valgrind
    clang
    gcc
    python-launcher # use python-launcher to use other versions
  ]) ++ pyenvs;
  shellHook = ''
    _SOURCE_ROOT=$(readlink -f ${builtins.toString ./.}/..)
    if [[ $_SOURCE_ROOT == /nix/store* ]]; then
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
    if command -v nix-build > /dev/null 2>&1; then
        TEMP_NIX_BUILD_COMMAND=nix-build
    else
        TEMP_NIX_BUILD_COMMAND=/run/current-system/sw/bin/nix-build
    fi
    $TEMP_NIX_BUILD_COMMAND shell.nix -A inputDerivation -o ${nix_pyenv_directory}/.nix-shell-inputs
    unset TEMP_NIX_BUILD_COMMAND

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
    if [[ ! -d Python-${using_python.version} ]]; then
        tar xvf ${using_python.src}
        chmod -R 700 Python-${using_python.version}
    fi
    if [[ ! -d orjson ]]; then
        # this is a directory, not a tarball
        _ORJSON_SOURCE=${(builtins.elemAt(builtins.filter (x: x.pname == "orjson") (required_python_packages using_python.pkgs)) 0).src}
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
