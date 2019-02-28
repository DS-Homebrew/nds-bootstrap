<p align="center">
 <img src="https://i.imgur.com/BFIu7xX.png"><br>
 <span style="padding-right: 5px;">
  <a href="https://travis-ci.org/ahezard/nds-bootstrap">
   <img src="https://travis-ci.org/ahezard/nds-bootstrap.svg?branch=master">
  </a>
 </span>
 <span style="padding-left: 5px;">
  <a href="https://dshomebrew.serveo.net/">
   <img src="https://i.imgur.com/fjHCL6s.png">
  </a>
 </span>
</p>

nds-bootstrap is an open-source application that allows Nintendo DS/DSi ROMs to be natively utilised rather than using an emulator. This works on Nintendo DSi/3DS SD cards through CFW and on Nintendo DS through flashcarts.

# ROM Compatibility

nds-bootstrap supports most DS ROMS, with a few exceptions. You can enhance your gaming experience with cheats and faster load times than general cartridges (for games that support those features). Game saving is supported too and will be saved in the `.sav` extention. If you find a bug, please report it in the [nds-bootstrap Github repository issues section](https://github.com/ahezard/nds-bootstrap/issues).

Be sure to manually patch out the Anti-Piracy functions though, as nds-bootstrap does not include the patches by default.

We also support many homebrew, including emulators such as lameboy & NesDS.

# Compiling

In order to compile this on your own, you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. DevkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM general-tools dstools ndstool libnds libfat-nds
```

Once all that is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends

Since nds-bootstrap uses a .ini file to load the settings, a front end isn't required. However, for ease of use, we recommend you get the frontend known as TWiLightMenu++. It's an open-souce Nintendo DS, DSi & 3DS home menu alternative made by RocketRobz that can also be used as a nds-bootstrap frontend. It has a DSi theme, a 3DS theme, an R4 theme and an Acekard theme, as well as the ability to launch NES, SEGA Genesis, SEGA Gamegear, Gameboy & SNES ROMS too as well as nintendo ds roms.

# Rocket.Chat server

Do you want a place to talk about all your experiences with nds-bootstrap? Do you want some assistance? Well, why not join our Rocket.Chat server!

Rocket.Chat is a self-hosted communication platform with the ability to share files and switch to an video/audio conferencing.

Come join today by following this link: https://dshomebrew.serveo.net/ ([alternative link](https://9e968c25.ngrok.io))
