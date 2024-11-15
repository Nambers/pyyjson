{
  pkgs ? import <nixpkgs> { },
  python ? pkgs.python3,
}:
python.pkgs.buildPythonApplication {
  pname = "pyyjson";
  version = "0.0.1";
  src = ./.;
  pyproject = false;
  nativeBuildInputs = [
    pkgs.cmake
    python.pkgs.setuptools
  ];
}
