<p align="center">
 <img src="https://i.imgur.com/BFIu7xX.png"><br>
 <span style="padding-right: 5px;">
  <a href="https://travis-ci.org/ahezard/nds-bootstrap">
   <img src="https://travis-ci.org/ahezard/nds-bootstrap.svg?branch=master">
 <span style="padding-right: 5px;">
  <a href="https://dev.azure.com/DS-Homebrew/Builds/_build?definitionId=8">
   <img src="https://dev.azure.com/DS-Homebrew/Builds/_apis/build/status/ahezard.nds-bootstrap?branchName=b4ds">
  </a>
 </span>
 <span style="padding-left: 5px;">
  <a href="https://dshomebrew.serveo.net/">
   <img src="https://github.com/ahezard/nds-bootstrap/blob/master/images/Rocket.Chat button.png" height="20">
  </a>
 </span>
</p>

nds-bootstrap for DS (B4DS) is an open-source application that allows Nintendo DS ROMs to be natively utilised rather than using an emulator. This works on the Nintendo (3)DS(i) through flashcarts.

# ROM Compatibility

B4DS supports a small amount of DS ROMS. If you find a bug, please report it on the Rocket.Chat Server.

Be sure to manually patch out the Anti-Piracy functions though, as B4DS does not include patches of this sort.

# Compiling

In order to compile this on your own, you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. DevkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM general-tools dstools ndstool libnds libfat-nds
```

Once everything is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends

Since B4DS uses a .ini file to load the settings, a frontend isn't required. However, for ease of use, we recommend you use a frontend known as TWiLightMenu++. It's an open-source Nintendo DS, DSi, and 3DS home menu alternative made by RocketRobz that can also be used as a frontend for B4DS. It has a DSi theme, a 3DS theme, an R4 theme and an Acekard theme, as well as the ability to launch a large variety of ROMs including, NES, SNES, Game Boy, Game Boy Color, SEGA Master System, Game Gear, and Genesis, as well as Nintendo DS roms!

# Rocket.Chat Server

Would you like a place to talk about your experiences with B4DS? Do you need some assistance? Well, why not join our Rocket.Chat server!

Rocket.Chat is a self-hosted communication platform with the ability to share files and switch to an video/audio conferencing.

Come join today by following this link: https://dshomebrew.serveo.net/ ([alternative link](https://b2b38a00.ngrok.io))
