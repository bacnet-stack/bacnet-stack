#include <string.h>

static void func()
{
    char b[256];
    memset(b, ' ', sizeof(b));
}

void bar()
{
    func();
}
