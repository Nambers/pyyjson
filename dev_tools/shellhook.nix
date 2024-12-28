{
  nix_pyenv_directory,
  pyenv,
  pyenvs,
  using_python,
  pkgs,
  inputDerivation,
  lib,
}:
let
  debugSourceDir = "debug_source";
  path_concate = x: builtins.toString "${x}";
  env_concate = builtins.map path_concate pyenvs;
  link_python_cmd = python_env: ''
    ensure_symlink ${nix_pyenv_directory}/bin/${python_env.executable} ${python_env.interpreter}
    # creating python library symlinks
    NIX_LIB_DIR=${nix_pyenv_directory}/lib/${python_env.libPrefix}
    mkdir -p $NIX_LIB_DIR
    # adding site packages
    for file in ${python_env}/${python_env.sitePackages}/*; do
        basefile=$(basename $file)
        if [ -d "$file" ]; then
            if [[ "$basefile" != *dist-info && "$basefile" != __pycache__ ]]; then
                ensure_symlink "$NIX_LIB_DIR/$basefile" $file
            fi
        else
            # the typing_extensions.py will make the vscode type checker not working!
            if [[ $basefile == *.so ]] || ([[ $basefile == *.py ]] && [[ $basefile != typing_extensions.py ]]); then
                ensure_symlink "$NIX_LIB_DIR/$basefile" $file
            fi
        fi
    done
    for file in $NIX_LIB_DIR/*; do
        if [[ -L "$file" ]] && [[ "$(dirname $(readlink "$file"))" != "${python_env}/${python_env.sitePackages}" ]]; then
            rm -f "$file"
        fi
    done
    # ensure the typing_extensions.py is not in the lib directory
    rm $NIX_LIB_DIR/typing_extensions.py > /dev/null 2>&1
    unset NIX_LIB_DIR

    mkdir -p ${debugSourceDir}
    if [[ ! -d ${debugSourceDir}/Python-${python_env.python.version} ]]; then
      tar xvf ${python_env.python.src} -C ${debugSourceDir} --exclude='Doc' --exclude='Grammar' --exclude='Lib' > /dev/null 2>&1
      chmod -R 755 ${debugSourceDir}/Python-${python_env.python.version}
    fi
  '';
  orjsonSource =
    lib.optionalString (using_python.sourceVersion.minor != "14")
      (builtins.elemAt (builtins.filter (x: x.pname == "orjson") (
        import ./py_requirements.nix using_python.pkgs
      )) 0).src;
in
''
  _SOURCE_ROOT=$(readlink -f ${builtins.toString ./.}/..)
  if [[ $_SOURCE_ROOT == /nix/store* ]]; then
      IN_FLAKE=true
      _SOURCE_ROOT=$(readlink -f .)
  fi
  cd $_SOURCE_ROOT

  if [ "$IN_FLAKE" = "true" ] && [ ! -f "flake.nix" ]; then
    _SHOULD_CREATE_PYENV=false
  else
    _SHOULD_CREATE_PYENV=true
  fi

  ensure_symlink() {
      local link_path="$1"
      local target_path="$2"
      if [[ -L "$link_path" ]] && [[ "$(readlink "$link_path")" = "$target_path" ]]; then
          return 0
      fi
      rm -f "$link_path" > /dev/null 2>&1
      ln -s "$target_path" "$link_path"
  }

  if [ "$_SHOULD_CREATE_PYENV" = "false" ]; then
    echo "Not creating pyenv because not in the root directory"
    return
  fi

  # ensure the nix-pyenv directory exists
  mkdir -p ${nix_pyenv_directory}
  mkdir -p ${nix_pyenv_directory}/lib
  mkdir -p ${nix_pyenv_directory}/bin
''
+ (pkgs.lib.strings.concatStrings (builtins.map link_python_cmd pyenvs))
+ ''
  # add python executable to the bin directory
  ensure_symlink "${nix_pyenv_directory}/bin/python" ${pyenv}/bin/python
  # export PATH=${using_python}/bin:${nix_pyenv_directory}/bin:$PATH
  export PATH=${nix_pyenv_directory}/bin:$PATH

  # prevent gc
  nix-store --add-root ${nix_pyenv_directory}/.nix-shell-inputs --realise ${inputDerivation}

  # custom
  ensure_symlink "${nix_pyenv_directory}/bin/valgrind" ${pkgs.valgrind}/bin/valgrind
  export CC=${pkgs.clang}/bin/clang
  export CXX=${pkgs.clang}/bin/clang++
  export Python3_ROOT_DIR=${using_python}
  ensure_symlink "${nix_pyenv_directory}/bin/clang" $CC
  ensure_symlink "${nix_pyenv_directory}/bin/clang++" $CXX
  ensure_symlink "${nix_pyenv_directory}/bin/cmake" ${pkgs.cmake}/bin/cmake
  ensure_symlink "${nix_pyenv_directory}/bin/clang-format" ${pkgs.clang-tools}/bin/clang-format
  # clang -print-file-name will give wrong asan path, use gcc version
  ensure_symlink "${nix_pyenv_directory}/lib/libasan.so" $(readlink -f $(gcc -print-file-name=libasan.so))

  # unzip orjson source
  mkdir -p ${debugSourceDir}
  if [[ ! -d ${debugSourceDir}/orjson && ${using_python.sourceVersion.minor} != "14" ]]; then
      # this is a directory, not a tarball
      cp -r ${orjsonSource} ${debugSourceDir}/orjson
      chmod -R 700 ${debugSourceDir}/orjson
  fi

  # save env for external use
  echo "PATH=$PATH" > ${nix_pyenv_directory}/.shell-env
  echo "CC=$CC" >> ${nix_pyenv_directory}/.shell-env
  echo "CXX=$CXX" >> ${nix_pyenv_directory}/.shell-env
''
