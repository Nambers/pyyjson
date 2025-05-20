{
  clangStdenv,
  python,
  cmake,
  ...
}:
clangStdenv.mkDerivation rec {
  pname = "pyyjson";
  version = "0.0.1";
  src = ./.;
  unpackPhase = ''
    cp -r ${./..}/* .
    chmod -R 700 .
  '';
  postInstall = ''
    patchelf --remove-needed $(patchelf --print-needed $out/pyyjson.so | grep libpython3) $out/pyyjson.so
  '';
  nativeBuildInputs = [
    cmake
  ];
  buildInputs = [ python ];
  cmakeFlags = [
    "-DPREDEFINED_VERSION=${version}"
    "-DBUILD_TEST=OFF"
    "-DBUILD_SHIPPING_SIMD=ON"
  ];
}
