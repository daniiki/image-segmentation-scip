{ stdenv, fetchurl, zlib, gmp, readline }:

stdenv.mkDerivation rec {
  name = "scipoptsuite-${version}";
  version = "4.0.0";

  src = fetchurl {
    url = "http://scip.zib.de/download/release/scipoptsuite-${version}.tgz";
    sha256 = "087535760eae3d633e2515d942a9b22e1f16332c022be8d093372bdc68e8a661";
  };

  buildInputs = [ zlib gmp readline ];

  makeFlags = [
    "SHARED=true"
    "ZIMPL=false"
  ];

  #preBuild = "set -x";
  #preBuild = ''
  #  makeFlagsArray=(SHELL="${pkgs.bash}/bin/bash -x" SHARED=true)
  #'';

  #doCheck = true;

  installPhase = ''
    make $makeFlags INSTALLDIR=$out install
    cp scip-${version}/src/objscip/*.h $out/include/objscip/
  '';

  # Hack to avoid TMPDIR in RPATHs.
  preFixup = ''rm -rf "$(pwd)" && mkdir "$(pwd)" '';
}
