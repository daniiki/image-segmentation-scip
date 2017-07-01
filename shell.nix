{ pkgs ? import <nixpkgs> {} }:

let
  stdenv = pkgs.stdenv;
  scipoptsuite = (import ./scipoptsuite.nix) { inherit (pkgs) stdenv fetchurl zlib gmp readline; };
  vlfeat = (import ./vlfeat.nix) { inherit (pkgs) stdenv fetchurl; };
in stdenv.mkDerivation {
  name = "Fortgeschrittenenpraktikum";
  buildInputs = with pkgs; [ scipoptsuite vlfeat boost pngpp ];
}
