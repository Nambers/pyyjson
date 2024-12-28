{ stdenv, ... }:
stdenv.mkDerivation {
  name = "intel-sde";
  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    mkdir -p $out/share
    cp -r $src/* $out/share
    mv $out/share/sde64 $out/bin
    mv $out/share/sde $out/bin
    mv $out/share/xed $out/bin
    mv $out/share/xed64 $out/bin
    runHook postInstall
  '';
  postFixup = ''
    patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" $out/bin/sde
    patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" $out/bin/sde64
    patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" $out/bin/xed
    patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" $out/bin/xed64
  '';
  phases = [
    "unpackPhase"
    "installPhase"
    "fixupPhase"
  ];
  src = builtins.fetchTarball {
    url = "https://downloadmirror.intel.com/843185/sde-external-9.48.0-2024-11-25-lin.tar.xz";
    sha256 = "sha256:1z9nd3lfixwm0nyxim7x7vgfkmxxzj608lacqwm983cbw1x2dg04";
  };
}
