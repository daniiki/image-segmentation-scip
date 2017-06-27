{ pkgs ? import <nixpkgs> {} }:

let
  stdenv = pkgs.stdenv;
  scipoptsuite = (import ./scipoptsuite.nix) { inherit (pkgs) stdenv fetchurl zlib gmp readline; };
in stdenv.mkDerivation {
  name = "Fortgeschrittenenpraktikum";
  buildInputs = with pkgs; [ scipoptsuite boost ];
}
