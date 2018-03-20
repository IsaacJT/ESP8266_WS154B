#include "logging.h"
#include "ets_sys.h"
#include "osapi.h"

static inline int32_t asm_ccount(void) {
    int32_t r;
    asm volatile ("rsr %0, ccount" : "=r"(r));
    return r;
}

void print_log(char entry[]) {
    os_printf("%u:\t%s\n", asm_ccount(), entry);
}
