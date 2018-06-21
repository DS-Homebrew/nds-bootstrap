nds-bootstrap is an application that allows the usage of Nintendo DS Homebrew & ROMs through the SD cards of the Nintendo DS/3DS, rather than through emulators or flashcarts. Check the linked compatibility list to see which games are playable.

# Bug Reporting

Before reporting a bug, check the [compatibility list](https://docs.google.com/spreadsheets/d/1M7MxYQzVhb4604esdvo57a7crSvbGzFIdotLW0bm0Co/edit?usp=sharing) to see if it has already been reported.

A few questions one can answer:
1) How you encountered the bug
2) What happens when the bug occurs
3) If the bug is reproduce-able, what steps can one take in order to get there. 

Bugs can be reported in the [Github issues section](https://github.com/ahezard/nds-bootstrap/issues) or on our [Discord server](https://discordapp.com/invite/7bxTQfZ).

# Compiling

Firstly, if you want to self compile, check [travis](https://travis-ci.org/ahezard/nds-bootstrap) and see if it can compile successfully.

To self-compile this, you'll need to have DevKitPro with DevKitARM. You'll also need the latest libnds. Once all that is downloaded, git clone this repo, open command prompt, navigate to the folder, and type `make`. It should compile nds-bootstrap. If there is an error, let us know.

In order to use this, you'll need to get a frontend. Here are some frontends that we recommend:
- [TWLoader(Discontinued](https://github.com/Robz8/TWLoader) is a CTR-mode GUI that looks and feels like the Nintendo DS<sup>i</sup> menu, but the theme can be changed to R4 or akmenu/Wood.
- [nds-hb-menu](https://github.com/ahezard/nds-hb-menu/releases) is another 3DS frontend, but based off the Nintendo DS<sup>i</sup> homebrew menu.
- [DSIMenuPlusPlus](https://github.com/Robz8/SRLoader) is a port of the dsi menu to the 3ds,dsi,and ds<sup>i</sup>.
