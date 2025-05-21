{
  clangStdenv,
  python,
  cmake,
  ...
}:
clangStdenv.mkDerivation rec {
  pname = "pyyjson";
  version = "0.0.0";
  src = ./.;
  unpackPhase = ''
    cp -r ${./..}/* .
    chmod -R 700 .
  '';
  # TODO aarch64?
  postInstall = ''
    patchelf --set-rpath /lib64 $out/pyyjson.so
    mv $out/pyyjson.so $out/pyyjson.cpython-3${python.sourceVersion.minor}-x86_64-linux-gnu.so
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
