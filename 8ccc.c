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

void skip_space(void)
{
    int c;
    while ((c = getc(stdin)) != EOF) {
        if (isspace(c)) {
            continue;
        }
        ungetc(c, stdin);
        return;
    }
}

int read_number(int n)
{
    int c;
    while ((c = getc(stdin)) != EOF) {
        if (!isdigit(c)) {
            ungetc(c, stdin);
            return n;
        }
        if (n < 0) {
            n = n * 10 - (c - '0');
        } else {
            n = n * 10 + (c - '0');
        }
    }
    return n;
}

int compile_expr2(void)
{
    for (;;) {
        skip_space();
        int c = getc(stdin);
        if (c == EOF) {
            printf("\tret\n");
            exit(0);
        }
        char *op;
        if (c == '+') {
            op = "add";
        } else if (c == '-') {
            op = "sub";
        } else {
            error("Operator expected, but got '%c'", c);
        }
        skip_space();
        c = getc(stdin);
        if (!isdigit(c)) {
            if (c == '-') {
                skip_space();
                c = getc(stdin);
                if (isdigit(c)) {
                    printf("\t%s $%d, %%rax\n", op, read_number(-(c - '0')));
                } else {
                    error("Number expected, but got '%c'", c);
                }
            } else {
                error("Number expected, but got '%c'", c);
            }
        } else {
            printf("\t%s $%d, %%rax\n", op, read_number(c - '0'));
        }
    }
}

void compile_expr(int n)
{
    n = read_number(n);
    printf(
        ".text\n"
        "\t.global _type\n"
        "\t.global _mymain\n"
        "_type:\n"
        "\t.long 1\n"
        "_mymain:\n"
        "\tmov $%d, %%rax\n",
        n);
    compile_expr2();
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
    exit(0);
}

void compile(void)
{
    int c = getc(stdin);
    if (isdigit(c)) {
        compile_expr(c - '0');
    } else if (c == '-') {
        c = getc(stdin);
        if (isdigit(c)) {
            compile_expr(-(c - '0'));
        } else {
            error("Number expected, but got '%c'", c);
        }
    } else if (c == '"') {
        compile_string();
    } else {
        error("Don't know how to handle '%c'", c);
    }
}

int main(int argc, char **argv)
{
    compile();
    return 0;
}
