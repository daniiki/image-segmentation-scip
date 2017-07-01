{ stdenv, fetchurl }:

stdenv.mkDerivation rec {
  name = "vlfeat-${version}";
  version = "0.9.20";

  src = fetchurl {
    url = "http://www.vlfeat.org/download/vlfeat-${version}.tar.gz";
    sha256 = "0sih7gbnsccmn4hg3lpv2np7b2k7q6rpy857rq51ipd7i6b1k6ck";
  };

  # inspired by https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=vlfeat
  installPhase = ''
    mkdir -p $out/{bin,man/{man1,man7},lib,include/vl}
    builddir=bin/*
    echo $builddir
    mv $builddir/sift $out/bin
    mv src/sift.1 $out/man/man1
    mv $builddir/mser $out/bin
    mv src/mser.1 $out/man/man1
    mv $builddir/libvl.so $out/lib
    mv src/vlfeat.7 $out/man/man7
    mv vl/*h $out/include/vl
  '';
}
