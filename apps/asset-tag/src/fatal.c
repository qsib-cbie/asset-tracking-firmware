#include <zephyr.h>
#include <fatal.h>

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf) {
    ARG_UNUSED(esf);

    // LOG_PANIC();
    // LOG_ERR("Resetting system");
    NVIC_SystemReset();
}
