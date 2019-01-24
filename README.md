nds-bootstrap is an application that allows the usage of Nintendo DS/DSi Homebrew and ROMs through the SD card of the Nintendo DSi/3DS, rather than through emulators or flashcarts.

# Bug Reporting

Before reporting a bug, check the [compatibility list](https://docs.google.com/spreadsheets/d/1LRTkXOUXraTMjg1eedz_f7b5jiuyMv2x6e_jY_nyHSc/edit?usp=sharing) to see if it has already been reported.

A few questions one can answer:
1) How you encountered the bug
2) What happens when the bug occurs
3) If the bug is reproduce-able, what steps can one take in order to get there. 

Bugs can be reported in the [Github issues section](https://github.com/ahezard/nds-bootstrap/issues) or on our [Discord server](https://discord.gg/yqSut8c).

# Compiling

Firstly, if you want to self compile, check [travis](https://travis-ci.org/ahezard/nds-bootstrap) and see if it can compile successfully.

To self-compile this you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. DevkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM general-tools dstools ndstool libnds libfat-nds
```

Once all that is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends

For ease of use, we recommend you get one of the following frontends:
- [TWiLight Menu++](https://github.com/RocketRobz/TWiLightMenu) (recommended) is an open-source DSi Menu upgrade/replacement for DS/DSi/3DS made by RocketRobz that can also be used as a nds-bootstrap frontend.
- [TWLoader](https://github.com/RocketRobz/TWLoader) is a discontinued frontend for nds-bootstrap made by RocketRobz that's 3DS exclusive as it uses the CTR-Mode.
- [nds-hb-menu](https://github.com/ahezard/nds-hb-menu) is a frontend based off the Nintendo DSi homebrew menu.
