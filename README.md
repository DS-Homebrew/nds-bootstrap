<p align="center">
 <img src="https://i.imgur.com/BFIu7xX.png"><br>
  <a href="https://gbatemp.net/threads/nds-bootstrap-loader-run-commercial-nds-backups-from-an-sd-card.454323/">
   <img src="https://img.shields.io/badge/GBATemp-Thread-blue.svg">
  </a>
  <a href="https://dev.azure.com/DS-Homebrew/Builds/_build?definitionId=12">
   <img src="https://dev.azure.com/DS-Homebrew/Builds/_apis/build/status/ahezard.nds-bootstrap?branchName=master" height="20">
  </a>
  <a href="https://discord.gg/yD3spjv">
   <img src="https://img.shields.io/badge/Discord%20Server-%23nds--bootstrap-green.svg">
  </a>
</p>

nds-bootstrap is an open-source application that allows Nintendo DS/DSi ROMs and homebrew to be natively utilised rather than using an emulator. nds-bootstrap works on Nintendo DSi/3DS SD cards through CFW and on Nintendo DS through flashcarts.

# ROM Compatibility

nds-bootstrap supports most DS ROMS, with a few exceptions. You can enhance your gaming experience with cheats and faster load times than general cartridges (for games that support those features). Game saving is supported too and will be saved in the `.sav` extention. If you find a bug, please report it in the [issues tab](https://github.com/ahezard/nds-bootstrap/issues).

Anti-Piracy patches can be loaded via IPS files, but they are not included inside the software itself.

nds-bootstrap also supports many homebrew applications, including games like DSCraft.

# Compiling

In order to compile this on your own, you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. devkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM devkitarm-rules general-tools dstools ndstool libnds libfat-nds
```

Once everything is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends
A frontend isn't required to be used as nds-bootstrap uses an ini file to load its parameters. However, it is very much recommended.

## [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)

TWiLight Menu++ is a frontend for nds-bootstrap, developed by [RocketRobz](https://github.com/RocketRobz). It has 5 customizable themes to choose from (the DSi Menu, the 3DS Home Menu, SEGA Saturn menu, the R4 kernel, and the Acekard theme, based on the AKAIO firmware).

It also includes a number of Anti-Piracy patches for the games and will automatically configure nds-boostrap for you, with customizable per game settings.
