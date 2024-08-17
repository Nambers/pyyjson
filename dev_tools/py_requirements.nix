pypkgs:

let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
in
with pypkgs;
[
  pip
  pytest
  # needed by tests
  arrow
  faker
  psutil
  pytest-random-order
  pytest-xdist
  pytz
  # add packages here
  (pypkgs.buildPythonPackage rec {
    pname = "orjson";
    version = "3.10.6";
    pyproject = true;

    disabled = pythonOlder "3.8";

    src = pkgs.fetchFromGitHub {
      owner = "ijl";
      repo = "orjson";
      rev = "refs/tags/${version}";
      hash = "sha256-K3wCzwaGOsaiCm2LW4Oc4XOnp6agrdTxCxqEIMq0fuU=";
    };

    cargoDeps = pkgs.rustPlatform.fetchCargoTarball {
      inherit src;
      name = "${pname}-${version}";
      hash = "sha256-SNdwqb47dJ084TMNsm2Btks1UCDerjSmSrQQUiGbx50=";
    };

    maturinBuildFlags = [ "--interpreter ${python.executable}" ];

    nativeBuildInputs =
      [ cffi ]
      ++ (with pkgs.rustPlatform; [
        cargoSetupHook
        (pypkgs.callPackage
          ({ pkgsHostTarget }:
            pkgs.makeSetupHook
              {
                name = "maturin-build-hook.sh";
                propagatedBuildInputs = [
                  pkgsHostTarget.maturin
                  pkgsHostTarget.cargo
                  pkgsHostTarget.rustc
                  pypkgs.wrapPython
                ];
                substitutions = {
                  inherit (pkgs.rust.envVars) rustTargetPlatformSpec setEnv;
                };
              } ./maturin-build-hook.sh)
          { })
      ]);

    buildInputs = lib.optionals stdenv.isDarwin [ libiconv ];

    nativeCheckInputs = [
      numpy
      psutil
      pytestCheckHook
      python-dateutil
      pytz
      xxhash
    ];

    preBuild =
      let ver = pypkgs.python.version; in
      ''
        cp -r . ../orjson
        cd ../orjson
      '';

    pythonImportsCheck = [ "orjson" ];

    passthru.tests = {
      inherit
        falcon
        fastapi
        gradio
        mashumaro
        ufolib2
        ;
    };
  })
] ++
(with pypkgs;
[
  (lib.mkIf (pypkgs.python.sourceVersion.minor < 12) objgraph)
]
)
