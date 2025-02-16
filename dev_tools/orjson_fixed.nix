{
  super,
  pkgs,
  lib,
  fetchFromGitHub,
  stdenv,
  rustPlatform,
  ...
}:
# super.orjson.overridePythonAttrs (
#   superAttr:
#   let
#     filterFunc = x: (!(builtins.isAttrs (x)) || ((x.pname or "") != "xxhash"));
#     _nativeBuildInputs = (builtins.filter filterFunc super.orjson.nativeBuildInputs);
#     # _nativeCheckInputs = (builtins.filter filterFunc (builtins.trace super.orjson super.orjson).nativeCheckInputs);
#     _buildInputs = (builtins.filter filterFunc super.orjson.buildInputs);
#   in
#   rec {
#     nativeBuildInputs =
#       assert (builtins.length _nativeBuildInputs) < (builtins.length super.orjson.nativeBuildInputs);
#       _nativeBuildInputs;
#       buildInputs =
#       assert (builtins.length _buildInputs) < (builtins.length super.orjson.buildInputs);
#       _buildInputs;
#     # nativeCheckInputs =
#     #   assert (builtins.length _nativeCheckInputs) < (builtins.length super.orjson.nativeCheckInputs);
#     #   _nativeCheckInputs;
#   }
# )
super.buildPythonPackage rec {
  pname = "orjson";
  version = super.orjson.version;
  pyproject = true;

  disabled = super.pythonOlder "3.8";

  src = fetchFromGitHub {
    owner = "ijl";
    repo = "orjson";
    tag = version;
    hash = "sha256-FlcWf6BhUP2Y5ivRQx1W0G8sgfvbuAQN7qpBJbd3N2I=";
  };

  cargoDeps = rustPlatform.fetchCargoVendor {
    inherit src;
    name = "${pname}-${version}";
    hash = "sha256-fHp5Rh2Mzn62ZUoVHETl/6kZ6Iztxkd5mjxira7NVBU=";
  };

  nativeBuildInputs =
    [ super.cffi ]
    ++ (with rustPlatform; [
      cargoSetupHook
      maturinBuildHook
    ]);

  buildInputs = lib.optionals stdenv.hostPlatform.isDarwin [ super.libiconv ];

  nativeCheckInputs = with super; [
    # numpy
    psutil
    pytestCheckHook
    python-dateutil
    pytz
    # xxhash
  ];

  pythonImportsCheck = [ "orjson" ];

  # passthru.tests = {
  #   inherit (super)
  #     falcon
  #     fastapi
  #     gradio
  #     mashumaro
  #     ufolib2
  #     ;
  # };

  # meta = with lib; {
  #   description = "Fast, correct Python JSON library supporting dataclasses, datetimes, and numpy";
  #   homepage = "https://github.com/ijl/orjson";
  #   changelog = "https://github.com/ijl/orjson/blob/${version}/CHANGELOG.md";
  #   license = with licenses; [
  #     asl20
  #     mit
  #   ];
  #   platforms = platforms.unix;
  #   maintainers = with maintainers; [ misuzu ];
  # };
}
