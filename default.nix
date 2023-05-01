{ 
    system ? builtins.currentSystem,
    pkgs ? import <nixpkgs> {inherit system;}, ... 
}:


pkgs.libsForQt5.callPackage(
    { mkDerivation, stdenv, darwin, cmake, bison, perl, ruby, qt5, boost, readline, SDL2, SDL, lib, wayland, waylandpp, pkgconfig }:

    mkDerivation {
        name = "executor2000";
        nativeBuildInputs = [cmake bison perl ruby pkgconfig];
        buildInputs = [
            qt5.qtbase
            boost
            readline
            SDL2
        ] ++ lib.optionals stdenv.isLinux (
            SDL
            wayland
            waylandpp
        ) ++ lib.optionals stdenv.isDarwin (with darwin.apple_sdk.frameworks; [
            Carbon
            Cocoa
        ]);
        src = pkgs.nix-gitignore.gitignoreSource [] ./.;
        hardeningDisable = [ "all" ];
        cmakeFlags = ["-DRUN_FIXUP_BUNDLE=NO" "-DNO_STATIC_BOOST=YES"];
    }
) {}