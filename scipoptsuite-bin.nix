with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "scipoptsuite-${version}";
  version = "4.0.0";

  srcs = [
    (fetchurl {
      url = "http://scip.zib.de/download/release/libscipopt-${version}.linux.x86_64.gnu.opt.so.zip";
      sha256 = "15292c42c1bdfb3f597db87b315794a880c9732e87adf9ce3474a82e045a0650";
    })
    (fetchurl {
      url = "http://scip.zib.de/download/release/scipoptheaders-${version}.tgz";
      sha256 = "a2f07946dcc6394191c74ec85e47c342b1a6fc6eb799d86925ec1a180716e1dd";
    })
  ];

  sourceRoot = ".";

  buildInputs = [ unzip ];

  dontBuild = true;

  installPhase = ''
    mkdir -p $out/lib
    mv *.so $out/lib/
    mv include $out/include
  '';
}
