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
  <a title="Crowdin" target="_blank" href="https://crowdin.com/project/nds-bootstrap"><img src="https://badges.crowdin.net/nds-bootstrap/localized.svg"></a>
</p>

nds-bootstrap is an open-source application that allows Nintendo DS/DSi ROMs and homebrew to be natively utilised rather than using an emulator. nds-bootstrap works on Nintendo DSi/3DS SD cards through CFW and on Nintendo DS through flashcarts.

# ROM Compatibility

nds-bootstrap supports most DS ROMs, with a few exceptions. You can enhance your gaming experience with cheats and faster load times than general cartridges (for games that support those features). Game saving is supported too and will be saved in the `.sav` extention. If you find a bug, please report it in the [issues tab](https://github.com/ahezard/nds-bootstrap/issues).

Anti-Piracy patches can be loaded via IPS files, but they are not included inside the software itself.

nds-bootstrap also supports many homebrew applications, including games like DSCraft.

B4DS (nds-bootstrap for DS-mode flashcards) only supports some DS ROMs. You can increase compatibility as well as enabling wireless/WiFi, by inserting a DS Memory Expansion Pak.

# Compiling

If your goal is to get the latest nightly, feel free to get it from our auto compiler, handled by TWLBot. Also, if you push your test commits to a GitHub forks, you could have GitHub Actions run on every commit instead. If, however, you'd like to compile this locally, you will need to install devkitARM with the necessary Nintendo DS development libraries. To learn how to set up, please check the [devkitPro wiki](https://devkitpro.org/wiki/Getting_Started).

```
 $ dkp-pacman -Syu devkitARM devkitarm-rules general-tools dstools ndstool libnds libfat-nds
```

You will also need to setup lzss, which can be done by installing gcc (`sudo apt-get install gcc`) and compiling lzss.c to the devKitARM binary directory (`sudo gcc lzss.c -o /opt/devkitpro/tools/bin/lzss`)

Once the development environment is ready, clone this repository using git (`git clone`), navigate to the cloned repo and run `make` to compile nds-bootstrap.

If you need help manually compiling, please let us know on our [Discord Server](https://discord.gg/yD3spjv). The issues tab on our GitHub repository is mainly for running the applications themselves, rather than development.

# Frontends
A frontend isn't required to be used as nds-bootstrap uses an ini file to load its parameters. However, it is very much recommended.

## [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)

TWiLight Menu++ is a frontend for nds-bootstrap, developed by [RocketRobz](https://github.com/RocketRobz) & co. It has 7 customizable launchers to choose from with the ability to launch emulators and other homebrew.

It also includes a number of Anti-Piracy patches for the games and will automatically configure nds-bootstrap for you, with customizable per game settings.
