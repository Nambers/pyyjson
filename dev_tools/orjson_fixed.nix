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
  version = if useNixpkgsUnstable then "3.10.15" else "3.10.1";
  pyproject = true;

  disabled = super.pythonOlder "3.8";

  src = fetchFromGitHub {
    owner = "ijl";
    repo = "orjson";
    rev = version;
    hash =
      if useNixpkgsUnstable then
        "sha256-FlcWf6BhUP2Y5ivRQx1W0G8sgfvbuAQN7qpBJbd3N2I="
      else
        "sha256-vEJriLd7f+zlYcMIyhDTkq2kmNc5MaNLHo0qMLS5hro=";
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
            "sha256-yQkpjedHwgsZiiZEzYV66aa9RepCFW0PBqtD29tfoMI=";
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
