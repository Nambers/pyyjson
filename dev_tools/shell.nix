let
  pkgs = import <nixpkgs> { };
  # define version
  using_python = (pkgs.enableDebugging pkgs.python312).override {
    self = using_python;
  };
  # import required python packages
  required_python_packages = import ./py_requirements.nix;
  #
  nix_pyenv_directory = ".nix-pyenv";
  pyenv = using_python.withPackages required_python_packages;
in
pkgs.mkShell {
  packages = [
    pyenv #(pkgs.enableDebugging using_python)
    pkgs.cmake
    pkgs.gdb
    pkgs.valgrind
    # pkgs.gcc
    pkgs.clang
    pkgs.gcc
  ];
  shellHook = ''
    cd ${builtins.toString ./.}/..

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
                ensure_symlink ${nix_pyenv_directory}/lib/$basefile $file
            fi
        else
            # the typing_extensions.py will make the vscode type checker not working!
            if [[ $basefile == *.so ]] || ([[ $basefile == *.py ]] && [[ $basefile != typing_extensions.py ]]); then
                ensure_symlink ${nix_pyenv_directory}/lib/$basefile $file
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
    ensure_symlink ${nix_pyenv_directory}/bin/python ${pyenv}/bin/python
    # export PATH=${using_python}/bin:${nix_pyenv_directory}/bin:$PATH
    export PATH=${nix_pyenv_directory}/bin:$PATH

    nix-build shell.nix -A inputDerivation -o ${nix_pyenv_directory}/.nix-shell-inputs

    # custom
    ensure_symlink ${nix_pyenv_directory}/bin/valgrind ${pkgs.valgrind}/bin/valgrind
    export CC=${pkgs.clang}/bin/clang
    export CXX=${pkgs.clang}/bin/clang++
    ensure_symlink ${nix_pyenv_directory}/bin/clang $CC
    ensure_symlink ${nix_pyenv_directory}/bin/clang++ $CXX
    ensure_symlink ${nix_pyenv_directory}/bin/cmake ${pkgs.cmake}/bin/cmake
    # unzip the source
    mkdir -p build
    cd build
    if [[ ! -d ${using_python.src} ]]; then
        tar xvf ${using_python.src} > /dev/null
    fi
    if [[ ! -d orjson ]]; then
      # this is a directory, not a tarball
      cp -r ${pkgs.python312Packages.orjson.src} orjson
      echo "orjson source copied"
    fi
    cd ..
    # python lib
    # export PYTHONPATH=${pyenv}/${using_python.sitePackages}:$PYTHONPATH
    echo ${using_python.version}
  '';
}
