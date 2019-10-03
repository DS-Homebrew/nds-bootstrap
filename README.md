<p align="center">
 <img src="https://i.imgur.com/BFIu7xX.png"><br>
  <a href="https://gbatemp.net/threads/nds-bootstrap-loader-run-commercial-nds-backups-from-an-sd-card.454323/">
   <img src="https://img.shields.io/badge/GBATemp-Thread-blue.svg">
  </a>
  <a href="https://dev.azure.com/DS-Homebrew/Builds/_build?definitionId=12">
   <img src="https://dev.azure.com/DS-Homebrew/Builds/_apis/build/status/ahezard.nds-bootstrap?branchName=master" height="20">
  </a>
  <a href="https://discord.gg/yqSut8c">
   <img src="https://img.shields.io/badge/Discord%20Server-%23nds--bootstrap-green.svg">
  </a>
</p>

nds-bootstrap for DS (B4DS) is an open-source application that allows Nintendo DS ROMs to be natively utilised rather than using an emulator. This works on the Nintendo (3)DS(i) through flashcarts.

# ROM Compatibility

B4DS supports a small amount of DS ROMS. If you find a bug, please report it on [our Discord Server](https://discord.gg/yqSut8c).

Be sure to manually patch out the Anti-Piracy functions though, as B4DS does not include patches of this sort.

# Compiling

In order to compile this on your own, you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. devkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM general-tools dstools ndstool libnds libfat-nds
```

Once everything is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends

Since B4DS uses a .ini file to load the settings, a frontend isn't required. However, for ease of use, we recommend you use a frontend known as TWiLightMenu++. It's an open-source Nintendo DS, DSi, and 3DS home menu alternative made by RocketRobz that can also be used as a frontend for B4DS. It has a DSi theme, a 3DS theme, a SEGA Saturn theme, an R4 theme, and even an Acekard theme. It also has the ability to launch a large variety of ROMs including, NES, SNES, Game Boy, Game Boy Color, SEGA Master System, SEGA Game Gear, and SEGA Genesis, as well as Nintendo DS roms!
