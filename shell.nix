{ pkgs ? import <nixpkgs> {} }:

let
  vlfeat = (import ./vlfeat.nix) { inherit (pkgs) stdenv fetchurl; };
in pkgs.stdenv.mkDerivation {
  name = "Fortgeschrittenenpraktikum";
  buildInputs = with pkgs; [
    zlib gmp readline vlfeat boost pngpp doxygen
    mesa freeglut libdevil
  ];
}
