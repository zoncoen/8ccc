#include <stdio.h>

extern int type;
extern char *mymain(void);

int main(int argc, char **argv)
{
    if (type == 1) {
        printf("%d\n", (int)(mymain()));
    } else {
        printf("%s\n", mymain());
    }
    return 0;
}
