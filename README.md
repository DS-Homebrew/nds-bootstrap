<p align="center">
   <img src="https://i.imgur.com/BFIu7xX.png"><br>
   <a href="https://gbatemp.net/threads/nds-bootstrap-loader-run-commercial-nds-backups-from-an-sd-card.454323/">
      <img src="https://img.shields.io/badge/GBAtemp-Thread-blue.svg" alt="GBAtemp thread">
   </a>
   <a href="https://discord.gg/yD3spjv">
      <img src="https://img.shields.io/badge/Discord%20Server-%23nds--bootstrap-green.svg" alt="Discord server: #nds-bootstrap">
   </a>
   <a href="https://github.com/DS-Homebrew/nds-bootstrap/actions/workflows/build.yml">
      <img src="https://github.com/DS-Homebrew/nds-bootstrap/actions/workflows/build.yml/badge.svg" alt="Build status on GitHub Actions">
   </a>
   <a title="Crowdin" target="_blank" href="https://crowdin.com/project/nds-bootstrap">
      <img src="https://badges.crowdin.net/nds-bootstrap/localized.svg" alt="Localization status on Crowdin">
   </a>
</p>

nds-bootstrap is an open-source application that allows Nintendo DS/DSi ROMs and homebrew to be natively utilised rather than using an emulator. nds-bootstrap works on Nintendo DSi/3DS SD cards through CFW and on Nintendo DS through flashcards.

# ROM Compatibility

nds-bootstrap supports most DS/DSi ROMs, with a few exceptions. You can enhance your gaming experience with cheats and faster load times than general cartridges (for games that support those features). Game saving is supported too and will be saved in the `.sav` extention, and `.pub` or `.prv` for DSiWare. If you find a bug, please report it in the [issues tab](https://github.com/ahezard/nds-bootstrap/issues). ROM compatibility is recorded in the [compatibility list](https://docs.google.com/spreadsheets/d/1LRTkXOUXraTMjg1eedz_f7b5jiuyMv2x6e_jY_nyHSc/edit#gid=0).

Anti-Piracy patches can be loaded via IPS files, but they are not included inside the software itself.

nds-bootstrap also supports many homebrew applications, including games like DScraft.

B4DS mode (a result of running nds-bootstrap on DS-mode flashcards with locked SCFG or DS Phat/lite) only supports some DS ROMs. You can increase compatibility by inserting a DS Memory Expansion Pak.

# Compiling

If your goal is to get a build of the latest commit, you can download that from our [TWLBot Builds repository](https://github.com/TWLBot/Builds). Also, if you push your commits to a GitHub fork, you can have GitHub Actions run on every commit that way. If, however, you'd like to compile locally, you will need to install devkitARM with the necessary Nintendo DS development libraries.

1. Install devkitPro's `pacman` package manager as described on the [devkitPro wiki](https://devkitpro.org/wiki/Getting_Started), then run the following command to install the needed libraries:
   ```
   sudo dkp-pacman -S nds-dev
   ```
   (Note: Command will vary by OS, `sudo` may not be needed and it may be just `pacman` instead)
2. Clone this repository using git (`git clone https://github.com/DS-Homebrew/nds-bootstrap.git`) and navigate to the cloned repo
3. Compile `lzss.c` to a directory in your PATH using a C compiler such as GCC (`gcc lzss.c -o /usr/local/bin/lzss`)
   - On Windows it must instead be `lzss.exe` in the root of the repository
4. Run `make package-nightly` to compile nds-bootstrap
   - The output files will be in the `bin` folder

If you need help compiling, please ask for help in our [Discord server](https://discord.gg/yD3spjv) or a [GitHub Discussion](https://github.com/DS-Homebrew/nds-bootstrap/discussions).

# Frontends

A frontend isn't required as nds-bootstrap uses an ini file to load its parameters. However, it is very much recommended.

## [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)

TWiLight Menu++ is a frontend for nds-bootstrap, developed by [RocketRobz](https://github.com/RocketRobz) & co. It has 7 customizable launchers to choose from with the ability to launch emulators and other homebrew.

It also includes a number of Anti-Piracy patches for the games and will automatically configure nds-bootstrap for you, with customizable per game settings.
