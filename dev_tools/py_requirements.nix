pypkgs:
let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  pythonVerConfig = lib.importJSON ./pyver.json;
in
with pypkgs;
[
  psutil
  pytz
  objgraph
  pytest
  pytest-random-order
]
++ (
  with pypkgs; # needed by tests, but cannot be built in python3.14
  (lib.optionals (minorVer < 14) [
    arrow
    pytest-xdist
  ])
)
# some dependecies of orjson cannot be built in python3.14
++ (lib.optionals (minorVer < 14) [
  (
    (pypkgs.buildPythonPackage rec {
      pname = "orjson";
      version = "3.10.11";
      pyproject = true;
      useFetchCargoVendor = true;

      disabled = pythonOlder "3.8";

      src = pkgs.fetchFromGitHub {
        owner = "ijl";
        repo = "orjson";
        rev = "refs/tags/${version}";
        hash = "sha256-RJcTyLf2pLb1kHd7+5K9dGMWja4KFdKIwdRAp6Ud+Ps=";
      };

      cargoDeps = pkgs.rustPlatform.fetchCargoVendor {
        inherit src;
        name = "${pname}-${version}";
        hash = "sha256-sxUp3q9S1PmwUmbmXWj235MB+qAFzTCu8x1OXOzVUpY=";
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
# benchmark is only needed for python3.13
++ (with pypkgs; (lib.optionals (minorVer == pythonVerConfig.latestStableVer) [ pytest-benchmark ]))
