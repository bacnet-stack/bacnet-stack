#include <string.h>

static void func()
{
    char a[128];
    memset(a, ' ', sizeof(a));
}

void foo()
{
    func();
}
