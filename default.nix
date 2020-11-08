{ 
    system ? builtins.currentSystem,
    pkgs ? import <nixpkgs> {inherit system;}, ... 
}:


pkgs.libsForQt5.callPackage(
    { mkDerivation, cmake, bison, perl, ruby, qt5, boost, readline, SDL2, SDL, lib, wayland, waylandpp, pkgconfig }:

    mkDerivation {
        name = "executor2000";
        nativeBuildInputs = [cmake bison perl ruby pkgconfig];
        buildInputs = [
            qt5.qtbase
            boost
            readline
            SDL2
            SDL
            wayland
            waylandpp
        ];
        src = pkgs.nix-gitignore.gitignoreSource [] ./.;
    }
) {}