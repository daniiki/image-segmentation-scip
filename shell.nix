{ pkgs ? import <nixpkgs> {} }:

let
  gmp-static = pkgs.gmp.override { withStatic = true; };
  vlfeat = (import ./vlfeat.nix) { inherit (pkgs) stdenv fetchurl; };
in pkgs.stdenv.mkDerivation {
  name = "Fortgeschrittenenpraktikum";
  propagatedBuildInputs = with pkgs; [ zlib zlib.static gmp-static stdenv.cc.libc.static readline vlfeat boost libpng pngpp libpng doxygen gdb ];
}
