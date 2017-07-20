nds-bootstrap is a WIP project that allows people to play their Nintendo DS Homebrew & ROMs on a Nintendo DS<sup>i</sup> or on a Nintendo 3DS natively using an SD card without using a flashcart or an emulator. Even though compatibility isn't perfect yet, it can still play a handful of games.

# Help! I found a bug!!!

Firstly, if you found a bug with a game, check the [compatibility list](https://docs.google.com/spreadsheets/d/1M7MxYQzVhb4604esdvo57a7crSvbGzFIdotLW0bm0Co/edit?usp=sharing) to see if its already reported.     
Secondly, you need to explain your bug in detail. We don't know what's the problem if all you are going to say.     
If you met all these citeria's, you can report the bug in the [Github issues section](https://github.com/ahezard/nds-bootstrap/issues) or on our [Discord server](https://discordapp.com/invite/7bxTQfZ).

# Compiling

Firstly, if you want to self compile, check [travis](https://travis-ci.org/ahezard/nds-bootstrap) and see if it can compile successfully.

To self-compile this, you'll need to have DevKitPro with DevKitARM. You'll also need the latest libnds. Once all that is downloaded, git clone this repo, open command prompt, navigate to the folder, and type `make`. It should compile nds-bootstrap. If there is an error, let us know.

In order to use this, you'll need to get a frontend. Here are some frontends that we recommend:
- [TWLoader](https://github.com/Robz8/TWLoader) is a CTR-mode GUI that looks and feels like the Nintendo DS<sup>i</sup> menu, but the theme can be changed to R4 or akmenu/Wood.
- [nds-hb-menu](https://github.com/ahezard/nds-hb-menu/releases) is another 3DS frontend, but based off the Nintendo DS<sup>i</sup> homebrew menu.
- [SRLoader](https://github.com/Robz8/SRLoader) is a port of the famous TWLoader to the DS<sup>i</sup>.
