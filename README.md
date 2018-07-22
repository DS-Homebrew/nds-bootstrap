nds-bootstrap is an application that allows the usage of Nintendo DS/DSi Homebrew and ROMs through the SD card of the Nintendo DSi/3DS, rather than through emulators or flashcarts.

# Bug Reporting

Before reporting a bug, check the [compatibility list](https://docs.google.com/spreadsheets/d/1M7MxYQzVhb4604esdvo57a7crSvbGzFIdotLW0bm0Co/edit?usp=sharing) to see if it has already been reported.

A few questions one can answer:
1) How you encountered the bug
2) What happens when the bug occurs
3) If the bug is reproduce-able, what steps can one take in order to get there. 

Bugs can be reported in the [Github issues section](https://github.com/ahezard/nds-bootstrap/issues) or on our [Discord server](https://discordapp.com/invite/7bxTQfZ).

# Compiling

Firstly, if you want to self compile, check [travis](https://travis-ci.org/ahezard/nds-bootstrap) and see if it can compile successfully.

To self-compile this you will need [devkitPro](https://devkitpro.org/) with the devkitARM toolchain, plus the necessary tools and libraries. DevkitPro includes `dkp-pacman` for easy installation of all components:

```
 $ dkp-pacman -Syu devkitARM general-tools dstools ndstool libnds libfat-nds
```

Once all that is downloaded and installed, `git clone` this repository, navigate to the folder, and run `make` to compile nds-bootstrap. If there is an error, let us know.

# Frontends

In order to use this, you'll need to get a frontend:
- [DSiMenu++](https://github.com/Robz8/SRLoader) (recommended) is an open-source DSi Menu upgrade/replacement for DS/DSi/3DS.
- [TWLoader](https://github.com/Robz8/TWLoader) is a discontinued 3DS frontend.
- [nds-hb-menu](https://github.com/ahezard/nds-hb-menu) is a 3DS frontend based off the Nintendo DSi homebrew menu.
