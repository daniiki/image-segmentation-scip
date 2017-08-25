{ pkgs ? import <nixpkgs> {} }:

let
  vlfeat = (import ./vlfeat.nix) { inherit (pkgs) stdenv fetchurl; };
  opencv-gtk2 = pkgs.opencv.override { enableGtk2 = true; };
in pkgs.stdenv.mkDerivation {
  name = "Fortgeschrittenenpraktikum";
  buildInputs = with pkgs; [
    zlib gmp readline vlfeat boost pngpp doxygen
    opencv-gtk2 gtk2 pkgconfig
  ];
}
