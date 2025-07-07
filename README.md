<p align="center">
   <img src="https://github.com/DS-Homebrew/nds-bootstrap/blob/master/logo.png"><br>
   <a href="https://gbatemp.net/threads/nds-bootstrap-loader-run-commercial-nds-backups-from-an-sd-card.454323/">
      <img src="https://img.shields.io/badge/GBAtemp-Thread-blue.svg" alt="GBAtemp thread">
   </a>
   <a href="https://discord.gg/fCzqcWteC4">
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

B4DS mode (a result of running nds-bootstrap on DS-mode flashcards with locked SCFG or DS Phat/Lite) supports most (if not all) DS ROMs of which are supported on DSi/3DS. Some DSiWare ROMs are also supported (see [this list for which ones are supported](https://github.com/DS-Homebrew/TWiLightMenu/blob/master/universal/include/compatibleDSiWareMap.h)).

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

If you need help compiling, please ask for help in our [Discord server](https://discord.gg/fCzqcWteC4) or a [GitHub Discussion](https://github.com/DS-Homebrew/nds-bootstrap/discussions).

# Frontends

A frontend isn't required as nds-bootstrap uses an ini file to load its parameters. However, it is very much recommended.

## [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)

TWiLight Menu++ is a frontend for nds-bootstrap, developed by [Rocket Robz](https://github.com/RocketRobz) & co. It has 6 customizable launchers to choose from with the ability to launch emulators and other homebrew.

It will automatically configure nds-bootstrap for you, with customizable per game settings.

## [Forwarders](https://wiki.ds-homebrew.com/ds-index/forwarders)

Allows you to run games directly from the DSi Menu or 3DS HOME Menu. Some compatibility features from TWiLight Menu++ are missing in forwarders so if you have issues you may need to edit the per-game settings by holding <kbd>Y</kbd> while loading the forwarder.

### [YANBF](https://gbatemp.net/threads/606138/) (Yet Another nds-bootstrap Forwarder)

An alternative forwarder generator for 3DS users. YANBF forwarders are 3DS-mode applications so they count towards the normal 300 title limit insted of the smaller 40 title limit on DSi-mode applications, however they cannot have animated icons and take slightly longer to load.

# Credits
## Developers
- [Rocket Robz](https://github.com/RocketRobz): Lead developer, DSi mode and DSiWare support, B4DS mode, general maintenance and updates
- [shutterbug2000](https://github.com/shutterbug2000): SDK5 support, help with DSi mode support, and some other implemented stuff
- [ahezard](https://github.com/ahezard): Starting the project, former lead developer
- [Pk11](https://github.com/Epicpkmn11): In-game menu, screenshot taking, manual loading, and translation management
- [Gericom](https://github.com/Gericom): Improving B4DS compatibility, parts of libtwl code used, Pokémon Wii connection patch, and SD -> flashcard R/W patch for DSiWare

## Other
- [devkitPro](https://devkitpro.org): devkitARM and libnds
- [Arisotura](https://github.com/Arisotura): BIOS reader from [dsibiosdumper](https://github.com/Arisotura/dsibiosdumper) used in the in-game menu
- retrogamefan & Rudolph: Included AP-patches
   - [enler](https://github.com/enler): Fixing AP-patch for Pokemon Black 2 (Japan) for DS⁽ⁱ⁾ mode compatibility
   - [Rocket Robz](https://github.com/RocketRobz): Fixing some DS⁽ⁱ⁾-Enhanced game AP-patches for DS⁽ⁱ⁾ mode compatibility
- [VeaNika](https://github.com/VeaNika): DS Phat (NTR-001) color LUT from [GBARunner3](https://github.com/Gericom/GBARunner3)

## Translators
- Catalan: [Juan Adolfo Ortiz De Dompablo](https://crowdin.com/profile/kloido)
- Chinese Simplified: [James-Makoto](https://crowdin.com/profile/VCMOD55), [R-YaTian](https://github.com/R-YaTian)
- Chinese Traditional: [James-Makoto](https://crowdin.com/profile/VCMOD55), [R-YaTian](https://github.com/R-YaTian)
- Danish: [Sebastian øllgaard](https://crowdin.com/profile/seba187d), [Nadia Pedersen](https://crowdin.com/profile/nadiaholmquist)
- Dutch: [guusbuk](https://crowdin.com/profile/guusbuk), [TM-47](https://crowdin.com/profile/-tm-)
- French: [Dhalian](https://crowdin.com/profile/DHALiaN3630), [Fleefie~](https://crowdin.com/profile/fleefie), [LinuxCat](https://github.com/LinUwUxCat), [SombrAbsol](https://crowdin.com/profile/sombrabsol), [TM-47](https://crowdin.com/profile/-tm-)
- German: [TheDude](https://crowdin.com/profile/the6771), [TM-47](https://crowdin.com/profile/-tm-)
- Greek: [TM-47](https://crowdin.com/profile/-tm-)
- Hebrew: [Barawer](https://crowdin.com/profile/barawer), [Yaniv Levin](https://crowdin.com/profile/y4niv)
- Hungarian: [TM-47](https://crowdin.com/profile/-tm-), [Viktor Varga](http://github.com/vargaviktor)
- Indonesian: [heydootdoot](https://crowdin.com/profile/heydootdoot), [ZianoGG](https://crowdin.com/profile/zianogg)
- Italian: [TM-47](https://crowdin.com/profile/-tm-)
- Japanese: [Pk11](https://github.com/Epicpkmn11)
- Korean: [I'm Not Cry](https://crowdin.com/profile/cryental), [Myebyeol_NOTE](https://crowdin.com/profile/groovy-mint)
- Norwegian: [Nullified Block](https://crowdin.com/profile/elasderas123), [TM-47](https://crowdin.com/profile/-tm-)
- Polish: [Avginike](https://crowdin.com/profile/avginike), [gierkowiec tv](https://crowdin.com/profile/krystianbederz), [SdgJapteratoc](https://crowdin.com/profile/sdgjapteratoc), [TM-47](https://crowdin.com/profile/-tm-)
- Portuguese (Portugal): [Tavisc0](https://crowdin.com/profile/tavisc0)
- Portuguese (Brazil): [Tavisc0](https://crowdin.com/profile/tavisc0), [TM-47](https://crowdin.com/profile/-tm-)
- Romanian: [Tescu](https://crowdin.com/profile/tescu48)
- Russian: [Ckau](https://crowdin.com/profile/Ckau), [mixyt](https://crowdin.com/profile/mixyt), [Rolfie](https://crowdin.com/profile/rolfiee)
- Ryukyuan: [kuragehime](https://crowdin.com/profile/kuragehimekurara1)
- Spanish: [beta215](https://crowdin.com/profile/beta215), [Juan Adolfo Ortiz De Dompablo](https://crowdin.com/profile/kloido), [Nintendo R](https://crowdin.com/profile/nintendor), [nuxa17](https://twitter.com/TimeLordJean), [Radriant](https://ja.crowdin.com/profile/radriant), [SofyUchiha](https://crowdin.com/profile/sofyuchiha), [TM-47](https://crowdin.com/profile/-tm-)
- Swedish: [TM-47](https://crowdin.com/profile/-tm-)
- Turkish: [Egehan.TWL](https://crowdin.com/profile/egehan.twl), [rewold20](https://crowdin.com/profile/rewold20), [TM-47](https://crowdin.com/profile/-tm-)
- Ukrainian: [MichaelBest01](https://crowdin.com/profile/michaelbest01), [TM-47](https://crowdin.com/profile/-tm-), [вухаста гітара](https://crowdin.com/profile/earedguitr)
- Valencian: [Juan Adolfo Ortiz De Dompablo](https://crowdin.com/profile/kloido), [tsolo](https://crowdin.com/profile/tsolo)
