#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>

void mg_random(void *buf, size_t len)
{
    //sys_csrand_get(buf, len);
    sys_rand_get(buf, len);
}

uint64_t mg_millis(void)
{
    return (uint64_t) k_uptime_get();
}
