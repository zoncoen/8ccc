#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define BUFLEN 256

void error(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

void compile_number(int n)
{
    int c;
    while ((c = getc(stdin)) != EOF) {
        if (isspace(c)) {
            break;
        }
        if (!isdigit(c)) {
            error("Invalid character in number: '%c'", c);
        }
        if (n < 0) {
            n = n * 10 - (c - '0');
        } else {
            n = n * 10 + (c - '0');
        }
    }
    printf(
        ".text\n"
        "\t.global _type\n"
        "\t.global _mymain\n"
        "_type:\n"
        "\t.long 1\n"
        "_mymain:\n"
        "\tmov $%d, %%rax\n"
        "\tret\n",
        n);
}

void compile_string(void)
{
    char buf[BUFLEN];
    int i = 0;
    for (;;) {
        int c = getc(stdin);
        if (c == EOF) {
            error("Unterminated string");
        }
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            c = getc(stdin);
            if (c == EOF) {
                error("Unterminated \\");
            }
        }
        buf[i++] = c;
        if (i == BUFLEN - 1) {
            error("String too long");
        }
    }
    buf[i] = '\0';
    printf(
        ".data\n"
        ".mydata:\n"
        "\t.string \"%s\"\n"
        "\t.text\n"
        "\t.global _type\n"
        "\t.global _mymain\n"
        "_type:\n"
        "\t.long 2\n"
        "_mymain:\n"
        "\tlea .mydata(%%rip), %%rax\n"
        "\tret\n",
        buf);
}

void compile(void)
{
    int c = getc(stdin);
    if (isdigit(c)) {
        return compile_number(c - '0');
    }
    if (c == '-') {
        c = getc(stdin);
        if (isdigit(c)) {
            return compile_number(-(c - '0'));
        }
    }
    if (c == '"') {
        return compile_string();
    }
    error("Don't know how to handle '%c'", c);
}

int main(int argc, char **argv)
{
    compile();
    return 0;
}
