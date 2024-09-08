pypkgs:

let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
in
with pypkgs;
[
  pip
  pytest
  # needed by tests
  arrow
  psutil
  pytest-random-order
  pytest-xdist
  pytz
  # add packages here
  (
    (pypkgs.buildPythonPackage rec {
      pname = "orjson";
      version = "3.10.7";
      pyproject = true;

      disabled = pythonOlder "3.8";

      src = pkgs.fetchFromGitHub {
        owner = "ijl";
        repo = "orjson";
        rev = "refs/tags/${version}";
        hash = "sha256-+ofDblSbaG8CjRXFfF0QFpq2yGmLF/2yILqk2m8PSl8=";
      };

      cargoDeps = pkgs.rustPlatform.fetchCargoTarball {
        inherit src;
        name = "${pname}-${version}";
        hash = "sha256-MACmdptHmnifBTfB5s+CY6npAOFIrh0zvrIImYghGsw=";
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
        psutil
        pytestCheckHook
        python-dateutil
        pytz
        xxhash
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
] ++
(with pypkgs;
(lib.optional (minorVer < 13) objgraph)
)
