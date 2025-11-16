// demonstrates concurrency

#include <u.h>
#include <libc.h>
#include <thread.h>

enum { STACK = 8192 }; // 8 KB

void
producer(void *v)
{
    Channel *ch = v;
    for (ulong i = 1; ; i++) {
        sleep(400);
        print("p %lud\n", i);
        sendul(ch, i);
    }
}

void
consumer(void *v)
{
    Channel *ch = v;
    for (;;) {
        ulong p = recvul(ch);
        print("\t\tc %lud\n", p);
        sleep(200 + nrand(600));
    }
}

void
threadmain(int argc, char **argv)
{
    int (*mk)(void (*fn)(void*), void *arg, uint stack);

    mk = threadcreate;

    Channel *ch = chancreate(sizeof(ulong), 1);

    mk(producer, ch, STACK);
    mk(consumer, ch, STACK);

    recvp(chancreate(sizeof(void*), 0));

    threadexitsall(nil);
}
