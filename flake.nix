{
  description = "flake for github.com:dekomote/vermouth";

  outputs =
    {
      nixpkgs,
      ...
    }:
    let
      forAllSystems =
        f:
        nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: {
        default = pkgs.stdenv.mkDerivation {
          pname = "vermouth";
          dontWrapQtApps = true;
          version = "1.9.2";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            kdePackages.extra-cmake-modules
            kdePackages.qttools
            icoutils
          ];

          buildInputs = with pkgs; [
            stdenv.cc.cc.lib
            kdePackages.qtbase
            kdePackages.qtdeclarative
            kdePackages.kirigami
            kdePackages.kcoreaddons
            kdePackages.ki18n
            kdePackages.qqc2-desktop-style
          ];
        };
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            kdePackages.extra-cmake-modules
            kdePackages.qttools
            icoutils
          ];

          buildInputs = with pkgs; [
            stdenv.cc.cc.lib
            kdePackages.qtbase
            kdePackages.qtdeclarative
            kdePackages.kirigami
            kdePackages.kcoreaddons
            kdePackages.ki18n
            kdePackages.qqc2-desktop-style
          ];

        };

      });

    };
}
