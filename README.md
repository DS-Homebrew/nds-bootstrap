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

# Compatibility

nds-bootstrap supports most DS ROMS, with a few exceptions. You can enhance your gaming experience with cheats and faster load times than general cartridges (for games that support those features). Game saving is supported too and will be saved in the `.sav` extention.

Be sure to manually patch out the Anti-Piracy functions though, as nds-bootstrap does not include the patches by default.

We also support many homebrew, including emulators such as lameboy & NesDS.

# Compiling

In order to compile this on your own, you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. DevkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM general-tools dstools ndstool libnds libfat-nds
```

Once all that is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends

Since nds-bootstrap uses a .ini file to load the settings, a front end isn't required. However, for ease of use, we recommend you get one of the following frontends:
- [TWiLight Menu++](https://github.com/RocketRobz/TWiLightMenu) is an open-source DS/DSi/3DS home menu alternative made by RocketRobz that can also be used as a nds-bootstrap frontend.
- [TWLoader](https://github.com/RocketRobz/TWLoader) is a discontinued 3DS-exclusive frontend for nds-bootstrap made by RocketRobz.
- [nds-hb-menu](https://github.com/ahezard/nds-hb-menu) is a frontend based off the Nintendo DSi homebrew menu.
