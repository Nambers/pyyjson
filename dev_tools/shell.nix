{
  pkgs ? import <nixpkgs> { },
}:
let
  nix_pyenv_directory = ".nix-pyenv";
  # define version
  pythonVerConfig = pkgs.lib.importJSON ./pyver.json;
  curVer = pythonVerConfig.curVer;
  leastVer = pythonVerConfig.minSupportVer;
  drvs = pkgs.callPackage ./_drvs.nix { };
  using_pythons = drvs.using_pythons;
  using_python = builtins.elemAt using_pythons (curVer - leastVer);
  pyenvs = drvs.pyenvs;
  pyenv = builtins.elemAt pyenvs (curVer - leastVer);
in
(pkgs.mkShell {
  packages = pkgs.callPackage ./packages.nix { };
  hardeningDisable = [ "fortify" ];

})
// {
  __drvs = drvs;
}
