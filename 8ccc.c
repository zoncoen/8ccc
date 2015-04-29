#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define BUFLEN 256
#define EXPR_LEN 100
#define MAX_ARGS 6

enum {
    AST_INT,
    AST_CHAR,
    AST_VAR,
    AST_STR,
    AST_FUNCALL,
};

typedef struct Ast {
    char type;
    union {
        // Integer
        int ival;
        // Char
        char c;
        // String
        struct {
            char *sval;
            int sid;
            struct Ast *snext;
        };
        // Variable
        struct {
            char *vname;
            int vpos;
            struct Ast *vnext;
        };
        // Binary operator
        struct {
            struct Ast *left;
            struct Ast *right;
        };
        // Function call
        struct {
            char *fname;
            int nargs;
            struct Ast **args;
        };
    };
} Ast;

Ast *vars = NULL;
Ast *strings = NULL;
char *REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void error(char *fmt, ...) __attribute__((noreturn));
void emit_expr(Ast *ast);
Ast *read_expr2(int prec);
Ast *read_expr(void);

void error(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

Ast *make_ast_op(char type, Ast *left, Ast *right)
{
    Ast *r = malloc(sizeof(Ast));
    r->type = type;
    r->left = left;
    r->right = right;
    return r;
}

Ast *make_ast_int(int val)
{
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_INT;
    r->ival = val;
    return r;
}

Ast *make_ast_char(char c)
{
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_CHAR;
    r->c = c;
    return r;
}

Ast *make_ast_var(char *vname)
{
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_VAR;
    r->vname = vname;
    r->vpos = vars ? vars->vpos + 1 : 1;
    r->vnext = vars;
    vars = r;
    return r;
}

Ast *make_ast_str(char *str)
{
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_STR;
    r->sval = str;
    if (strings == NULL) {
        r->sid = 0;
        r->snext = NULL;
    } else {
        r->sid = strings->sid + 1;
        r->snext = strings;
    }
    strings = r;
    return r;
}

Ast *make_ast_funcall(char *fname, int nargs, Ast **args)
{
    Ast *r = malloc(sizeof(Ast));
    r->type = AST_FUNCALL;
    r->fname = fname;
    r->nargs = nargs;
    r->args = args;
    return r;
}

Ast *find_var(char *name)
{
    for (Ast *p = vars; p; p = p->vnext) {
        if (!strcmp(name, p->vname)) {
            return p;
        }
    }
    return NULL;
}

int priority(char op)
{
    switch (op) {
        case '=':
            return 1;
        case '+':
        case '-':
            return 2;
        case '*':
        case '/':
            return 3;
        default:
            return -1;
    }
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

Ast *read_number(int n)
{
    for (;;) {
        int c = getc(stdin);
        if (!isdigit(c)) {
            ungetc(c, stdin);
            return make_ast_int(n);
        }
        if (n < 0) {
            n = n * 10 - (c - '0');
        } else {
            n = n * 10 + (c - '0');
        }
    }
    return make_ast_int(n);
}

char *read_ident(char c)
{
    char *buf = malloc(BUFLEN);
    buf[0] = c;
    int i = 1;
    for (;;) {
        int c = getc(stdin);
        if (!isalnum(c)) {
            ungetc(c, stdin);
            break;
        }
        buf[i++] = c;
        if (i == BUFLEN - 1) {
            error("Identifier too long");
        }
    }
    buf[i] = '\0';
    return buf;
}

Ast *read_func_args(char *fname)
{
    Ast **args = malloc(sizeof(Ast *) * (MAX_ARGS + 1));
    int i = 0, nargs = 0;
    for (; i < MAX_ARGS; i++) {
        skip_space();
        char c = getc(stdin);
        if (c == ')') {
            break;
        }
        ungetc(c, stdin);
        args[i] = read_expr2(0);
        nargs++;
        c = getc(stdin);
        if (c == ')') {
            break;
        }
        if (c == ',') {
            skip_space();
        } else {
            error("Unexpected charanter: '%c'", c);
        }
    }
    if (i == MAX_ARGS) {
        error("Too many arguments: %s", fname);
    }
    return make_ast_funcall(fname, nargs, args);
}

Ast *read_ident_or_func(char c)
{
    char *name = read_ident(c);
    skip_space();
    char c2 = getc(stdin);
    if (c2 == '(') {
        return read_func_args(name);
    }
    ungetc(c2, stdin);
    Ast *v = find_var(name);
    return v ? v : make_ast_var(name);
}

Ast *read_char(void)
{
    char c = getc(stdin);
    if (c == EOF) {
        goto err;
    }
    if (c == '\\') {
        c = getc(stdin);
        if (c == EOF) {
            goto err;
        }
    }
    char c2 = getc(stdin);
    if (c2 == EOF) {
        goto err;
    }
    if (c2 != '\'') {
        error("Malformed char constant");
    }
    return make_ast_char(c);
err:
    error("Unterminated char");
}

Ast *read_string(void)
{
    char *buf = malloc(BUFLEN);
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
    return make_ast_str(buf);
}

Ast *read_prim(void)
{
    int c = getc(stdin);
    if (isdigit(c)) {
        return read_number(c - '0');
    } else if (c == '-') {
        skip_space();
        c = getc(stdin);
        if (isdigit(c)) {
            return read_number(-(c - '0'));
        }
    } else if (c == '\'') {
        return read_char();
    } else if (c == '"') {
        return read_string();
    } else if (isalpha(c)) {
        return read_ident_or_func(c);
    } else if (c == EOF) {
        return NULL;
    }
    error("Don't know how to handle '%c'", c);
}

Ast *read_expr2(int prec)
{
    skip_space();
    Ast *ast = read_prim();
    if (!ast) {
        return NULL;
    }
    for (;;) {
        skip_space();
        int c = getc(stdin);
        if (c == EOF) {
            return ast;
        }
        int prec2 = priority(c);
        if (prec2 < 0 || prec2 < prec) {
            ungetc(c, stdin);
            return ast;
        }
        skip_space();
        ast = make_ast_op(c, ast, read_expr2(prec2 + 1));
    }
    return ast;
}

Ast *read_expr(void)
{
    Ast *r = read_expr2(0);
    if (!r) {
        return NULL;
    }
    skip_space();
    int c = getc(stdin);
    if (c != ';') {
        error("Unterminated expression '%c'", c);
    }
    return r;
}

void print_quote(char *p)
{
    while (*p) {
        if (*p == '\"' || *p == '\\') {
            printf("\\");
        }
        printf("%c", *p);
        p++;
    }
}

void print_ast(Ast *ast)
{
    switch (ast->type) {
        case AST_INT:
            printf("%d", ast->ival);
            break;
        case AST_CHAR:
            printf("'%c'", ast->c);
            break;
        case AST_VAR:
            printf("%s", ast->vname);
            break;
        case AST_STR:
            printf("\"");
            print_quote(ast->sval);
            printf("\"");
            break;
        case AST_FUNCALL:
            printf("%s(", ast->fname);
            for (int i = 0; ast->args[i]; i++) {
                print_ast(ast->args[i]);
                if (ast->args[i + 1]) {
                    printf(",");
                }
            }
            printf(")");
            break;
        default:
            printf("(%c ", ast->type);
            print_ast(ast->left);
            printf(" ");
            print_ast(ast->right);
            printf(")");
    }
}

void emit_binop(Ast *ast)
{
    if (ast->type == '=') {
        emit_expr(ast->right);
        if (ast->left->type != AST_VAR) {
            error("Symbol expected");
        }
        printf("\tmov %%eax, -%d(%%rbp)\n", ast->left->vpos * 4);
        return;
    }
    char *op;
    switch (ast->type) {
        case '+':
            op = "add";
            break;
        case '-':
            op = "sub";
            break;
        case '*':
            op = "imul";
            break;
        case '/':
            break;
        default:
            error("invalid operator '%c'", ast->type);
    }
    emit_expr(ast->left);
    printf("\tpush %%rax\n");
    emit_expr(ast->right);
    if (ast->type == '/') {
        printf("\tmov %%eax, %%ebx\n");
        printf("\tpop %%rax\n");
        printf("\tmov $0, %%edx\n");
        printf("\tidiv %%rbx\n");
    } else {
        printf("\tpop %%rbx\n");
        printf("\t%s %%rbx, %%rax\n", op);
    }
}

void emit_expr(Ast *ast)
{
    switch (ast->type) {
        case AST_INT:
            printf("\tmov $%d, %%eax\n", ast->ival);
            break;
        case AST_CHAR:
            printf("\tmov $%d, %%eax\n", ast->c);
            break;
        case AST_VAR:
            printf("\tmov -%d(%%rbp), %%eax\n", ast->vpos * 4);
            break;
        case AST_STR:
            printf("\tlea .s%d(%%rip), %%rax\n", ast->sid);
            break;
        case AST_FUNCALL:
            for (int i = 1; i < ast->nargs; i++) {
                printf("\tpush %%%s\n", REGS[i]);
            }
            for (int i = 0; i < ast->nargs; i++) {
                emit_expr(ast->args[i]);
                printf("\tpush %%rax\n");
            }
            for (int i = ast->nargs - 1; i >= 0; i--) {
                printf("\tpop %%%s\n", REGS[i]);
            }
            printf("\tmov $0, %%eax\n");
            printf("\tcall _%s\n", ast->fname);
            for (int i = ast->nargs - 1; i > 0; i--) {
                printf("\tpop %%%s\n", REGS[i]);
            }
            break;
        default:
            emit_binop(ast);
    }
}

void emit_data_section(void)
{
    if (!strings) {
        return;
    }
    printf("\t.data\n");
    for (Ast *p = strings; p; p = p->snext) {
        printf(".s%d:\n", p->sid);
        printf("\t.string \"");
        print_quote(p->sval);
        printf("\"\n");
    }
}

int main(int argc, char **argv)
{
    int wantast = (argc > 1 && !strcmp(argv[1], "-a"));
    Ast *exprs[EXPR_LEN];
    int i;
    for (i = 0; i < EXPR_LEN; i++) {
        Ast *t = read_expr();
        if (!t) {
            break;
        }
        exprs[i] = t;
    }
    int nexpr = i;
    if (!wantast) {
        emit_data_section();
        printf(
            "\t.text\n"
            "\t.global _mymain\n"
            "_mymain:\n");
    }
    for (i = 0; i < nexpr; i++) {
        if (wantast) {
            print_ast(exprs[i]);
        } else {
            emit_expr(exprs[i]);
        }
    }
    if (!wantast) {
        printf("\tret\n");
    }
    return 0;
}
