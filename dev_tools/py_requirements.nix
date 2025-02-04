pypkgs:
let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
in
with pypkgs;
[
  psutil
  pytz
]
++ (
  with pypkgs; # needed by tests
  (lib.optionals (minorVer < 14) [
    pytest
    arrow
    pytest-random-order
    pytest-xdist
  ])
)
++ (lib.optionals (minorVer < 14) [
  (
    (pypkgs.buildPythonPackage rec {
      pname = "orjson";
      version = "3.10.11";
      pyproject = true;

      disabled = pythonOlder "3.8";

      src = pkgs.fetchFromGitHub {
        owner = "ijl";
        repo = "orjson";
        rev = "refs/tags/${version}";
        hash = "sha256-RJcTyLf2pLb1kHd7+5K9dGMWja4KFdKIwdRAp6Ud+Ps=";
      };

      cargoDeps = pkgs.rustPlatform.fetchCargoTarball {
        inherit src;
        name = "${pname}-${version}";
        hash = "sha256-HlvsV3Bsxa4Ud1+RrEnDWKX82DRyfgBS7GvK9827/wE=";
      };

      maturinBuildFlags = [ "--interpreter ${python.executable}" ];

      nativeBuildInputs =
        [ cffi ]
        ++ (with pkgs.rustPlatform; [
          cargoSetupHook
          (pypkgs.callPackage (
            { pkgsHostTarget }:
            pkgs.makeSetupHook {
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
            } ./maturin-build-hook.sh
          ) { })
        ]);

      buildInputs = lib.optionals stdenv.isDarwin [ libiconv ];

      nativeCheckInputs = [
        psutil
        pytestCheckHook
        python-dateutil
        pytz
        # xxhash
      ];

      preBuild = ''
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
  )
])
++ (with pypkgs; (lib.optionals (minorVer < 13) [ objgraph ]))
