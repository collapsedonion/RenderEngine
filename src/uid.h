//
// Created by Роман  Тимофеев on 11.05.2026.
//

#ifndef RENDERENGINE_UID_H
#define RENDERENGINE_UID_H

#include <chrono>

typedef uint64_t UID;

inline UID gen_uid() {
    uint16_t rand_i = std::ceil((float)rand() / ((float)rand() * 12) * UINT16_MAX);
    uint64_t rand_j = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

    return rand_i * rand_j;
}

#endif //RENDERENGINE_UID_H
