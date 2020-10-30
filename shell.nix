{ nixroot ? (import <nixpkgs> { }), defaultLv2Plugins ? false, lv2Plugins ? [ ]
, releaseMode ? false, enableKeyfinder ? true, buildType ? "auto", cFlags ? [ ]
}:
let
  inherit (nixroot)
    stdenv pkgs lib makeWrapper clang-tools cmake fetchurl fetchgit glibcLocales
    nix-gitignore python3 python37Packages;

  cmakeBuildType = if buildType == "auto" then
    (if releaseMode then "RelWithDebInfo" else "Debug")
  else
    buildType;

  allCFlags = cFlags ++ [
    ("-DKEYFINDER=" + (if enableKeyfinder then "ON" else "OFF"))
    ("-DCMAKE_BUILD_TYPE=" + cmakeBuildType)
  ];

  git-clang-format = stdenv.mkDerivation {
    name = "git-clang-format";
    version = "2019-06-21";
    src = fetchurl {
      url =
        "https://raw.githubusercontent.com/llvm-mirror/clang/2bb8e0fe002e8ffaa9ce5fa58034453c94c7e208/tools/clang-format/git-clang-format";
      sha256 = "1kby36i80js6rwi11v3ny4bqsi6i44b9yzs23pdcn9wswffx1nlf";
      executable = true;
    };
    nativeBuildInputs = [ makeWrapper ];
    buildInputs = [ clang-tools python3 ];
    unpackPhase = ":";
    installPhase = ''
      mkdir -p $out/opt $out/bin
      cp $src $out/opt/git-clang-format
      makeWrapper $out/opt/git-clang-format $out/bin/git-clang-format \
        --add-flags --binary \
        --add-flags ${clang-tools}/bin/clang-format
    '';
  };

  libkeyfinder = if builtins.hasAttr "libkeyfinder" pkgs then
    pkgs.libkeyfinder
  else
    stdenv.mkDerivation {
      name = "libkeyfinder";
      version = "2.2.1-dev";

      src = fetchgit {
        url = "https://github.com/ibsh/libKeyFinder.git";
        rev = "9b4440d66789b06483fe5273e06368a381d22707";
        sha256 = "008xg6v4mpr8hqqkn8r5y5vnigggnqjjcrhv5r6q41xg6cfz0k72";
      };

      nativeBuildInputs = [ cmake ];
      buildInputs = with pkgs; [ fftw pkgconfig ];

      buildPhase = ''
        make VERBOSE=1 keyfinder
      '';
      installPhase = ''
        make install/fast
      '';
    };

  shell-configure = nixroot.writeShellScriptBin "configure" ''
    mkdir -p cbuild
    cd cbuild
    cmake .. ${lib.escapeShellArgs allCFlags} "$@"
    cd ..
    if [ ! -e venv/bin/pre-commit ]; then
      virtualenv -p python3 venv
      ./venv/bin/pip install pre-commit
      ./venv/bin/pre-commit install
    fi
  '';

  wrapper = (if builtins.hasAttr "wrapQtAppsHook" pkgs then
    "source ${pkgs.wrapQtAppsHook}/nix-support/setup-hook"
  else
    "source ${pkgs.makeWrapper}/nix-support/setup-hook");

  wrapperCmd = (if builtins.hasAttr "wrapQtAppsHook" pkgs then
    "wrapQtApp"
  else
    "wrapProgram");

  shell-build = nixroot.writeShellScriptBin "build" ''
    set -e
    if [ ! -d "cbuild" ]; then
      >&2 echo "First you have to run configure."
      exit 1
    fi
    cd cbuild
    rm -f .mixxx-wrapped mixxx
    cmake --build . --parallel $NIX_BUILD_CORES "$@"
    ${wrapper}
    ${wrapperCmd} mixxx --prefix LV2_PATH : ${
      lib.makeSearchPath "lib/lv2" allLv2Plugins
    }
    ${wrapperCmd} mixxx-test
  '';

  shell-run = nixroot.writeShellScriptBin "run" ''
    if [ ! -f "cbuild/mixxx" ]; then
      >&2 echo "First you have to run build."
      exit 1
    fi
    cd cbuild
    exec ./mixxx --resourcePath res/ "$@"
  '';

  shell-debug = nixroot.writeShellScriptBin "debug" ''
    if [ ! -f "cbuild/mixxx" ]; then
      >&2 echo "First you have to run build."
      exit 1
    fi
    cd cbuild
    exec env LV2_PATH=${
      lib.makeSearchPath "lib/lv2" allLv2Plugins
    } gdb --args ./.mixxx-wrapped --resourcePath res/ "$@"
  '';

  shell-run-tests = nixroot.writeShellScriptBin "run-tests" ''
    if [ ! -f "cbuild/mixxx-test" ]; then
      >&2 echo "First you have to run build."
      exit 1
    fi
    cd cbuild
    exec env LV2_PATH=${lib.makeSearchPath "lib/lv2" allLv2Plugins} ./mixxx-test
  '';

  allLv2Plugins = lv2Plugins ++ (if defaultLv2Plugins then
    (with pkgs; [
      artyFX
      infamousPlugins
      libsepol
      mod-distortion
      mount
      pcre
      rkrlv2
      utillinux
      x42-plugins
      zam-plugins
    ])
  else
    [ ]);

in stdenv.mkDerivation rec {
  name = "mixxx-${version}";
  # Reading the version from git output is very hard to do without wasting lots of diskspace and
  # runtime. Reading version file is easy.
  version = lib.strings.removeSuffix ''
    "
  '' (lib.strings.removePrefix ''#define MIXXX_VERSION "''
    (builtins.readFile ./src/_version.h));

  # SOURCE_DATE_EPOCH helps with python and pre-commit hook
  shellHook = ''
    if [ -z "$LOCALE_ARCHIVE" ]; then
      export LOCALE_ARCHIVE="${glibcLocales}/lib/locale/locale-archive";
    fi
    export PYTHONPATH=venv/lib/python3.7/site-packages/:$PYTHONPATH
    export SOURCE_DATE_EPOCH=315532800
    echo -e "Mixxx development shell. Available commands:\n"
    echo " configure - configures cmake (only has to run once)"
    echo " build - compiles Mixxx"
    echo " run - runs Mixxx with development settings"
    echo " debug - runs Mixxx inside gdb"
    echo " run-tests - runs Mixxx tests"
      '';

  src = if releaseMode then
    (nix-gitignore.gitignoreSource ''
      /cbuild
      /.envrc
      /result*
      /shell.nix
      /venv
    '' ./.)
  else
    null;

  nativeBuildInputs = [ pkgs.cmake ] ++ (if !releaseMode then
    (with pkgs; [
      ccache
      clang-tools
      gdb
      git-clang-format
      glibcLocales
      # for pre-commit installation since nixpkg.pre-commit may be to old
      python3
      python37Packages.pip
      python37Packages.setuptools
      python37Packages.virtualenv
      shell-build
      shell-configure
      shell-debug
      shell-run
      shell-run-tests
    ])
  else
    [ ]);

  cmakeFlags = allCFlags;

  buildInputs = (with pkgs; [
    chromaprint
    ffmpeg
    fftw
    flac
    git
    glib
    hidapi
    lame
    libebur128
    libGLU
    libid3tag
    libmad
    libmodplug
    libopus
    libshout
    libsecret
    libselinux
    libsepol
    libsForQt5.qtkeychain
    libsndfile
    libusb1
    libvorbis
    lilv
    lv2
    makeWrapper
    mp4v2
    opusfile
    pcre
    pkgconfig
    portaudio
    portmidi
    protobuf
    qt5.qtbase
    qt5.qtdoc
    qt5.qtscript
    qt5.qtsvg
    qt5.qtx11extras
    rubberband
    serd
    sord
    soundtouch
    sqlite
    taglib
    upower
    utillinux
    vamp.vampSDK
    wavpack
    x11
  ]) ++ allLv2Plugins ++ (if enableKeyfinder then [ libkeyfinder ] else [ ])
    ++ (if builtins.hasAttr "wrapQtAppsHook" pkgs then
      [ pkgs.wrapQtAppsHook ]
    else
      [ ]);

  postInstall = (if releaseMode then ''
    ${wrapperCmd} $out/bin/mixxx --prefix LV2_PATH : ${
      lib.makeSearchPath "lib/lv2" allLv2Plugins
    }
  '' else
    "");

  meta = with nixroot.stdenv.lib; {
    homepage = "https://mixxx.org";
    description = "Digital DJ mixing software";
    license = licenses.gpl2Plus;
    maintainers = nixroot.pkgs.mixxx.meta.maintainers;
    platforms = platforms.linux;
  };
}
