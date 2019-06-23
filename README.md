<p align="center">
 <img src="https://i.imgur.com/BFIu7xX.png"><br>
  <a href="https://gbatemp.net/threads/nds-bootstrap-loader-run-commercial-nds-backups-from-an-sd-card.454323/">
   <img src="https://img.shields.io/badge/GBATemp-Thread-blue.svg">
  </a>
  <a href="https://dev.azure.com/DS-Homebrew/Builds/_build?definitionId=12">
   <img src="https://dev.azure.com/DS-Homebrew/Builds/_apis/build/status/ahezard.nds-bootstrap?branchName=master" height="20">
  </a>
  <a href="https://dshomebrew.serveo.net/">
   <img src="https://img.shields.io/badge/Rocket.Chat%20Server-%23nds--bootstrap-green.svg">
  </a>
</p>

nds-bootstrap is an open-source application that allows Nintendo DS/DSi ROMs and homebrew to be natively utilised rather than using an emulator. nds-bootstrap works on Nintendo DSi/3DS SD cards through CFW and on Nintendo DS through flashcarts.

# ROM Compatibility

nds-bootstrap supports most DS ROMS, with a few exceptions. You can enhance your gaming experience with cheats and faster load times than general cartridges (for games that support those features). Game saving is supported too and will be saved in the `.sav` extention. If you find a bug, please report it in the [issues tab](https://github.com/ahezard/nds-bootstrap/issues).

Be sure to manually patch out the Anti-Piracy functions though, as nds-bootstrap does not include patches of this sort.

nds-bootstrap also supports many homebrew applications, including emulators such as lameboy and NesDS.

# Compiling

In order to compile this on your own, you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. devkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM devkitarm-rules general-tools dstools ndstool libnds libfat-nds
```

Once everything is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends
A frontend isn't required to be used as nds-bootstrap uses an ini file to load its parameters. However, it is very much recommended.

## [TWiLight Menu++](https://github.com/RocketRobz/TWiLightMenu)

TWiLight Menu++ is a frontend for nds-bootstrap, developed by [RocketRobz](https://github.com/RocketRobz). It has 4 customizable themes to choose from (the DSi Menu, the 3DS Home Menu, the R4 kernel, and the Acekard theme, based on the AKAIO firmware).

# Rocket.Chat Server

Would you like a place to talk about your experiences with nds-bootstrap or need some assistance? Well, why not join our Rocket.Chat server!

Rocket.Chat is a self-hosted communication platform with the ability to share files and switch to an video/audio conferencing.

Come join today by following this link: https://dshomebrew.serveo.net ([alternative link](https://5a31a0c4.ngrok.io/))
