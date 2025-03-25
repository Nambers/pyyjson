{
  super,
  pkgs,
  lib,
  fetchFromGitHub,
  stdenv,
  rustPlatform,
  ...
}:
let
  minorVer = lib.strings.toInt super.python.sourceVersion.minor;
  pythonVerConfig = lib.importJSON ./pyver.json;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
super.buildPythonPackage rec {
  pname = "orjson";
  version = super.orjson.version;
  pyproject = true;

  disabled = super.pythonOlder "3.8";

  src = fetchFromGitHub {
    owner = "ijl";
    repo = "orjson";
    rev = version;
    hash = "sha256-FlcWf6BhUP2Y5ivRQx1W0G8sgfvbuAQN7qpBJbd3N2I=";
  };

  cargoDeps =
    (if useNixpkgsUnstable then rustPlatform.fetchCargoVendor else pkgs.rustPlatform.fetchCargoTarball)
      {
        inherit src;
        name = "${pname}-${version}";
        hash =
          if useNixpkgsUnstable then
            "sha256-fHp5Rh2Mzn62ZUoVHETl/6kZ6Iztxkd5mjxira7NVBU="
          else
            "sha256-YvZl0zYuUBTIBAdIh6IDR3vIWlk5ye5e3cLB0j/41pk=";
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
