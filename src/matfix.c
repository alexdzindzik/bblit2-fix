#include "matfix.h"

int mf_unwrap(const short C[9], const short W[9], const short O[9], float f[9]) {
    int wrapped = 0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            long long sum = 0;
            for (int k = 0; k < 3; k++) sum += (long long)C[i * 3 + k] * W[k * 3 + j];
            if (sum < -2147483647LL - 1 || sum > 2147483647LL) return 0;   // o jogo somaria em 32 bits com estouro
            int q = (int)sum >> 12;                                        // como o jogo: soma de 32 bits, sar 12
            if ((short)q != O[i * 3 + j]) return 0;
            if (q != (short)q) wrapped = 1;
            f[i * 3 + j] = (float)q / 4096.0f;
        }
    return wrapped;
}
