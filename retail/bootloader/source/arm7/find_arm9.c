#include <stddef.h> // NULL
#include "patch.h"
#include "nds_header.h"
#include "find.h"
#include "debug_file.h"

//#define memset __builtin_memset

//
// Subroutine function signatures ARM9
//

// Module params
static const u32 moduleParamsSignature[2] = {0xDEC00621, 0x2106C0DE};
static const u32 moduleParamsLtdSignature[2] = {0xDEC01463, 0x6314C0DE}; // SDK 5

// Card read
static const u32 cardReadEndSignature[2]            = {0x04100010, 0x040001A4}; // SDK < 4
static const u32 cardReadEndSignature3Elab[3]       = {0x04100010, 0x040001A4, 0xE92D4FF0}; // SDK 3
static const u32 cardReadEndSignatureAlt[2]         = {0x040001A4, 0x04100010};
static const u32 cardReadEndSignatureSdk2Alt[3]     = {0x040001A4, 0x04100010, 0xE92D000F}; // SDK 2
static const u32 cardReadEndSignatureAlt2[3]        = {0x040001A4, 0x040001A1, 0x04100010};
static const u32 cardReadEndSignature5Elab[4]       = {0x040001A4, 0x04100010, 0xE92D4010, 0xE59F00A0}; // SDK 5
static const u16 cardReadEndSignatureThumb[4]       = {0x01A4, 0x0400, 0x0200, 0x0000};
static const u16 cardReadEndSignatureThumb5[4]      = {0x01A4, 0x0400, 0xFE00, 0xFFFF};                                 // SDK 5
static const u16 cardReadEndSignatureThumb5Alt1[5]  = {0x01A4, 0x0400, 0x0010, 0x0410, 0xB510};                         // SDK 5
static const u32 cardReadStartSignature[1]          = {0xE92D4FF0};
static const u32 cardReadStartSignatureAlt[1]       = {0xE92D47F0};
static const u32 cardReadStartSignatureAlt2[1]      = {0xE92D4070};
static const u32 cardReadStartSignatureDebug[3]     = {0xE92D000F, 0xE92D47F0, 0xE24DD010};								// DEBUG
static const u32 cardReadStartSignatureDebugAlt[3]  = {0xE92D47F0, 0xE1A05000, 0xE59F40E4};								// DEBUG
static const u32 cardReadStartSignature5[1]         = {0xE92D4FF8};                                                     // SDK 5
static const u32 cardReadStartSignature5Alt[4]      = {0xE92D4010};													// SDK 5.5
static const u32 cardReadCheckSignatureMvDK4[3]     = {0xE5C02289, 0xE5C02288, 0xE5D0028A};
static const u16 cardReadStartSignatureThumb[2]     = {0xB5F8, 0xB082};
static const u16 cardReadStartSignatureThumbAlt[2]  = {0xB5F0, 0xB083};
static const u16 cardReadStartSignatureThumb5[1]    = {0xB5F0};                                                         // SDK 5
static const u16 cardReadStartSignatureThumb5Alt[1] = {0xB5F8};                                                         // SDK 5

// Card save command
static const u32 cardSaveCmdSignature2[4]          = {0xE92D47F0, 0xE59F60BC, 0xE1A0A000, 0xE1A09001};
static const u32 cardSaveCmdSignature2Debug[4]     = {0xE92D000F, 0xE92D4000, 0xE24DD00C, 0xE3A00001};
static const u32 cardSaveCmdSignature21[4]         = {0xE92D47F0, 0xE59F60B8, 0xE1A0A000, 0xE1A09001};
static const u16 cardSaveCmdSignatureThumb2[7]     = {0xB5F0, 0xB083, 0x1C06, 0x1C0F, 0x9200, 0x9301, 0x4D1F};
static const u32 cardSaveCmdSignature3[4]          = {0xE92D47F0, 0xE1A0A000, 0xE59F60D4, 0xE59F00D4}; // SDK 2.2 - 3
static const u32 cardSaveCmdSignature3Alt[4]       = {0xE92D43F8, 0xE1A09000, 0xE59F40C8, 0xE59F00C8}; // SDK 3-4
static const u16 cardSaveCmdSignatureThumb3[8]     = {0xB5F0, 0xB083, 0x1C06, 0x1C0F, 0x9200, 0x9301, 0x4D28, 0x4829}; // SDK 2.2 - 3
static const u16 cardSaveCmdSignatureThumb3Alt[8]  = {0xB5F0, 0xB085, 0x9000, 0x9101, 0x9202, 0x9303, 0x4D26, 0x4827};
static const u16 cardSaveCmdSignatureThumb3Alt2[4] = {0xB5F0, 0xB085, 0x9000, 0x4827}; // SDK 3-4
static const u32 cardSaveCmdSignature5[5]          = {0xE92D4070, 0xE1A05001, 0xE1A06000, 0xE59D1010, 0xE1A00003}; // SDK 5
static const u32 cardSaveCmdSignature5Alt[5]       = {0xE92D4070, 0xE59DC020, 0xE1A06000, 0xE1A04002, 0xE1A05001}; // SDK 5
static const u16 cardSaveCmdSignatureThumb5[5]     = {0xB570, 0x1C05, 0x9808, 0x1C0C, 0x1C16}; // SDK 5

//static const u32 instructionBHI[1] = {0x8A000001};

// Card pull out
static const u32 cardPullOutSignature1[4]         = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011}; // SDK <= 3
static const u32 cardPullOutSignature1Elab[5]     = {0xE92D4000, 0xE24DD004, 0xE201003F, 0xE3500011, 0x1A00000F}; // SDK 2
static const u32 cardPullOutSignature2Alt[4]      = {0xE92D000F, 0xE92D4030, 0xE24DD004, 0xE59D0014}; // SDK 2
static const u32 cardPullOutSignature4[4]         = {0xE92D4008, 0xE201003F, 0xE3500011, 0x1A00000D}; // SDK >= 4
static const u32 cardPullOutSignatureDebug[5]     = {0xE92D000F, 0xE92D4038, 0xE59D0014, 0xE200503F, 0xE3550011}; // SDK 4 (DEBUG)
static const u32 cardPullOutSignature5[4]         = {0xE92D4010, 0xE201003F, 0xE3500011, 0x1A000012}; // SDK 5
static const u32 cardPullOutSignature5Alt[4]      = {0xE92D4038, 0xE201003F, 0xE3500011, 0x1A000011}; // SDK 5
static const u32 cardPullOutSignatureDebug5[5]    = {0xE92D000F, 0xE92D4038, 0xE59D0014, 0xE200403F, 0xE3540011}; // SDK 5 (DEBUG)
static const u16 cardPullOutSignatureThumb[5]     = {0xB508, 0x203F, 0x4008, 0x2811, 0xD10E};
static const u16 cardPullOutSignatureThumbAlt[4]  = {0xB500, 0xB081, 0x203F, 0x4001};
static const u16 cardPullOutSignatureThumbAlt2[4] = {0xB5F8, 0x203F, 0x4008, 0x2811};
static const u16 cardPullOutSignatureThumb5[4]    = {0xB510, 0x203F, 0x4008, 0x2811};                 // SDK 5
static const u16 cardPullOutSignatureThumb5Alt[4] = {0xB538, 0x203F, 0x4008, 0x2811};                 // SDK 5

//static const u32 cardSendSignature[7] = {0xE92D40F0, 0xE24DD004, 0xE1A07000, 0xE1A06001, 0xE1A01007, 0xE3A0000E, 0xE3A02000};

// Force to power off
//static const u32 forceToPowerOffSignature[4] = {0xE92D4000, 0xE24DD004, 0xE59F0028, 0xE28D1000};

// Card id
static const u32 cardIdEndSignature[2]            = {0x040001A4, 0x04100010};
static const u32 cardIdEndSignature5[4]           = {0xE8BD8010, 0x02FFFAE0, 0x040001A4, 0x04100010}; // SDK 5
static const u32 cardIdEndSignature5Alt[3]        = {0x02FFFAE0, 0x040001A4, 0x04100010};             // SDK 5
static const u32 cardIdEndSignatureDebug5[4]      = {0x0AFFFFFA, 0xE59F0008, 0xE5900000, 0xE8BD8010};             // SDK 5
static const u16 cardIdEndSignatureThumb[6]       = {0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410};
static const u16 cardIdEndSignatureThumbAlt[6]    = {0xFFFF, 0xF8FF, 0x0000, 0xA700, 0xE000, 0xFFFF};
static const u16 cardIdEndSignatureThumb5[8]      = {0xFAE0, 0x02FF, 0xFFFF, 0xF8FF, 0x01A4, 0x0400, 0x0010, 0x0410}; // SDK 5
static const u32 cardIdStartSignature[1]          = {0xE92D4000};
static const u32 cardIdStartSignatureAlt1[1]      = {0xE92D4008};
static const u32 cardIdStartSignatureAlt2[1]      = {0xE92D4010};
static const u32 cardIdStartSignature5[2]         = {0xE92D4010, 0xE3A050B8}; // SDK 5
static const u32 cardIdStartSignature5Alt[2]      = {0xE92D4038, 0xE3A050B8}; // SDK 5
static const u16 cardIdStartSignatureThumb[2]     = {0xB500, 0xB081};
static const u16 cardIdStartSignatureThumbAlt1[2] = {0xB508, 0x202E};
static const u16 cardIdStartSignatureThumbAlt2[2] = {0xB508, 0x20B8};
static const u16 cardIdStartSignatureThumbAlt3[2] = {0xB510, 0x24B8};

// Card read DMA
static const u32 cardReadDmaEndSignature[2]          = {0x01FF8000, 0x000001FF};
static const u32 cardReadDmaEndSignatureSdk2Alt[2]   = {0x01FF8000, 0xE92D4030}; // SDK 2
static const u32 cardReadDmaEndSignatureDebug[3]     = {0xE28DD010, 0xE12FFF1E, 0x000001FF}; // DEBUG
static const u16 cardReadDmaEndSignatureThumbAlt[4]  = {0x8000, 0x01FF, 0x0000, 0x0200};
static const u32 cardReadDmaStartSignature[1]        = {0xE92D4FF8};
static const u32 cardReadDmaStartSignatureSdk2Alt[1] = {0xE92D4070};
static const u32 cardReadDmaStartSignatureAlt1[1]    = {0xE92D47F0};
static const u32 cardReadDmaStartSignatureAlt2[1]    = {0xE92D4FF0};
static const u32 cardReadDmaStartSignature5[1]       = {0xE92D43F8}; // SDK 5
static const u16 cardReadDmaStartSignatureThumb1[1]  = {0xB5F0}; // SDK <= 2
static const u16 cardReadDmaStartSignatureThumb3[1]  = {0xB5F8}; // SDK >= 3

// Card end read DMA
static const u16 cardEndReadDmaSignatureThumb3[1]  = {0x481E};
static const u32 cardEndReadDmaSignature4[1]    = {0xE3A00702};
static const u32 cardEndReadDmaSignature4Alt[1] = {0xE3A04702};
static const u16 cardEndReadDmaSignatureThumb4[2]  = {0x2002, 0x0480};
static const u32 cardEndReadDmaSignature5[4]  = {0xE59F0010, 0xE3A02000, 0xE5901000, 0xE5812000};
static const u16 cardEndReadDmaSignatureThumb5[4]  = {0x4803, 0x2200, 0x6801, 0x600A};

// Card set DMA
static const u32 cardSetDmaSignatureValue1[1]       = {0x4100010};
static const u32 cardSetDmaSignatureValue2[1]       = {0x40001A4};
static const u16 cardSetDmaSignatureStartThumb3[4]  = {0xB510, 0x4C0A, 0x6AA0, 0x490A};
static const u16 cardSetDmaSignatureStartThumb4[4]  = {0xB538, 0x4D0A, 0x2302, 0x6AA8};
static const u32 cardSetDmaSignatureStart2Early[4]  = {0xE92D4000, 0xE24DD004, 0xE59FC054, 0xE59F1054};
static const u32 cardSetDmaSignatureStart2[3]       = {0xE92D4010, 0xE59F403C, 0xE59F103C};
static const u32 cardSetDmaSignatureStart3[3]       = {0xE92D4010, 0xE59F4038, 0xE59F1038};
static const u32 cardSetDmaSignatureStart4[3]       = {0xE92D4038, 0xE59F4038, 0xE59F1038};
static const u32 cardSetDmaSignatureStart4Alt[3]    = {0xE92D4038, 0xE59F5038, 0xE59F1038};
static const u32 cardSetDmaSignatureStart5[2]       = {0xE92D4070, 0xE1A06000};
static const u32 cardSetDmaSignatureStart5Alt[2]    = {0xE92D4038, 0xE1A05000};
static const u16 cardSetDmaSignatureStartThumb5[2]  = {0xB570, 0x1C05};

// Random patch
static const u32 randomPatchSignature[4]        = {0xE3500000, 0x1597002C, 0x10406004, 0x03E06000};
// static const u32 randomPatchSignature5First[4]  = {0xE92D43F8, 0xE3A04000, 0xE1A09001, 0xE1A08002}; // SDK 5
static const u32 randomPatchSignature5Second[3] = {0xE59F003C, 0xE590001C, 0xE3500000};             // SDK 5

// FileIO functions (SDK 5)
static const u32 fileIoOpenSignature50[4]      = {0xE92D40F8, 0xE24DDF4A, 0xE1A04000, 0xE1A07002}; // SDK 5.0 - 5.2
static const u32 fileIoOpenSignature[4]        = {0xE92D43F8, 0xE24DDF4A, 0xE28D4010, 0xE1A07000}; // SDK 5.1+
static const u16 fileIoOpenSignatureThumb[4]   = {0xB5F8, 0xB0CA, 0x1C05, 0x1C16}; // SDK 5.1+
static const u32 fileIoCloseSignature[4]       = {0xE59FC008, 0xE3A01008, 0xE3A02001, 0xE12FFF1C}; // SDK 5.x
static const u16 fileIoCloseSignatureThumb[4]  = {0x4B01, 0x2108, 0x2201, 0x4718}; // SDK 5.x
static const u32 fileIoSeekSignature50[4]      = {0xE92D4008, 0xE24DD008, 0xE28D3000, 0xE5803010}; // SDK 5.0, SDK 5.1+ alt
static const u32 fileIoSeekSignature[4]        = {0xE92D4070, 0xE24DD008, 0xE1A06000, 0xE1A05001}; // SDK 5.3+
static const u16 fileIoSeekSignatureThumb[4]   = {0xB508, 0xB082, 0xAB00, 0x6103}; // SDK 5.x
static const u32 fileIoReadSignature50[4]      = {0xE92D4010, 0xE24DD008, 0xE59F305C, 0xE1A04000}; // SDK 5.0
static const u32 fileIoReadSignature51[4]      = {0xE92D4010, 0xE24DD008, 0xE28D3000, 0xE1A04000}; // SDK 5.0 - 5.2
static const u32 fileIoReadSignature[4]        = {0xE92D4038, 0xE24DD008, 0xE28D3000, 0xE1A05000}; // SDK 5.1+
static const u16 fileIoReadSignature51Thumb[4] = {0xB510, 0xB082, 0x1C04, 0xAB00}; // SDK 5.1
static const u16 fileIoReadSignatureThumb[4]   = {0xB538, 0xB082, 0x1C05, 0xAB00}; // SDK 5.?

// Init Lock (SDK 5)
static const u32 initLockEndSignature[2]      = {0x02FFFFB0, 0x04000204};
static const u32 initLockEndSignatureThumb[3] = {0x02FFFFB0, 0xFFFF0000, 0x04000204};
static const u32 initLockEndSignatureDebug[3] = {0x02FFFFB0, 0x02FFFFB4, 0x02FFFFC0};

// irq enable
static const u32 irqEnableStartSignature1[4]        = {0xE59FC028, 0xE3A01000, 0xE1DC30B0, 0xE59F2020};					// SDK <= 3
static const u32 irqEnableStartSignature2Alt[4]     = {0xE92D000F, 0xE92D4030, 0xE24DD004, 0xEBFFFFDB};					// SDK 2
static const u32 irqEnableStartSignature4[4]        = {0xE59F3024, 0xE3A01000, 0xE1D320B0, 0xE1C310B0};					// SDK >= 4
static const u32 irqEnableStartSignature4Debug[4]   = {0xE92D000F, 0xE92D4038, 0xEBFFFFE5, 0xE1A05000};					// SDK >= 4 (DEBUG)
static const u32 irqEnableStartSignatureThumb[5]    = {0x4D07B430, 0x2100882C, 0x4B068029, 0x1C11681A, 0x60194301};		// SDK <= 3
static const u32 irqEnableStartSignatureThumbAlt[4] = {0x4C07B418, 0x88232100, 0x32081C22, 0x68118021};					// SDK >= 3

// Mpu cache
static const u32 mpuInitRegion0Signature[1] = {0xEE060F10};
static const u32 mpuInitRegion0Data[1]      = {0x4000033};
static const u32 mpuInitRegion1Signature[1] = {0xEE060F11};
static const u32 mpuInitRegion1Data1[1]     = {0x200002D}; // SDK <= 4
static const u32 mpuInitRegion1DataAlt[1]   = {0x200002B};
static const u32 mpuInitRegion1Data5[1]     = {0x2000031}; // SDK 5
static const u32 mpuInitRegion2Signature[1] = {0xEE060F12};
static const u32 mpuInitRegion2Data1[1]     = {0x27C0023}; // SDK <= 2
static const u32 mpuInitRegion2Data3[1]     = {0x27E0021}; // SDK >= 2 (Late)
static const u32 mpuInitRegion2Data5[1]     = {0x27FF017}; // SDK 5
static const u32 mpuInitRegion3Signature[1] = {0xEE060F13};
static const u32 mpuInitRegion3Data[1]      = {0x8000035};
static const u32 mpuFlagsSetSignature[4]    = {0xE3A0004A, 0xEE020F30, 0xE3A0004A, 0xEE020F10}; // SDK 5
static const u32 mpuCodeCacheChangeSignature[4] = {0xEE121F30, 0xE1811000, 0xEE021F30, 0xE12FFF1E}; // SDK 5
static const u32 mpuChangeRegion1Signature[3]         = {0xE3A00001, 0xE3A01402, 0xE3A0202A};
static const u32 mpuChangeRegion1SignatureAlt[3]      = {0x03A0202C, 0xE3A00001, 0xE3A01402};
static const u16 mpuChangeRegion1SignatureThumb[3]    = {0x2001, 0x0609, 0x222A};
static const u16 mpuChangeRegion1SignatureThumbAlt[3] = {0x2001, 0x0621, 0x222A};

// Init Heap
static const u32 initHeapEndSignature1[2]              = {0x27FF000, 0x37F8000};
static const u32 initHeapEndSignature5[2]              = {0x2FFF000, 0x37F8000};
static const u32 initHeapEndFuncSignature[1]           = {0xE12FFF1E};
static const u32 initHeapEndFunc2Signature[2]          = {0xE12FFF1E, 0x023E0000};
static const u32 initHeapEndFuncSignatureAlt[1]        = {0xE8BD8008};
static const u32 initHeapEndFunc2SignatureAlt1[2]      = {0xE8BD8000, 0x023E0000};
static const u32 initHeapEndFunc2SignatureAlt2[2]      = {0xE8BD8008, 0x023E0000};
static const u32 initHeapEndFunc2SignatureAlt3[2]      = {0xE8BD8010, 0x023E0000};
static const u16 initHeapEndFuncSignatureThumb[1]      = {0xBD08};
static const u16 initHeapEndFuncSignatureThumbAlt[1]   = {0x4718};
static const u32 initHeapEndFunc2SignatureThumb[2]     = {0xBD082000, 0x023E0000};
static const u32 initHeapEndFunc2SignatureThumbAlt1[2] = {0x46C04718, 0x023E0000};
static const u32 initHeapEndFunc2SignatureThumbAlt2[2] = {0xBD082010, 0x023E0000};
static const u32 initHeapEndFunc2SignatureThumbAlt3[2] = {0xBD102000, 0x023E0000};

// Reset
static const u32 resetSignature2[4]     = {0xE92D4030, 0xE24DD004, 0xE59F1090, 0xE1A05000}; // sdk2
static const u32 resetSignature2Alt1[4] = {0xE92D000F, 0xE92D4010, 0xEB000026, 0xE3500000}; // sdk2
static const u32 resetSignature2Alt2[4] = {0xE92D4010, 0xE59F1078, 0xE1A04000, 0xE1D100B0}; // sdk2
static const u32 resetSignature3[4]     = {0xE92D4010, 0xE59F106C, 0xE1A04000, 0xE1D100B0}; // sdk3
static const u32 resetSignature3Alt[4]  = {0xE92D4010, 0xE59F1068, 0xE1A04000, 0xE1D100B0}; // sdk3 and sdk4
static const u32 resetSignature3Eoo[2]  = {0xE92D4010, 0xE1A04000}; // eoo.dat (Pokemon)
static const u32 resetSignature4[4]     = {0xE92D4070, 0xE59F10A0, 0xE1A04000, 0xE1D100B0}; // sdk4
static const u32 resetSignature4Alt[4]  = {0xE92D4010, 0xE59F1084, 0xE1A04000, 0xE1D100B0}; // sdk4
static const u32 resetSignature5[4]     = {0xE92D4038, 0xE59F1054, 0xE1A05000, 0xE1D100B0}; // sdk5
static const u32 resetSignature5Alt1[4] = {0xE92D4010, 0xE59F104C, 0xE1A04000, 0xE1D100B0}; // sdk2 and sdk5
static const u32 resetSignature5Alt2[4] = {0xE92D4010, 0xE59F1088, 0xE1A04000, 0xE1D100B0}; // sdk5
static const u32 resetSignature5Alt3[4] = {0xE92D4038, 0xE59F106C, 0xE1A05000, 0xE1D100B0}; // sdk5
static const u32 resetSignature5Alt4[4] = {0xE92D4038, 0xE59F1090, 0xE1A05000, 0xE1D100B0}; // sdk5

static const u32 resetConstant[1]       = {RESET_PARAM};
static const u32 resetConstant5[1]      = {RESET_PARAM_SDK5};

// SRL start
static const u32 srlStartSignature3[4]  = {0xE92D4010, 0xE59F003C, 0xE5904000, 0xE3540000}; // eoo.dat (Pokemon)

// Reset (TWL)
static const u32 nandTmpJumpFuncStart30[1]  = {0xE92D000F};
static const u32 nandTmpJumpFuncStart3[1]   = {0xE92D4008};
static const u32 nandTmpJumpFuncStart4[1]   = {0xE92D4010};

static const u32 nandTmpJumpFuncConstant[1] = {0x02FFDFC0};

// Sleep mode trigger (TWL)
static const u32 twlSleepModeEndSignatureEarly[4]      = {0xE2855001, 0xE3550003, 0xE286600C, 0x9AFFFFE2}; // SDK 5.1 & 5.2
static const u32 twlSleepModeEndSignature[3]           = {0xE2866001, 0xE3560003, 0x9AFFFFDF}; // SDK 5.3+
static const u16 twlSleepModeEndSignatureThumbEarly[7] = {0x9800, 0x1C64, 0x1D00, 0x350C, 0x9000, 0x2C03, 0xD9CE}; // SDK 5.1 & 5.2
static const u16 twlSleepModeEndSignatureThumb[3]      = {0x1C6D, 0x2D03, 0xD9D1}; // SDK 5.3+

// TWL Shared Font
static const char* nandSharedFontSignature = "nand:/<sharedFont>";

// Panic
// TODO : could be a good idea to catch the call to Panic function and store the message somewhere


extern u32 iUncompressedSize;

extern bool isPawsAndClaws(const tNDSHeader* ndsHeader);

u32* findModuleParamsOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findModuleParamsOffset:\n");

	u32* moduleParamsOffset = findOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		moduleParamsSignature, 2
	);
	if (moduleParamsOffset) {
		dbg_printf("Module params offset found\n");
	} else {
		dbg_printf("Module params offset not found\n");
	}

	return moduleParamsOffset;
}

u32* findLtdModuleParamsOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findLtdModuleParamsOffset:\n");

	u32* moduleParamsOffset = findOffset(
		(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
		moduleParamsLtdSignature, 2
	);
	if (moduleParamsOffset) {
		dbg_printf("Ltd module params offset found\n");
	} else {
		dbg_printf("Ltd module params offset not found\n");
	}

	return moduleParamsOffset;
}

u32* findCardReadEndOffsetType0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	dbg_printf("findCardReadEndOffsetType0:\n");

	const char* romTid = getRomTid(ndsHeader);

	u32* cardReadEndOffset = NULL;
	if ((moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4008000) || isPawsAndClaws(ndsHeader)) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignature3Elab, 3
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end (type 0) elaborate found: ");
		} else {
			dbg_printf("ARM9 Card read end (type 0) elaborate not found\n");
		}
	}

	if (!cardReadEndOffset && moduleParams->sdk_version < 0x2008000) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignatureSdk2Alt, 3
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end SDK 2 alt found: ");
		} else {
			dbg_printf("ARM9 Card read end SDK 2 alt not found\n");
		}
	}

	if (!cardReadEndOffset && strncmp(romTid, "UOR", 3) != 0 && (moduleParams->sdk_version < 0x4008000 || moduleParams->sdk_version > 0x5000000)) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignature, 2
		);
		if (cardReadEndOffset) {
			dbg_printf("ARM9 Card read end (type 0) short found: ");
		} else {
			dbg_printf("ARM9 Card read end (type 0) short not found\n");
		}
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadEndOffsetType1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	dbg_printf("findCardReadEndOffsetType1:\n");

	// const char* romTid = getRomTid(ndsHeader);

	u32* cardReadEndOffset = NULL;
	//readType = 1;
	if (isSdk5(moduleParams)) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignature5Elab, 4
		);
	}
	if (!cardReadEndOffset) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignatureAlt, 2
		);
	}

	if (!cardReadEndOffset) {
		cardReadEndOffset = findOffset(
			(u32*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
			cardReadEndSignatureAlt2, 3
		);
	}

	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end alt (type 1) found: ");
	} else {
		dbg_printf("ARM9 Card read end alt (type 1) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u16* findCardReadEndOffsetThumb(const tNDSHeader* ndsHeader, u32 startOffset) {
	dbg_printf("findCardReadEndOffsetThumb:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb, 4
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end thumb found: ");
	} else {
		dbg_printf("ARM9 Card read end thumb not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

// SDK 5
u16* findCardReadEndOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findCardReadEndOffsetThumb5Type1:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb5, 4
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end SDK 5 thumb (type 1) found: ");
	} else {
		dbg_printf("ARM9 Card read end SDK 5 thumb (type 1) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

// SDK 5
u16* findCardReadEndOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, u32 startOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	dbg_printf("findCardReadEndOffsetThumb5Type0:\n");

	//usesThumb = true;

	u16* cardReadEndOffset = findOffsetThumb(
		(u16*)startOffset, iUncompressedSize-(startOffset-0x02000000),//ndsHeader->arm9binarySize,
		cardReadEndSignatureThumb5Alt1, 5
	);
	if (cardReadEndOffset) {
		dbg_printf("ARM9 Card read end SDK 5 thumb alt 1 (type 0) found: ");
	} else {
		dbg_printf("ARM9 Card read end SDK 5 thumb alt 1 (type 0) not found\n");
	}

	if (cardReadEndOffset) {
		dbg_hexa((u32)cardReadEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadEndOffset;
}

u32* findCardReadStartOffsetType0(const module_params_t* moduleParams, const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetType0:\n");

	//if (readType != 1) {

	u32* cardReadStartOffset = findOffsetBackwards(
		cardReadEndOffset, 0x118,
		cardReadStartSignature, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read start (type 0) not found\n");
	}

	if (!cardReadStartOffset && moduleParams->sdk_version < 0x2008000) {
		cardReadStartOffset = findOffsetBackwards(
			cardReadEndOffset, 0x118,
			cardReadStartSignatureAlt, 1
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start alt 1 (type 0) found\n");
		} else {
			dbg_printf("ARM9 Card read start alt 1 (type 0) not found\n");
		}
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardReadStartOffsetType1(const u32* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetType1:\n");

	//if (readType == 1) {
	u32* cardReadStartOffset = findOffsetBackwards(
		cardReadEndOffset, 0x118,
		cardReadStartSignatureAlt2, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start alt 2 (type 1) found\n");
	} else {
		dbg_printf("ARM9 Card read start alt 2 (type 1) not found\n");
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwards(
			cardReadEndOffset, 0x178,
			cardReadStartSignatureDebug, 3
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start DEBUG (type 1) found\n");
		} else {
			dbg_printf("ARM9 Card read start DEBUG (type 1) not found\n");
		}
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwards(
			cardReadEndOffset, 0x118,
			cardReadStartSignatureDebugAlt, 3
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start DEBUG alt (type 1) found\n");
		} else {
			dbg_printf("ARM9 Card read start DEBUG alt (type 1) not found\n");
		}
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

// SDK 5
u32* findCardReadStartOffset5(const module_params_t* moduleParams, const u32* cardReadEndOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffset5:\n");

	u32* cardReadStartOffset = findOffsetBackwards(
		(u32*)cardReadEndOffset, 0x120,
		cardReadStartSignature5, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start SDK 5 found\n");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 not found\n");
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwards(
			(u32*)cardReadEndOffset, 0x120,
			cardReadStartSignature5Alt, 1
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start SDK 5.5 found\n");
		} else {
			dbg_printf("ARM9 Card read start SDK 5.5 not found\n");
		}
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardReadCheckOffsetMvDK4(u32 startOffset) {
	dbg_printf("findCardReadCheckOffsetMvDK4:\n");

	u32* cardReadStartOffset = findOffset(
		(u32*)startOffset, 0x20000,
		cardReadCheckSignatureMvDK4, 3
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read check (MvDK4) found\n");
	} else {
		dbg_printf("ARM9 Card read check (MvDK4) not found\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u16* findCardReadStartOffsetThumb(const u16* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetThumb:\n");

	u16* cardReadStartOffset = findOffsetBackwardsThumb(
		cardReadEndOffset, 0x120,
		cardReadStartSignatureThumb, 2
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start thumb found\n");
	} else {
		dbg_printf("ARM9 Card read start thumb not found\n");
	}

	if (!cardReadStartOffset) {
		cardReadStartOffset = findOffsetBackwardsThumb(
			cardReadEndOffset, 0xC0,
			cardReadStartSignatureThumbAlt, 2
		);
		if (cardReadStartOffset) {
			dbg_printf("ARM9 Card read start thumb alt found\n");
		} else {
			dbg_printf("ARM9 Card read start thumb alt not found\n");
		}
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

// SDK 5
u16* findCardReadStartOffsetThumb5Type0(const module_params_t* moduleParams, const u16* cardReadEndOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetThumb5Type0:\n");

	u16* cardReadStartOffset = findOffsetBackwardsThumb(
		cardReadEndOffset, 0xD0,
		cardReadStartSignatureThumb5, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start SDK 5 thumb (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 thumb (type 0) not found\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

// SDK 5
u16* findCardReadStartOffsetThumb5Type1(const module_params_t* moduleParams, const u16* cardReadEndOffset) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadStartOffsetThumb5Type1:\n");

	u16* cardReadStartOffset = findOffsetBackwardsThumb(
		(u16*)cardReadEndOffset, 0xD0,
		cardReadStartSignatureThumb5Alt, 1
	);
	if (cardReadStartOffset) {
		dbg_printf("ARM9 Card read start SDK 5 thumb alt (type 0) found\n");
	} else {
		dbg_printf("ARM9 Card read start SDK 5 thumb alt (type 0) not found\n");
	}

	dbg_printf("\n");
	return cardReadStartOffset;
}

u32* findCardSaveCmdOffset2(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardSaveCmdOffset2:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		cardSaveCmdSignature2, 4
	);
	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignature2Debug, 4
		);
	}
	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignature21, 4
		);
	}
	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignatureThumb2, 7
		);
	}
	if (offset) {
		dbg_printf("Card save command found\n");
	} else {
		dbg_printf("Card save command not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findCardSaveCmdOffset3(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardSaveCmdOffset3:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		cardSaveCmdSignature3, 4
	);
	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignature3Alt, 4
		);
	}
	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignatureThumb3, 8
		);
	}
	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignatureThumb3Alt, 8
		);
	}
	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignatureThumb3Alt2, 4
		);
	}
	if (offset) {
		dbg_printf("Card save command found\n");
	} else {
		dbg_printf("Card save command not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findCardSaveCmdOffset5(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardSaveCmdOffset5:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		cardSaveCmdSignature5, 5
	);
	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignature5Alt, 5
		);
	}
	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardSaveCmdSignatureThumb5, 5
		);
	}
	if (offset) {
		dbg_printf("Card save command found\n");
	} else {
		dbg_printf("Card save command not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findCardPullOutOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardPullOutOffset:\n");

	//if (!usesThumb) {
	
	u32* cardPullOutOffset = 0;
	if (moduleParams->sdk_version > 0x5000000) { // SDK 5
		cardPullOutOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardPullOutSignature5, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler SDK 5 found\n");
		} else {
			dbg_printf("Card pull out handler SDK 5 not found\n");
		}

		if (!cardPullOutOffset) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature5Alt, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 5 alt found\n");
			} else {
				dbg_printf("Card pull out handler SDK 5 alt not found\n");
			}
		}

		if (!cardPullOutOffset) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignatureDebug5, 5
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 5 DEBUG found\n");
			} else {
				dbg_printf("Card pull out handler SDK 5 DEBUG not found\n");
			}
		}
	} else {
		if (moduleParams->sdk_version > 0x2008000 && moduleParams->sdk_version < 0x3000000) {
			// SDK 2
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature1Elab, 5
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 2 elaborate found\n");
			} else {
				dbg_printf("Card pull out handler SDK 2 elaborate not found\n");
			}
		}

		if (!cardPullOutOffset && moduleParams->sdk_version < 0x4000000) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature1, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler found\n");
			} else {
				dbg_printf("Card pull out handler not found\n");
			}
		}

		if (!cardPullOutOffset && moduleParams->sdk_version < 0x2008000) {
			// SDK 2
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature2Alt, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 2 alt found\n");
			} else {
				dbg_printf("Card pull out handler SDK 2 alt not found\n");
			}
		}

		if (!cardPullOutOffset) {
			// SDK 4
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature4, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler SDK 4 found\n");
			} else {
				dbg_printf("Card pull out handler SDK 4 not found\n");
			}
		}

		if (!cardPullOutOffset && moduleParams->sdk_version > 0x4000000) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignature1, 4
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler found\n");
			} else {
				dbg_printf("Card pull out handler not found\n");
			}
		}

		if (!cardPullOutOffset) {
			cardPullOutOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				cardPullOutSignatureDebug, 5
			);
			if (cardPullOutOffset) {
				dbg_printf("Card pull out handler DEBUG found\n");
			} else {
				dbg_printf("Card pull out handler DEBUG not found\n");
			}
		}
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

u16* findCardPullOutOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardPullOutOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb, 5
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler thumb found\n");
	} else {
		dbg_printf("Card pull out handler thumb not found\n");
	}

	if (!cardPullOutOffset) {
		cardPullOutOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardPullOutSignatureThumbAlt, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler thumb alt found\n");
		} else {
			dbg_printf("Card pull out handler thumb alt not found\n");
		}
	}

	if (!cardPullOutOffset) {
		cardPullOutOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardPullOutSignatureThumbAlt2, 4
		);
		if (cardPullOutOffset) {
			dbg_printf("Card pull out handler thumb alt 2 found\n");
		} else {
			dbg_printf("Card pull out handler thumb alt 2 not found\n");
		}
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

// SDK 5
u16* findCardPullOutOffsetThumb5Type0(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}

	dbg_printf("findCardPullOutOffsetThumbType0:\n");
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb5, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler SDK 5 thumb (type 0) found\n");
	} else {
		dbg_printf("Card pull out handler SDK 5 thumb (type 0) not found\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

// SDK 5
u16* findCardPullOutOffsetThumb5Type1(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	if (moduleParams->sdk_version < 0x5000000) {
		return NULL;
	}
	
	dbg_printf("findCardPullOutOffsetThumbType1:\n");
	
	u16* cardPullOutOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
		cardPullOutSignatureThumb5Alt, 4
	);
	if (cardPullOutOffset) {
		dbg_printf("Card pull out handler SDK 5 thumb alt (type 1) found\n");
	} else {
		dbg_printf("Card pull out handler SDK 5 thumb alt (type 1) not found\n");
	}

	dbg_printf("\n");
	return cardPullOutOffset;
}

/*u32* findForceToPowerOffOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findForceToPowerOffOffset:\n");

	u32 forceToPowerOffOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		forceToPowerOffSignature, 4
	);
	if (forceToPowerOffOffset) {
		dbg_printf("Force to power off handler found: ");
	} else {
		dbg_printf("Force to power off handler not found\n");
	}

	if (forceToPowerOffOffset) {
		dbg_hexa((u32)forceToPowerOffOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return forceToPowerOffOffset;
}*/

u32* findCardIdEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u32* cardReadEndOffset) {
	if (!isPawsAndClaws(ndsHeader) && !cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdEndOffset:\n");

	u32* cardIdEndOffset = NULL;

	if (isSdk5(moduleParams)) {
		if (cardReadEndOffset) {
			cardIdEndOffset = findOffsetBackwards(
				(u32*)cardReadEndOffset, 0x800,
				cardIdEndSignature5, 4
			);
		} else {
			cardIdEndOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,
				cardIdEndSignature5, 4
			);
		}
		if (cardIdEndOffset) {
			dbg_printf("Card ID end SDK 5 found: ");
		} else {
			dbg_printf("Card ID end SDK 5 not found\n");
		}

		if (!cardIdEndOffset) {
			cardIdEndOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,
				cardIdEndSignature5Alt, 3
			);
			if (cardIdEndOffset) {
				dbg_printf("Card ID end SDK 5 alt found: ");
			} else {
				dbg_printf("Card ID end SDK 5 alt not found\n");
			}
		}

		if (!cardIdEndOffset) {
			if (cardReadEndOffset) {
				cardIdEndOffset = findOffsetBackwards(
					(u32*)cardReadEndOffset, 0x800,
					cardIdEndSignatureDebug5, 4
				);
			} else {
				cardIdEndOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,
					cardIdEndSignatureDebug5, 4
				);
			}
			if (cardIdEndOffset) {
				dbg_printf("Card ID end SDK 5 DEBUG found: ");
			} else {
				dbg_printf("Card ID end SDK 5 DEBUG not found\n");
			}
		}
	} else {
		if (cardReadEndOffset) {
			cardIdEndOffset = findOffset(
				(u32*)cardReadEndOffset + 0x10, iUncompressedSize,
				cardIdEndSignature, 2
			);
		}
		if (!cardIdEndOffset) {
			cardIdEndOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,
				cardIdEndSignature, 2
			);
		}
		if (cardIdEndOffset) {
			cardIdEndOffset[0] = 0; // Prevent being searched again
			dbg_printf("Card ID end found: ");
		} else {
			dbg_printf("Card ID end not found\n");
		}
	}

	if (cardIdEndOffset) {
		dbg_hexa((u32)cardIdEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdEndOffset;
}

u16* findCardIdEndOffsetThumb(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const u16* cardReadEndOffset) {
	if (!cardReadEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdEndOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardIdEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, iUncompressedSize,
		cardIdEndSignatureThumb, 6
	);
	if (cardIdEndOffset) {
		dbg_printf("Card ID end thumb found: ");
	} else {
		dbg_printf("Card ID end thumb not found\n");
	}

	if (!cardIdEndOffset && moduleParams->sdk_version < 0x5000000) {
		// SDK <= 4
		cardIdEndOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardIdEndSignatureThumbAlt, 6
		);
		if (cardIdEndOffset) {
			dbg_printf("Card ID end thumb alt found: ");
		} else {
			dbg_printf("Card ID end thumb alt not found\n");
		}
	}

	if (!cardIdEndOffset && isSdk5(moduleParams)) {
		// SDK 5
		cardIdEndOffset = findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			cardIdEndSignatureThumb5, 8
		);
		if (cardIdEndOffset) {
			dbg_printf("Card ID end SDK 5 thumb found: ");
		} else {
			dbg_printf("Card ID end SDK 5 thumb not found\n");
		}
	}

	if (cardIdEndOffset) {
		dbg_hexa((u32)cardIdEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIdEndOffset;
}

u32* findCardIdStartOffset(const module_params_t* moduleParams, const u32* cardIdEndOffset) {
	if (!cardIdEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdStartOffset:\n");

	u32* cardIdStartOffset = NULL;

	if (isSdk5(moduleParams)) {
		// SDK 5
		cardIdStartOffset = findOffsetBackwards2(
			(u32*)cardIdEndOffset, 0x200,
			cardIdStartSignature5, cardIdStartSignature5Alt, 2
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start SDK 5 found\n");
		} else {
			dbg_printf("Card ID start SDK 5 not found\n");
		}

		if (!cardIdStartOffset) {
			cardIdStartOffset = findOffsetBackwards2(
				(u32*)cardIdEndOffset, 0x100,
				cardIdStartSignatureAlt1, cardIdStartSignatureAlt2, 1
			);
			if (cardIdStartOffset) {
				dbg_printf("Card ID start alt found\n");
			} else {
				dbg_printf("Card ID start alt not found\n");
			}
		}
	} else {
		cardIdStartOffset = findOffsetBackwards3(
			(u32*)cardIdEndOffset, 0x100,
			cardIdStartSignature, cardIdStartSignatureAlt1, cardIdStartSignatureAlt2, 1
		);
		if (cardIdStartOffset) {
			dbg_printf("Card ID start found\n");
		} else {
			dbg_printf("Card ID start not found\n");
		}
	}

	dbg_printf("\n");
	return cardIdStartOffset;
}

u16* findCardIdStartOffsetThumb(const module_params_t* moduleParams, const u16* cardIdEndOffset) {
	if (!cardIdEndOffset) {
		return NULL;
	}

	dbg_printf("findCardIdStartOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardIdStartOffset = findOffsetBackwardsThumb4(
		(u16*)cardIdEndOffset, 0x50,
		cardIdStartSignatureThumb, cardIdStartSignatureThumbAlt1, cardIdStartSignatureThumbAlt2, cardIdStartSignatureThumbAlt3, 2
	);
	if (cardIdStartOffset) {
		dbg_printf("Card ID start thumb found\n");
	} else {
		dbg_printf("Card ID start thumb not found\n");
	}

	dbg_printf("\n");
	return cardIdStartOffset;
}

u32* findCardReadDmaEndOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findCardReadDmaEndOffset:\n");

	u32* cardReadDmaEndOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		cardReadDmaEndSignature, 2
	);
	if (cardReadDmaEndOffset) {
		dbg_printf("Card read DMA end found: ");
	} else {
		dbg_printf("Card read DMA end not found\n");
	}

	if (!cardReadDmaEndOffset && moduleParams->sdk_version < 0x2008000) {
		cardReadDmaEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardReadDmaEndSignatureSdk2Alt, 2
		);
		if (cardReadDmaEndOffset) {
			dbg_printf("Card read DMA end SDK 2 alt found: ");
		} else {
			dbg_printf("Card read DMA end SDK 2 alt not found\n");
		}
	}

	if (!cardReadDmaEndOffset) {
		cardReadDmaEndOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			cardReadDmaEndSignatureDebug, 3
		);
		if (cardReadDmaEndOffset) {
			dbg_printf("Card read DMA end DEBUG found: ");
		} else {
			dbg_printf("Card read DMA end DEBUG not found\n");
		}
	}

	if (cardReadDmaEndOffset) {
		dbg_hexa((u32)cardReadDmaEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaEndOffset;
}

u16* findCardReadDmaEndOffsetThumb(const tNDSHeader* ndsHeader) {
	dbg_printf("findCardReadDmaEndOffsetThumb:\n");

	//if (usesThumb) {

	u16* cardReadDmaEndOffset = findOffsetThumb(
		(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		cardReadDmaEndSignatureThumbAlt, 4
	);
	if (cardReadDmaEndOffset) {
		dbg_printf("Card read DMA end thumb alt found: ");
	} else {
		dbg_printf("Card read DMA end thumb alt not found\n");
	}

	if (cardReadDmaEndOffset) {
		dbg_hexa((u32)cardReadDmaEndOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaEndOffset;
}

u32* findCardReadDmaStartOffset(const module_params_t* moduleParams, const u32* cardReadDmaEndOffset) {
	if (!cardReadDmaEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadDmaStartOffset:\n");

	//if (!usesThumb) {

	u32* cardReadDmaStartOffset = NULL;

	if (moduleParams->sdk_version > 0x5000000) {
		cardReadDmaStartOffset = findOffsetBackwards(
			(u32*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignature5, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start SDK 5 found: ");
			if (cardReadDmaStartOffset[-1] == 0xE92D000F) cardReadDmaStartOffset--;
		} else {
			dbg_printf("Card read DMA start SDK 5 not found\n");
		}
	} else {
		cardReadDmaStartOffset = findOffsetBackwards(
			(u32*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignature, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start found: ");
		} else {
			dbg_printf("Card read DMA start not found\n");
		}

		if (!cardReadDmaStartOffset && moduleParams->sdk_version < 0x2008000) {
			cardReadDmaStartOffset = findOffsetBackwards(
				(u32*)cardReadDmaEndOffset, 0x200,
				cardReadDmaStartSignatureSdk2Alt, 1
			);
			if (cardReadDmaStartOffset) {
				dbg_printf("Card read DMA start SDK 2 alt found: ");
			} else {
				dbg_printf("Card read DMA start SDK 2 alt not found\n");
			}
		}

		if (!cardReadDmaStartOffset) {
			cardReadDmaStartOffset = findOffsetBackwards(
				(u32*)cardReadDmaEndOffset, 0x200,
				cardReadDmaStartSignatureAlt1, 1
			);
			if (cardReadDmaStartOffset) {
				dbg_printf("Card read DMA start alt 1 found: ");
			} else {
				dbg_printf("Card read DMA start alt 1 not found\n");
			}
		}
		if (!cardReadDmaStartOffset) {
			cardReadDmaStartOffset = findOffsetBackwards(
				(u32*)cardReadDmaEndOffset, 0x200,
				cardReadDmaStartSignatureAlt2, 1
			);
			if (cardReadDmaStartOffset) {
				dbg_printf("Card read DMA start alt 2 found: ");
			} else {
				dbg_printf("Card read DMA start alt 2 not found\n");
			}
		}
	}

	if (cardReadDmaStartOffset) {
		dbg_hexa((u32)cardReadDmaStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaStartOffset;
}

u16* findCardReadDmaStartOffsetThumb(const u16* cardReadDmaEndOffset) {
	if (!cardReadDmaEndOffset) {
		return NULL;
	}

	dbg_printf("findCardReadDmaStartOffsetThumb:\n");

	//if (usesThumb) {
	
	u16* cardReadDmaStartOffset = findOffsetBackwardsThumb(
		(u16*)cardReadDmaEndOffset, 0x200,
		cardReadDmaStartSignatureThumb1, 1
	);
	if (cardReadDmaStartOffset) {
		dbg_printf("Card read DMA start thumb SDK < 3 found: ");
	} else {
		dbg_printf("Card read DMA start thumb SDK < 3 not found\n");
	}

	if (!cardReadDmaStartOffset) {
		cardReadDmaStartOffset = findOffsetBackwardsThumb(
			(u16*)cardReadDmaEndOffset, 0x200,
			cardReadDmaStartSignatureThumb3, 1
		);
		if (cardReadDmaStartOffset) {
			dbg_printf("Card read DMA start thumb SDK >= 3 found: ");
		} else {
			dbg_printf("Card read DMA start thumb SDK >= 3 not found\n");
		}
	}

	if (cardReadDmaStartOffset) {
		dbg_hexa((u32)cardReadDmaStartOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardReadDmaStartOffset;
}

u32* findInitLockEndOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findInitLockEndOffset:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		initLockEndSignature, 2
	);
	if (offset) {
		dbg_printf("Init lock end found\n");
	} else {
		dbg_printf("Init lock end not found\n");
	}

	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initLockEndSignatureThumb, 3
		);

		if (offset) {
			dbg_printf("Init lock end THUMB found\n");
		} else {
			dbg_printf("Init lock end THUMB not found\n");
		}
	}

	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initLockEndSignatureDebug, 3
		);

		if (offset) {
			dbg_printf("Init lock end Debug found\n");
		} else {
			dbg_printf("Init lock end Debug not found\n");
		}
	}

	dbg_printf("\n");
	return offset;
}

u32* a9FindCardIrqEnableOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool* usesThumb) {
	dbg_printf("findCardIrqEnableOffset:\n");
	
	const u32* irqEnableStartSignature = irqEnableStartSignature1;
	if (moduleParams->sdk_version > 0x4008000) {
		irqEnableStartSignature = irqEnableStartSignature4;
	}

	u32* cardIrqEnableOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
		irqEnableStartSignature, 4
	);
	if (cardIrqEnableOffset) {
		dbg_printf("irq enable found: ");
	} else {
		dbg_printf("irq enable not found\n");
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version < 0x2008000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature2Alt, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 2 alt found: ");
		} else {
			dbg_printf("irq enable SDK 2 alt not found\n");
		}
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4008000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature4, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 4 found: ");
		} else {
			dbg_printf("irq enable SDK 4 not found\n");
		}
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version > 0x4000000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignature4Debug, 4
		);
		if (cardIrqEnableOffset) {
			dbg_printf("irq enable SDK 4 debugger found: ");
		} else {
			dbg_printf("irq enable SDK 4 debugger not found\n");
		}
	}

	if (!cardIrqEnableOffset && moduleParams->sdk_version < 0x4008000) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignatureThumb, 5
		);
		if (cardIrqEnableOffset) {
			*usesThumb = true;
			dbg_printf("irq enable thumb found: ");
		} else {
			dbg_printf("irq enable thumb not found\n");
		}
	}

	if (!cardIrqEnableOffset) {
		cardIrqEnableOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//, ndsHeader->arm9binarySize,
            irqEnableStartSignatureThumbAlt, 4
		);
		if (cardIrqEnableOffset) {
			*usesThumb = true;
			dbg_printf("irq enable thumb alt found: ");
		} else {
			dbg_printf("irq enable thumb alt not found\n");
		}
	}

	if (cardIrqEnableOffset) {
		dbg_hexa((u32)cardIrqEnableOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return cardIrqEnableOffset;
}

const u32* getMpuInitRegionSignature(u32 patchMpuRegion) {
	switch (patchMpuRegion) {
		case 0: return mpuInitRegion0Signature;
		case 1: return mpuInitRegion1Signature;
		case 2: return mpuInitRegion2Signature;
		case 3: return mpuInitRegion3Signature;
	}
	return mpuInitRegion1Signature;
}

u32* findMpuStartOffset(const tNDSHeader* ndsHeader, u32 patchMpuRegion) {
	dbg_printf("findMpuStartOffset:\n");

	const u32* mpuInitRegionSignature = getMpuInitRegionSignature(patchMpuRegion);

	u32* mpuStartOffset = NULL;
	if (((u32)ndsHeader->arm9executeAddress - (u32)ndsHeader->arm9destination) >= 0x1000) {
		mpuStartOffset = findOffset(
			(u32*)ndsHeader->arm9executeAddress, 0x400,
			mpuInitRegionSignature, 1
		);
	}
	if (!mpuStartOffset) {
		mpuStartOffset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			mpuInitRegionSignature, 1
		);
	}
	if (mpuStartOffset) {
		dbg_printf("Mpu init found\n");
	} else {
		dbg_printf("Mpu init not found\n");
	}

	dbg_printf("\n");
	return mpuStartOffset;
}

u32* findMpuDataOffset(const module_params_t* moduleParams, u32 patchMpuRegion, const u32* mpuStartOffset) {
	if (!mpuStartOffset) {
		return NULL;
	}

	dbg_printf("findMpuDataOffset:\n");

	const u32* mpuInitRegion1Data = mpuInitRegion1Data1;
	const u32* mpuInitRegion2Data = mpuInitRegion2Data1;
	if (moduleParams->sdk_version > 0x4000000) {
		mpuInitRegion2Data = mpuInitRegion2Data3;
	}
	if (moduleParams->sdk_version > 0x5000000) {
		mpuInitRegion1Data = mpuInitRegion1Data5;
		mpuInitRegion2Data = mpuInitRegion2Data5;
	}

	const u32* mpuInitRegionData = mpuInitRegion1Data;
	switch (patchMpuRegion) {
		case 0:
			mpuInitRegionData = mpuInitRegion0Data;
			break;
		case 2:
			mpuInitRegionData = mpuInitRegion2Data;
			break;
		case 3:
			mpuInitRegionData = mpuInitRegion3Data;
			break;
	}
	
	u32* mpuDataOffset = findOffset(
		mpuStartOffset, 0x100,
		mpuInitRegionData, 1
	);
	if (!mpuDataOffset && patchMpuRegion == 1 && moduleParams->sdk_version < 0x4000000) {
		mpuDataOffset = findOffset(
			mpuStartOffset, 0x100,
			mpuInitRegion1DataAlt, 1
		);
	}
	if (!mpuDataOffset && patchMpuRegion == 2 && moduleParams->sdk_version < 0x4000000) {
		mpuDataOffset = findOffset(
			mpuStartOffset, 0x100,
			mpuInitRegion2Data3, 1
		);
	}
	if (!mpuDataOffset) {
		// Try to find it
		for (int i = 0; i < 0x100; i++) {
			mpuDataOffset += i;
			if ((*mpuDataOffset & 0xFFFFFF00) == 0x02000000) {
				break;
			}
		}
	}
	if (mpuDataOffset) {
		dbg_printf("Mpu data found\n");
	} else {
		dbg_printf("Mpu data not found\n");
	}

	dbg_printf("\n");
	return mpuDataOffset;
}

u32* findMpuDataOffsetAlt(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuDataOffsetAlt:\n");

	u32* mpuDataOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuInitRegion1DataAlt, 1
	);
	if (mpuDataOffset) {
		dbg_printf("Mpu data found\n");
	} else {
		dbg_printf("Mpu data not found\n");
	}

	dbg_printf("\n");
	return mpuDataOffset;
}

u32* findMpuFlagsSetOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuFlagsSet:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuFlagsSetSignature, 4
	);
	if (offset) {
		dbg_printf("Mpu flags set found\n");
	} else {
		dbg_printf("Mpu flags set not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findMpuCodeCacheChangeOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuCodeCacheChangeOffset:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuCodeCacheChangeSignature, 4
	);
	if (offset) {
		dbg_printf("Mpu code cache change found\n");
	} else {
		dbg_printf("Mpu code cache change not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findMpuChange(const tNDSHeader* ndsHeader) {
	dbg_printf("findMpuChange:\n");

	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		mpuChangeRegion1Signature, 3
	);
	if (offset) {
		dbg_printf("Mpu change found\n");
	} else {
		dbg_printf("Mpu change not found\n");
	}

	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			mpuChangeRegion1SignatureAlt, 3
		);
		if (offset) {
			dbg_printf("Mpu change alt found\n");
		} else {
			dbg_printf("Mpu change alt not found\n");
		}
	}

	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			mpuChangeRegion1SignatureThumb, 3
		);
		if (offset) {
			dbg_printf("Mpu change thumb found\n");
		} else {
			dbg_printf("Mpu change thumb not found\n");
		}
	}

	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			mpuChangeRegion1SignatureThumbAlt, 3
		);
		if (offset) {
			dbg_printf("Mpu change thumb alt found\n");
		} else {
			dbg_printf("Mpu change thumb alt not found\n");
		}
	}

	dbg_printf("\n");
	return offset;
}

u32* findHeapPointerOffset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	dbg_printf("findHeapPointerOffset:\n");
    
	const u32* initHeapEndSignature = initHeapEndSignature1;
	if (moduleParams->sdk_version > 0x5000000) {
		initHeapEndSignature = initHeapEndSignature5;
	}

    u32* initHeapEnd = findOffset(
        (u32*)ndsHeader->arm9destination, iUncompressedSize,
		initHeapEndSignature, 2
	);
    if (initHeapEnd) {
		dbg_printf("Init Heap End found: ");
	} else {
		dbg_printf("Init Heap End not found\n\n");
        return 0;
	}
    
    dbg_hexa((u32)initHeapEnd);
	dbg_printf("\n");
    dbg_printf("heapPointer: ");

	u32* initEndFunc = findOffsetBackwards(
		(u32*)initHeapEnd, 0x40,
		initHeapEndFuncSignature, 1
	);
	if (!initEndFunc) {
		initEndFunc = findOffsetBackwards(
			(u32*)initHeapEnd, 0x40,
			initHeapEndFuncSignatureAlt, 1
		);
	}
    u32* heapPointer = initEndFunc + 1;
    
	if (!initEndFunc) {
		u16* initEndFuncThumb = findOffsetBackwardsThumb(
			(u16*)initHeapEnd, 0x40,
			initHeapEndFuncSignatureThumb, 1
		);
		if (!initEndFuncThumb) {
			initEndFuncThumb = findOffsetBackwardsThumb(
				(u16*)initHeapEnd, 0x40,
				initHeapEndFuncSignatureThumbAlt, 1
			);
		}
        heapPointer = (u32*)((u16*)initEndFuncThumb+1);
	}
    
    dbg_hexa((u32)heapPointer);
	dbg_printf("\n");

	return heapPointer;
}

u32* findHeapPointer2Offset(const module_params_t* moduleParams, const tNDSHeader* ndsHeader) {
	dbg_printf("findHeapPointer2Offset:\n");
    
	u32* initEndFunc = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,
		initHeapEndFunc2Signature, 2
	);
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureAlt1, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureAlt2, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureAlt3, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumb, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumbAlt1, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumbAlt2, 2
		);
	}
	if (!initEndFunc) {
		initEndFunc = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			initHeapEndFunc2SignatureThumbAlt3, 2
		);
	}
    u32* heapPointer = initEndFunc + 1;
    
    dbg_hexa((u32)heapPointer);
	dbg_printf("\n");

	return heapPointer;
}

u32* findRandomPatchOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findRandomPatchOffset:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		randomPatchSignature, 4
	);
	if (randomPatchOffset) {
		dbg_printf("Random patch found: ");
	} else {
		dbg_printf("Random patch not found\n");
	}

	if (randomPatchOffset) {
		dbg_hexa((u32)randomPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return randomPatchOffset;
}

// SDK 5
u32* findRandomPatchOffset5Second(const tNDSHeader* ndsHeader) {
	dbg_printf("findRandomPatchOffset5Second:\n");

	u32* randomPatchOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
        randomPatchSignature5Second, 3
	);
	if (randomPatchOffset) {
		dbg_printf("Random patch SDK 5 second found: ");
	} else {
		dbg_printf("Random patch SDK 5 second not found\n");
	}

	if (randomPatchOffset) {
		dbg_hexa((u32)randomPatchOffset);
		dbg_printf("\n");
	}

	dbg_printf("\n");
	return randomPatchOffset;
}

u32* findFileIoOpenOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findFileIoOpenOffset:\n");

	u32* offset = NULL;
	if (moduleParams->sdk_version > 0x5010000) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			fileIoOpenSignature, 4
		);
	}
	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,
			fileIoOpenSignature50, 4
		);
	}

	if (!offset) {
		dbg_printf("FileIO open (ARM) not found. Trying thumb\n");
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,
			fileIoOpenSignatureThumb, 4
		);
	}

	if (offset) {
		dbg_printf("FileIO open found\n");
	} else {
		dbg_printf("FileIO open not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findFileIoCloseOffset(const u32* fileIoOpenOffset) {
	dbg_printf("findFileIoCloseOffset:\n");

	u32* offset = findOffset(
		fileIoOpenOffset + 8, 0xA0,
		fileIoCloseSignature, 4
	);

	if (offset) {
		dbg_printf("FileIO close found\n");
	} else {
		dbg_printf("FileIO close not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u16* findFileIoCloseOffsetThumb(const u16* fileIoOpenOffset) {
	dbg_printf("findFileIoCloseOffsetThumb:\n");

	u16* offset = findOffsetThumb(
		fileIoOpenOffset + 8, 0x60,
		fileIoCloseSignatureThumb, 4
	);

	if (offset) {
		dbg_printf("FileIO close THUMB found\n");
	} else {
		dbg_printf("FileIO close THUMB not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findFileIoSeekOffset(const u32* fileIoCloseOffset, const module_params_t* moduleParams) {
	dbg_printf("findFileIoSeekOffset:\n");

	u32* offset = NULL;
	if (moduleParams->sdk_version > 0x5030000) {
		offset = findOffset(
			fileIoCloseOffset + 8, 0x180,
			fileIoSeekSignature, 4
		);
	}
	if (!offset) {
		offset = findOffset(
			fileIoCloseOffset + 8, 0x180,
			fileIoSeekSignature50, 4
		);
	}

	if (offset) {
		dbg_printf("FileIO seek found\n");
	} else {
		dbg_printf("FileIO seek not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u16* findFileIoSeekOffsetThumb(const u16* fileIoCloseOffset) {
	dbg_printf("findFileIoSeekOffsetThumb:\n");

	u16* offset = findOffsetThumb(
		fileIoCloseOffset + 8, 0x100,
		fileIoSeekSignatureThumb, 4
	);

	if (offset) {
		dbg_printf("FileIO seek THUMB found\n");
	} else {
		dbg_printf("FileIO seek THUMB not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findFileIoReadOffset(const u32* fileIoSeekOffset, const module_params_t* moduleParams) {
	dbg_printf("findFileIoReadOffset:\n");

	u32* offset = NULL;
	if (moduleParams->sdk_version > 0x5030000) {
		offset = findOffset(
			fileIoSeekOffset, 0x80,
			fileIoReadSignature, 4
		);
	} else if (moduleParams->sdk_version > 0x5010000) {
		offset = findOffset(
			fileIoSeekOffset, 0x80,
			fileIoReadSignature, 4
		);

		if (!offset) {
			offset = findOffset(
				fileIoSeekOffset, 0x80,
				fileIoReadSignature51, 4
			);
		}
	} else {
		offset = findOffset(
			fileIoSeekOffset, 0x180,
			fileIoReadSignature50, 4
		);

		if (!offset) {
			offset = findOffset(
				fileIoSeekOffset, 0x80,
				fileIoReadSignature51, 4
			);
		}
	}

	if (offset) {
		dbg_printf("FileIO read found\n");
	} else {
		dbg_printf("FileIO read not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u16* findFileIoReadOffsetThumb(const u16* fileIoSeekOffset, const module_params_t* moduleParams) {
	dbg_printf("findFileIoReadOffsetThumb:\n");

	u16* offset = NULL;
	if (moduleParams->sdk_version > 0x5030000) {
		offset = findOffsetThumb(
			fileIoSeekOffset, 0x40,
			fileIoReadSignatureThumb, 4
		);
	} else {
		offset = findOffsetThumb(
			fileIoSeekOffset, 0x40,
			fileIoReadSignature51Thumb, 4
		);
	}

	if (offset) {
		dbg_printf("FileIO read THUMB found\n");
	} else {
		dbg_printf("FileIO read THUMB not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findCardEndReadDmaSdk5(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	dbg_printf("findCardEndReadDmaSdk5\n");

    const u16* cardEndReadDmaSignatureThumb = cardEndReadDmaSignatureThumb5;
    const u32* cardEndReadDmaSignature = cardEndReadDmaSignature5;

    u32 * offset = NULL;
    
    if(usesThumb) {
  		offset = (u32*)findOffsetThumb(
      		(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
            cardEndReadDmaSignatureThumb, 4
        );
    } else {
  		offset = findOffset(
      		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
            cardEndReadDmaSignature, 4
        ); 
    } 
    
    if (offset) {
		dbg_printf("cardEndReadDma found\n");
	} else {
		dbg_printf("cardEndReadDma not found\n");
	}
    
    dbg_printf("\n");
	return offset;
}

u32* findCardEndReadDma(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb, const u32* cardReadDmaEndOffset, u32* dmaHandlerOffset) {
	dbg_printf("findCardEndReadDma\n");

    if (moduleParams->sdk_version > 0x5000000) {
        return findCardEndReadDmaSdk5(ndsHeader,moduleParams,usesThumb);
    }

	const u32* offsetDmaHandler = NULL;
	if (moduleParams->sdk_version < 0x4000000) {
		offsetDmaHandler = cardReadDmaEndOffset+8;
	}

    if(moduleParams->sdk_version > 0x4000000 || *offsetDmaHandler<0x2000000 || *offsetDmaHandler>0x2400000) {
        offsetDmaHandler = cardReadDmaEndOffset+4; 
    }

    if(*offsetDmaHandler<0x2000000 || *offsetDmaHandler>0x2400000) {
        offsetDmaHandler = cardReadDmaEndOffset+3; 
    }

    if(*offsetDmaHandler<0x2000000 || *offsetDmaHandler>0x2400000) {
        dbg_printf("offsetDmaHandler not found\n");
        return 0;
    }

    dbg_printf("\noffsetDmaHandler found\n");
 	dbg_hexa((u32)offsetDmaHandler);
	dbg_printf(" : ");
    dbg_hexa(*offsetDmaHandler);
    dbg_printf("\n");

    u32 * offset = NULL;

    if (usesThumb) {
  		offset = (u32*)findOffsetThumb(
      		(u16*)((*offsetDmaHandler)-1), 0x200,//ndsHeader->arm9binarySize,
            cardEndReadDmaSignatureThumb4, 2
        );
    } else {
  		offset = findOffset(
      		(u32*)*offsetDmaHandler, 0x200,//ndsHeader->arm9binarySize,
            cardEndReadDmaSignature4, 1
        );
		if (!offset) {
			offset = findOffset(
				(u32*)*offsetDmaHandler, 0x200,//ndsHeader->arm9binarySize,
				cardEndReadDmaSignature4Alt, 1
			);
		}
    } 

	if (!offset) {
		if (usesThumb) {
			offset = (u32*)findOffsetThumb(
				(u16*)((*offsetDmaHandler)-1), 0x200,//ndsHeader->arm9binarySize,
				cardEndReadDmaSignatureThumb3, 1
			);
		} else {
			// Relocation alternative
			offset = findOffset(
				offsetDmaHandler+1, 0x200,//ndsHeader->arm9binarySize,
				cardEndReadDmaSignature4, 1
			);
			if (offset) {
				*dmaHandlerOffset = *offsetDmaHandler;
				dbg_printf("Offset before relocation: ");
				dbg_hexa((u32)offsetDmaHandler+4);
				dbg_printf("\n");
			}
		}
	}

    if (offset) {
		dbg_printf("cardEndReadDma found\n");
	} else {
		dbg_printf("cardEndReadDma not found\n");
	}

    dbg_printf("\n");
	return offset;
}

u32* findCardSetDmaSdk5(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	dbg_printf("findCardSetDmaSdk5\n");
    
    u32* currentOffset = (u32*)ndsHeader->arm9destination;
    u32* startOffset = NULL;
	while (startOffset==NULL) {
        u32* cardSetDmaEndOffset = findOffset(
      		currentOffset+1, iUncompressedSize,
            cardSetDmaSignatureValue1, 1
        );
        if (cardSetDmaEndOffset==NULL) {          
		    dbg_printf("cardSetDmaEnd not found\n");
            return NULL;
        } else {
            dbg_printf("cardSetDmaSignatureValue1 found\n");
         	dbg_hexa((u32)cardSetDmaEndOffset);
        	dbg_printf(" : ");
            dbg_hexa(*cardSetDmaEndOffset);
            dbg_printf("\n");
        
            currentOffset = cardSetDmaEndOffset+2;
             if(usesThumb) {
                  dbg_printf("cardSetDmaSignatureStartThumb used: ");
            		startOffset = (u32*)findOffsetBackwardsThumb(
                		(u16*)cardSetDmaEndOffset, 0x90,
                      cardSetDmaSignatureStartThumb5, 2
                  );
              } else {
                  dbg_printf("cardSetDmaSignatureStart used: ");
            		startOffset = findOffsetBackwards(
                		cardSetDmaEndOffset, 0x90,
                      cardSetDmaSignatureStart5, 2
                  );
              } 
            if (!startOffset && !usesThumb) {
            	startOffset = findOffsetBackwards(
            		cardSetDmaEndOffset, 0x90,
                  cardSetDmaSignatureStart5Alt, 2
              );
			}
            if (startOffset!=NULL) {
                dbg_printf("cardSetDmaSignatureStart found\n");
             	/*dbg_hexa((u32)startOffset);
            	dbg_printf(" : ");
                dbg_hexa(*startOffset);   
                dbg_printf("\n");*/
                
                return startOffset;
            }
        }
    }

    return NULL;
}

u32* findCardSetDma(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, bool usesThumb) {
	dbg_printf("findCardSetDma\n");

    if (moduleParams->sdk_version > 0x5000000) {
        return findCardSetDmaSdk5(ndsHeader,moduleParams,usesThumb);
    }

    //const u16* cardSetDmaSignatureStartThumb = cardSetDmaSignatureStartThumb4;
    const u32* cardSetDmaSignatureStart = cardSetDmaSignatureStart4;
	int cardSetDmaSignatureStartLen = 3;

    if (moduleParams->sdk_version < 0x2004000) {
		cardSetDmaSignatureStart = cardSetDmaSignatureStart2Early;
		cardSetDmaSignatureStartLen = 4;
    } else if (moduleParams->sdk_version < 0x4000000) {
		cardSetDmaSignatureStart = cardSetDmaSignatureStart2;
    }

  	u32* cardSetDmaEndOffset = NULL;
    u32* currentOffset = (u32*)ndsHeader->arm9destination;
	while (cardSetDmaEndOffset==NULL) {
        cardSetDmaEndOffset = findOffset(
      		currentOffset+1, iUncompressedSize,
            cardSetDmaSignatureValue1, 1
        );
        if (cardSetDmaEndOffset==NULL) {          
		    dbg_printf("cardSetDmaEnd not found\n");
            return NULL;
        } else {
            dbg_printf("cardSetDmaSignatureValue1 found\n");
         	dbg_hexa((u32)cardSetDmaEndOffset);
        	dbg_printf(" : ");
            dbg_hexa(*cardSetDmaEndOffset);
            dbg_printf("\n");
        
            currentOffset = cardSetDmaEndOffset+2;
            cardSetDmaEndOffset = findOffset(
          		currentOffset, 0x18,
                cardSetDmaSignatureValue2, 1
            );
            if (cardSetDmaEndOffset!=NULL) {
                dbg_printf("cardSetDmaSignatureValue2 found\n");
             	dbg_hexa((u32)cardSetDmaEndOffset);
            	dbg_printf(" : ");
                dbg_hexa(*cardSetDmaEndOffset);
                dbg_printf("\n");
                
                break;
            }             
        }     
    } 

    dbg_printf("cardSetDmaEnd found\n");
 	dbg_hexa((u32)cardSetDmaEndOffset);
	dbg_printf(" : ");
    dbg_hexa(*cardSetDmaEndOffset);
    dbg_printf("\n");

    u32 * offset = NULL;

    if(usesThumb) {
        dbg_printf("cardSetDmaSignatureStartThumb used: ");
  		offset = (u32*)findOffsetBackwardsThumb(
      		(u16*)cardSetDmaEndOffset, 0x60,
            cardSetDmaSignatureStartThumb4, 4
        );
    } else {
        dbg_printf("cardSetDmaSignatureStart used: ");
  		offset = findOffsetBackwards(
      		cardSetDmaEndOffset, 0x80,
            cardSetDmaSignatureStart, cardSetDmaSignatureStartLen
        );
    }

	if (!offset && usesThumb) {
  		offset = (u32*)findOffsetBackwardsThumb(
      		(u16*)cardSetDmaEndOffset, 0x60,
            cardSetDmaSignatureStartThumb3, 4
        );
	}
	if (!offset && !usesThumb) {
		if (moduleParams->sdk_version > 0x3000000 && moduleParams->sdk_version < 0x4000000) {
			offset = findOffsetBackwards(
				cardSetDmaEndOffset, 0x60,
				cardSetDmaSignatureStart3, 3
			);
		} else if (moduleParams->sdk_version > 0x4000000) {
			offset = findOffsetBackwards(
				cardSetDmaEndOffset, 0x60,
				cardSetDmaSignatureStart4Alt, 3
			);
		}
	}

    if (offset) {
		dbg_printf("cardSetDma found\n");
	} else {
		dbg_printf("cardSetDma not found\n");
	}

    dbg_printf("\n");
	return offset;
}

u32* findSrlStartOffset9(const tNDSHeader* ndsHeader) {
	dbg_printf("findSrlStartOffset9\n");

    u32* offset = findOffset(
    	(u32*)ndsHeader->arm9destination, iUncompressedSize,
		srlStartSignature3, 4
	);

	if (offset) {
		dbg_printf("SRL start function found\n");
	} else {
		dbg_printf("SRL start function not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findResetOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams, const bool softResetMb) {
	dbg_printf("findResetOffset\n");
    const u32* resetSignature = resetSignature2;

    if (moduleParams->sdk_version > 0x4008000 && moduleParams->sdk_version < 0x5000000) {
        resetSignature = resetSignature4;
    }
    if (moduleParams->sdk_version > 0x5000000) {
        resetSignature = resetSignature5;
    }

    u32 * resetOffset = NULL;

	if (softResetMb) {
    	u32* resetEndOffset = findOffset(
    		(u32*)ndsHeader->arm9destination, iUncompressedSize,
    		resetConstant, 1
    	);
		if (resetEndOffset) {
    		dbg_printf("Reset constant found: ");
            dbg_hexa((u32)resetEndOffset);
    		dbg_printf("\n");

			resetOffset = findOffsetBackwards(
				resetEndOffset, 0x80,
				resetSignature3Eoo, 2
			);

			if (resetOffset) {
				dbg_printf("Reset found\n");
				dbg_printf("\n");
				return resetOffset;
			} else {
				dbg_printf("Reset not found\n");
			}
		}
	}

  	resetOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		resetSignature, 4
	);
	
	if (!resetOffset) {
		if (moduleParams->sdk_version > 0x2000000 && moduleParams->sdk_version < 0x2008000) {
			resetOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature5Alt1, 4
			);
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature2Alt1, 4
				);
			}
		} else if (moduleParams->sdk_version < 0x4008000) {
			if (moduleParams->sdk_version > 0x2008000 && moduleParams->sdk_version < 0x3000000) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature2Alt2, 4
				);
			}
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature3, 4
				);
			}
			if (moduleParams->sdk_version > 0x3000000) {
				if (!resetOffset) {
					resetOffset = findOffset(
						(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
						resetSignature3Alt, 4
					);
				}
			}
		} else if (moduleParams->sdk_version > 0x4008000 && moduleParams->sdk_version < 0x5000000) {
			resetOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature4Alt, 4
			);
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature3Alt, 4
				);
			}
		} else if (moduleParams->sdk_version > 0x5000000) {
			resetOffset = findOffset(
				(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature5Alt1, 4
			);
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature5Alt2, 4
				);
			}
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature5Alt3, 4
				);
			}
			if (!resetOffset) {
				resetOffset = findOffset(
					(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
					resetSignature5Alt4, 4
				);
			}
		}
	}
    
    if (resetOffset) {
		dbg_printf("Reset found: ");
        dbg_hexa((u32)resetOffset);
		dbg_printf("\n");
    } 
    
    while(resetOffset!=NULL) {
    	u32* resetEndOffset = findOffset(
    		resetOffset, 0x200,
    		(isSdk5(moduleParams) ? resetConstant5 : resetConstant), 1
    	);
        if (resetEndOffset) {
    		dbg_printf("Reset constant found: ");
            dbg_hexa((u32)resetEndOffset);
    		dbg_printf("\n");
            break;
        } 
        
      	resetOffset = findOffset(
				resetOffset+1, iUncompressedSize,//ndsHeader->arm9binarySize,
				resetSignature, 4
			);
        if (resetOffset) {
		    dbg_printf("Reset found: ");
            dbg_hexa((u32)resetOffset);
    		dbg_printf("\n");
        } 
    } 
    
	if (resetOffset) {
		dbg_printf("Reset found\n");
	} else {
		dbg_printf("Reset not found\n");
	}

	dbg_printf("\n");
	return resetOffset;
}

u32* findNandTmpJumpFuncOffset(const tNDSHeader* ndsHeader, const module_params_t* moduleParams) {
	dbg_printf("findNandTmpJumpFuncOffset\n");
	
	u32* endOffset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		nandTmpJumpFuncConstant, 1
	);

	u32* offset = NULL;
	if (moduleParams->sdk_version < 0x5008000) {
		offset = findOffsetBackwards(
			endOffset, 0x400,
			nandTmpJumpFuncStart30, 1
		);
	} else {
		offset = findOffsetBackwards(
			endOffset, 0x60,
			nandTmpJumpFuncStart3, 1
		);

		if (!offset) {
			offset = findOffsetBackwards(
				endOffset, 0x60,
				nandTmpJumpFuncStart4, 1
			);
		}
	}

	if (offset) {
		dbg_printf("nandTmpJumpFunc found\n");
	} else {
		dbg_printf("nandTmpJumpFunc not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findTwlSleepModeEndOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findTwlSleepModeEndOffset\n");
	
	u32* offset = findOffset(
		(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
		twlSleepModeEndSignatureEarly, 4
	);

	if (!offset) {
		offset = findOffset(
			(u32*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			twlSleepModeEndSignature, 3
		);
	}

	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			twlSleepModeEndSignatureThumbEarly, 7
		);
	}

	if (!offset) {
		offset = (u32*)findOffsetThumb(
			(u16*)ndsHeader->arm9destination, iUncompressedSize,//ndsHeader->arm9binarySize,
			twlSleepModeEndSignatureThumb, 3
		);
	}

	if (offset) {
		dbg_printf("twlSleepModeEnd found\n");
	} else {
		dbg_printf("twlSleepModeEnd not found\n");
	}

	dbg_printf("\n");
	return offset;
}

u32* findSharedFontPathOffset(const tNDSHeader* ndsHeader) {
	dbg_printf("findSharedFontPathOffset\n");

	char* offset = NULL;

	char* arm9dst = (char*)ndsHeader->arm9destination;
	for (u32 i = 0; i < iUncompressedSize; i++) {
		if (strcmp(arm9dst+i, nandSharedFontSignature) == 0) {
			offset = arm9dst+i;
			break;
		}
	}

	if (offset) {
		dbg_printf("Shared font path offset found\n");
	} else {
		dbg_printf("Shared font path offset not found\n");
	}

	dbg_printf("\n");
	return (u32*)offset;
}
