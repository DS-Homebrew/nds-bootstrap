#ifndef CHEAT_ENGINE_H
#define CHEAT_ENGINE_H

#define CHEAT_DATA_MAX_SIZE (32 * 1024)	// 32KiB
#define CHEAT_DATA_MAX_LEN  (CHEAT_DATA_MAX_SIZE / sizeof(u32))

inline bool checkCheatDataLen(u32 cheat_data_len) {
    return (cheat_data_len + 2 <= CHEAT_DATA_MAX_LEN);
}

inline void endCheatData(u32* cheat_data, u32* cheat_data_len_ptr) {
    if (*cheat_data_len_ptr + 2 > CHEAT_DATA_MAX_LEN) { // Not necessarily needed
        return;
    }
    cheat_data[*cheat_data_len_ptr]     = 0xCF000000;
    cheat_data[*cheat_data_len_ptr + 1] = 0x00000000;
    *cheat_data_len_ptr += 2;
}

#endif // CHEAT_ENGINE_H