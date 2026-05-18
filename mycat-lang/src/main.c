#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOK_EOF = 0,
    TOK_NEWLINE,
    TOK_NUMBER,
    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_LET,
    TOK_PRINT,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_END,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_ASSIGN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE
} TokenType;

typedef struct {
    TokenType type;
    int line;
    long number;
    char *text;
} Token;

typedef struct {
    Token *items;
    size_t len;
    size_t cap;
} TokenArray;

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum {
    EXPR_NUMBER = 0,
    EXPR_VAR,
    EXPR_UNARY,
    EXPR_BINARY
} ExprType;

struct Expr {
    ExprType type;
    union {
        long number;
        char *name;
        struct {
            TokenType op;
            Expr *right;
        } unary;
        struct {
            TokenType op;
            Expr *left;
            Expr *right;
        } binary;
    } as;
};

typedef struct {
    Stmt **items;
    size_t len;
    size_t cap;
} StmtList;

typedef enum {
    STMT_LET = 0,
    STMT_PRINT,
    STMT_IF,
    STMT_WHILE
} StmtType;

struct Stmt {
    StmtType type;
    int line;
    union {
        struct {
            char *name;
            Expr *value;
        } let_stmt;
        struct {
            int is_string;
            char *text;
            Expr *value;
        } print_stmt;
        struct {
            Expr *condition;
            StmtList then_branch;
            StmtList else_branch;
            int has_else;
        } if_stmt;
        struct {
            Expr *condition;
            StmtList body;
        } while_stmt;
    } as;
};

typedef struct {
    Token *tokens;
    size_t len;
    size_t pos;
    int failed;
} Parser;

typedef struct Var {
    char *name;
    long value;
    struct Var *next;
} Var;

typedef struct {
    Var *vars;
    int failed;
} Runtime;

static char *xstrdup(const char *src) {
    size_t len;
    char *copy;

    if (!src) {
        return NULL;
    }

    len = strlen(src);
    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, src, len + 1);
    return copy;
}

static void token_array_push(TokenArray *arr, Token token) {
    Token *new_items;
    size_t new_cap;

    if (arr->len == arr->cap) {
        new_cap = (arr->cap == 0) ? 64 : arr->cap * 2;
        new_items = (Token *)realloc(arr->items, new_cap * sizeof(Token));
        if (!new_items) {
            fprintf(stderr, "mycat-lang: out of memory\n");
            exit(EXIT_FAILURE);
        }
        arr->items = new_items;
        arr->cap = new_cap;
    }

    arr->items[arr->len++] = token;
}

static void free_tokens(TokenArray *arr) {
    size_t i;

    for (i = 0; i < arr->len; i++) {
        free(arr->items[i].text);
    }
    free(arr->items);
}

static Token make_token(TokenType type, int line, long number, char *text) {
    Token token;
    token.type = type;
    token.line = line;
    token.number = number;
    token.text = text;
    return token;
}

static int is_ident_start(int ch) {
    return isalpha(ch) || ch == '_';
}

static int is_ident_char(int ch) {
    return isalnum(ch) || ch == '_';
}

static void add_identifier_or_keyword(TokenArray *tokens, const char *src,
        size_t *pos, int line) {
    size_t start = *pos;
    size_t end;
    size_t len;
    char *text;
    TokenType type = TOK_IDENTIFIER;

    while (src[*pos] != '\0' && is_ident_char((unsigned char)src[*pos])) {
        (*pos)++;
    }
    end = *pos;
    len = end - start;
    text = (char *)malloc(len + 1);
    if (!text) {
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    memcpy(text, src + start, len);
    text[len] = '\0';

    if (strcmp(text, "let") == 0) {
        type = TOK_LET;
    } else if (strcmp(text, "print") == 0) {
        type = TOK_PRINT;
    } else if (strcmp(text, "if") == 0) {
        type = TOK_IF;
    } else if (strcmp(text, "else") == 0) {
        type = TOK_ELSE;
    } else if (strcmp(text, "while") == 0) {
        type = TOK_WHILE;
    } else if (strcmp(text, "end") == 0) {
        type = TOK_END;
    }

    if (type != TOK_IDENTIFIER) {
        free(text);
        text = NULL;
    }

    token_array_push(tokens, make_token(type, line, 0, text));
}

static void add_number(TokenArray *tokens, const char *src, size_t *pos, int line) {
    size_t start = *pos;
    char *end_ptr = NULL;
    long value;

    while (isdigit((unsigned char)src[*pos])) {
        (*pos)++;
    }

    errno = 0;
    value = strtol(src + start, &end_ptr, 10);
    if (errno != 0 || end_ptr == src + start) {
        fprintf(stderr, "mycat-lang:%d: invalid number\n", line);
        exit(EXIT_FAILURE);
    }

    token_array_push(tokens, make_token(TOK_NUMBER, line, value, NULL));
}

static void add_string(TokenArray *tokens, const char *src, size_t *pos, int *line) {
    size_t start;
    size_t len;
    size_t i;
    char *raw;
    char *value;
    size_t out = 0;

    (*pos)++;
    start = *pos;
    while (src[*pos] != '\0' && src[*pos] != '"') {
        if (src[*pos] == '\n') {
            fprintf(stderr, "mycat-lang:%d: unterminated string\n", *line);
            exit(EXIT_FAILURE);
        }
        (*pos)++;
    }

    if (src[*pos] != '"') {
        fprintf(stderr, "mycat-lang:%d: unterminated string\n", *line);
        exit(EXIT_FAILURE);
    }

    len = *pos - start;
    raw = (char *)malloc(len + 1);
    if (!raw) {
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    memcpy(raw, src + start, len);
    raw[len] = '\0';

    value = (char *)malloc(len + 1);
    if (!value) {
        free(raw);
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < len; i++) {
        if (raw[i] == '\\' && i + 1 < len) {
            i++;
            if (raw[i] == 'n') {
                value[out++] = '\n';
            } else if (raw[i] == 't') {
                value[out++] = '\t';
            } else if (raw[i] == '"') {
                value[out++] = '"';
            } else if (raw[i] == '\\') {
                value[out++] = '\\';
            } else {
                value[out++] = raw[i];
            }
        } else {
            value[out++] = raw[i];
        }
    }
    value[out] = '\0';

    free(raw);
    token_array_push(tokens, make_token(TOK_STRING, *line, 0, value));
    (*pos)++;
}

static TokenArray lex_source(const char *src) {
    TokenArray tokens = {0};
    size_t pos = 0;
    int line = 1;
    int ch;

    while ((ch = (unsigned char)src[pos]) != '\0') {
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            pos++;
            continue;
        }
        if (ch == '#') {
            while (src[pos] != '\0' && src[pos] != '\n') {
                pos++;
            }
            continue;
        }
        if (ch == '\n') {
            token_array_push(&tokens, make_token(TOK_NEWLINE, line, 0, NULL));
            line++;
            pos++;
            continue;
        }
        if (isdigit(ch)) {
            add_number(&tokens, src, &pos, line);
            continue;
        }
        if (is_ident_start(ch)) {
            add_identifier_or_keyword(&tokens, src, &pos, line);
            continue;
        }
        if (ch == '"') {
            add_string(&tokens, src, &pos, &line);
            continue;
        }

        if (ch == '(') {
            token_array_push(&tokens, make_token(TOK_LPAREN, line, 0, NULL));
            pos++;
            continue;
        }
        if (ch == ')') {
            token_array_push(&tokens, make_token(TOK_RPAREN, line, 0, NULL));
            pos++;
            continue;
        }
        if (ch == '+') {
            token_array_push(&tokens, make_token(TOK_PLUS, line, 0, NULL));
            pos++;
            continue;
        }
        if (ch == '-') {
            token_array_push(&tokens, make_token(TOK_MINUS, line, 0, NULL));
            pos++;
            continue;
        }
        if (ch == '*') {
            token_array_push(&tokens, make_token(TOK_STAR, line, 0, NULL));
            pos++;
            continue;
        }
        if (ch == '/') {
            token_array_push(&tokens, make_token(TOK_SLASH, line, 0, NULL));
            pos++;
            continue;
        }
        if (ch == '=') {
            if (src[pos + 1] == '=') {
                token_array_push(&tokens, make_token(TOK_EQ, line, 0, NULL));
                pos += 2;
            } else {
                token_array_push(&tokens, make_token(TOK_ASSIGN, line, 0, NULL));
                pos++;
            }
            continue;
        }
        if (ch == '!') {
            if (src[pos + 1] == '=') {
                token_array_push(&tokens, make_token(TOK_NEQ, line, 0, NULL));
                pos += 2;
                continue;
            }
            fprintf(stderr, "mycat-lang:%d: unexpected '!'\n", line);
            exit(EXIT_FAILURE);
        }
        if (ch == '<') {
            if (src[pos + 1] == '=') {
                token_array_push(&tokens, make_token(TOK_LTE, line, 0, NULL));
                pos += 2;
            } else {
                token_array_push(&tokens, make_token(TOK_LT, line, 0, NULL));
                pos++;
            }
            continue;
        }
        if (ch == '>') {
            if (src[pos + 1] == '=') {
                token_array_push(&tokens, make_token(TOK_GTE, line, 0, NULL));
                pos += 2;
            } else {
                token_array_push(&tokens, make_token(TOK_GT, line, 0, NULL));
                pos++;
            }
            continue;
        }

        fprintf(stderr, "mycat-lang:%d: unexpected character '%c'\n", line, ch);
        exit(EXIT_FAILURE);
    }

    token_array_push(&tokens, make_token(TOK_EOF, line, 0, NULL));
    return tokens;
}

static Expr *new_expr(ExprType type) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (!expr) {
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    expr->type = type;
    return expr;
}

static Stmt *new_stmt(StmtType type, int line) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (!stmt) {
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    stmt->type = type;
    stmt->line = line;
    return stmt;
}

static void stmt_list_push(StmtList *list, Stmt *stmt) {
    Stmt **new_items;
    size_t new_cap;

    if (list->len == list->cap) {
        new_cap = (list->cap == 0) ? 16 : list->cap * 2;
        new_items = (Stmt **)realloc(list->items, new_cap * sizeof(Stmt *));
        if (!new_items) {
            fprintf(stderr, "mycat-lang: out of memory\n");
            exit(EXIT_FAILURE);
        }
        list->items = new_items;
        list->cap = new_cap;
    }

    list->items[list->len++] = stmt;
}

static Token *parser_current(Parser *p) {
    if (p->pos >= p->len) {
        return &p->tokens[p->len - 1];
    }
    return &p->tokens[p->pos];
}

static Token *parser_previous(Parser *p) {
    if (p->pos == 0) {
        return &p->tokens[0];
    }
    return &p->tokens[p->pos - 1];
}

static int parser_match(Parser *p, TokenType type) {
    if (parser_current(p)->type == type) {
        p->pos++;
        return 1;
    }
    return 0;
}

static Token *parser_expect(Parser *p, TokenType type, const char *message) {
    Token *tok = parser_current(p);
    if (tok->type == type) {
        p->pos++;
        return tok;
    }

    fprintf(stderr, "mycat-lang:%d: %s\n", tok->line, message);
    p->failed = 1;
    return tok;
}

static void parser_skip_newlines(Parser *p) {
    while (parser_match(p, TOK_NEWLINE)) {
    }
}

static Expr *parse_expression(Parser *p);

static Expr *parse_primary(Parser *p) {
    Token *tok = parser_current(p);
    Expr *expr;

    if (parser_match(p, TOK_NUMBER)) {
        expr = new_expr(EXPR_NUMBER);
        expr->as.number = parser_previous(p)->number;
        return expr;
    }
    if (parser_match(p, TOK_IDENTIFIER)) {
        expr = new_expr(EXPR_VAR);
        expr->as.name = xstrdup(parser_previous(p)->text);
        if (!expr->as.name) {
            fprintf(stderr, "mycat-lang: out of memory\n");
            exit(EXIT_FAILURE);
        }
        return expr;
    }
    if (parser_match(p, TOK_LPAREN)) {
        expr = parse_expression(p);
        parser_expect(p, TOK_RPAREN, "expected ')'");
        return expr;
    }

    fprintf(stderr, "mycat-lang:%d: expected expression\n", tok->line);
    p->failed = 1;
    return new_expr(EXPR_NUMBER);
}

static Expr *parse_unary(Parser *p) {
    Expr *expr;
    TokenType op;

    if (parser_match(p, TOK_MINUS)) {
        op = TOK_MINUS;
        expr = new_expr(EXPR_UNARY);
        expr->as.unary.op = op;
        expr->as.unary.right = parse_unary(p);
        return expr;
    }

    return parse_primary(p);
}

static Expr *parse_factor(Parser *p) {
    Expr *expr = parse_unary(p);
    Expr *right;
    Expr *bin;
    TokenType op;

    while (parser_match(p, TOK_STAR) || parser_match(p, TOK_SLASH)) {
        op = parser_previous(p)->type;
        right = parse_unary(p);
        bin = new_expr(EXPR_BINARY);
        bin->as.binary.op = op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static Expr *parse_term(Parser *p) {
    Expr *expr = parse_factor(p);
    Expr *right;
    Expr *bin;
    TokenType op;

    while (parser_match(p, TOK_PLUS) || parser_match(p, TOK_MINUS)) {
        op = parser_previous(p)->type;
        right = parse_factor(p);
        bin = new_expr(EXPR_BINARY);
        bin->as.binary.op = op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static Expr *parse_comparison(Parser *p) {
    Expr *expr = parse_term(p);
    Expr *right;
    Expr *bin;
    TokenType op;

    while (parser_match(p, TOK_LT) || parser_match(p, TOK_LTE) ||
           parser_match(p, TOK_GT) || parser_match(p, TOK_GTE)) {
        op = parser_previous(p)->type;
        right = parse_term(p);
        bin = new_expr(EXPR_BINARY);
        bin->as.binary.op = op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static Expr *parse_equality(Parser *p) {
    Expr *expr = parse_comparison(p);
    Expr *right;
    Expr *bin;
    TokenType op;

    while (parser_match(p, TOK_EQ) || parser_match(p, TOK_NEQ)) {
        op = parser_previous(p)->type;
        right = parse_comparison(p);
        bin = new_expr(EXPR_BINARY);
        bin->as.binary.op = op;
        bin->as.binary.left = expr;
        bin->as.binary.right = right;
        expr = bin;
    }
    return expr;
}

static Expr *parse_expression(Parser *p) {
    return parse_equality(p);
}

static Stmt *parse_statement(Parser *p);

static StmtList parse_stmt_list(Parser *p, TokenType stop_a, TokenType stop_b) {
    StmtList list = {0};
    TokenType current;
    Stmt *stmt;

    parser_skip_newlines(p);

    while (!p->failed) {
        current = parser_current(p)->type;
        if (current == TOK_EOF || current == stop_a || current == stop_b) {
            break;
        }

        stmt = parse_statement(p);
        if (p->failed) {
            break;
        }
        stmt_list_push(&list, stmt);

        parser_skip_newlines(p);
    }

    return list;
}

static Stmt *parse_let(Parser *p) {
    Token *name;
    Stmt *stmt;

    name = parser_expect(p, TOK_IDENTIFIER, "expected variable name after 'let'");
    parser_expect(p, TOK_ASSIGN, "expected '=' after variable name");

    stmt = new_stmt(STMT_LET, name->line);
    stmt->as.let_stmt.name = xstrdup(name->text);
    if (!stmt->as.let_stmt.name) {
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    stmt->as.let_stmt.value = parse_expression(p);
    return stmt;
}

static Stmt *parse_print(Parser *p, int line) {
    Stmt *stmt = new_stmt(STMT_PRINT, line);

    if (parser_match(p, TOK_STRING)) {
        stmt->as.print_stmt.is_string = 1;
        stmt->as.print_stmt.text = xstrdup(parser_previous(p)->text);
        if (!stmt->as.print_stmt.text) {
            fprintf(stderr, "mycat-lang: out of memory\n");
            exit(EXIT_FAILURE);
        }
        return stmt;
    }

    stmt->as.print_stmt.value = parse_expression(p);
    return stmt;
}

static Stmt *parse_if(Parser *p, int line) {
    Stmt *stmt = new_stmt(STMT_IF, line);

    stmt->as.if_stmt.condition = parse_expression(p);
    parser_expect(p, TOK_NEWLINE, "expected newline after if condition");
    stmt->as.if_stmt.then_branch = parse_stmt_list(p, TOK_ELSE, TOK_END);

    if (parser_match(p, TOK_ELSE)) {
        stmt->as.if_stmt.has_else = 1;
        parser_expect(p, TOK_NEWLINE, "expected newline after else");
        stmt->as.if_stmt.else_branch = parse_stmt_list(p, TOK_END, TOK_EOF);
    }

    parser_expect(p, TOK_END, "expected 'end' to close if");
    return stmt;
}

static Stmt *parse_while(Parser *p, int line) {
    Stmt *stmt = new_stmt(STMT_WHILE, line);

    stmt->as.while_stmt.condition = parse_expression(p);
    parser_expect(p, TOK_NEWLINE, "expected newline after while condition");
    stmt->as.while_stmt.body = parse_stmt_list(p, TOK_END, TOK_EOF);
    parser_expect(p, TOK_END, "expected 'end' to close while");
    return stmt;
}

static Stmt *parse_statement(Parser *p) {
    Token *tok = parser_current(p);

    if (parser_match(p, TOK_LET)) {
        return parse_let(p);
    }
    if (parser_match(p, TOK_PRINT)) {
        return parse_print(p, tok->line);
    }
    if (parser_match(p, TOK_IF)) {
        return parse_if(p, tok->line);
    }
    if (parser_match(p, TOK_WHILE)) {
        return parse_while(p, tok->line);
    }

    fprintf(stderr, "mycat-lang:%d: unexpected token in statement\n", tok->line);
    p->failed = 1;
    return new_stmt(STMT_PRINT, tok->line);
}

static StmtList parse_program(TokenArray *tokens) {
    Parser parser;
    StmtList program;

    parser.tokens = tokens->items;
    parser.len = tokens->len;
    parser.pos = 0;
    parser.failed = 0;

    program = parse_stmt_list(&parser, TOK_EOF, TOK_EOF);
    parser_expect(&parser, TOK_EOF, "expected end of file");

    if (parser.failed) {
        exit(EXIT_FAILURE);
    }

    return program;
}

static void free_expr(Expr *expr) {
    if (!expr) {
        return;
    }

    if (expr->type == EXPR_VAR) {
        free(expr->as.name);
    } else if (expr->type == EXPR_UNARY) {
        free_expr(expr->as.unary.right);
    } else if (expr->type == EXPR_BINARY) {
        free_expr(expr->as.binary.left);
        free_expr(expr->as.binary.right);
    }

    free(expr);
}

static void free_stmt_list(StmtList *list);

static void free_stmt(Stmt *stmt) {
    if (!stmt) {
        return;
    }

    if (stmt->type == STMT_LET) {
        free(stmt->as.let_stmt.name);
        free_expr(stmt->as.let_stmt.value);
    } else if (stmt->type == STMT_PRINT) {
        free(stmt->as.print_stmt.text);
        free_expr(stmt->as.print_stmt.value);
    } else if (stmt->type == STMT_IF) {
        free_expr(stmt->as.if_stmt.condition);
        free_stmt_list(&stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.has_else) {
            free_stmt_list(&stmt->as.if_stmt.else_branch);
        }
    } else if (stmt->type == STMT_WHILE) {
        free_expr(stmt->as.while_stmt.condition);
        free_stmt_list(&stmt->as.while_stmt.body);
    }

    free(stmt);
}

static void free_stmt_list(StmtList *list) {
    size_t i;

    for (i = 0; i < list->len; i++) {
        free_stmt(list->items[i]);
    }
    free(list->items);
}

static Var *runtime_find_var(Runtime *rt, const char *name) {
    Var *cur = rt->vars;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static void runtime_set(Runtime *rt, const char *name, long value) {
    Var *var = runtime_find_var(rt, name);
    if (var) {
        var->value = value;
        return;
    }

    var = (Var *)calloc(1, sizeof(Var));
    if (!var) {
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    var->name = xstrdup(name);
    if (!var->name) {
        free(var);
        fprintf(stderr, "mycat-lang: out of memory\n");
        exit(EXIT_FAILURE);
    }
    var->value = value;
    var->next = rt->vars;
    rt->vars = var;
}

static long runtime_get(Runtime *rt, const char *name, int line) {
    Var *var = runtime_find_var(rt, name);
    if (!var) {
        fprintf(stderr, "mycat-lang:%d: undefined variable '%s'\n", line, name);
        rt->failed = 1;
        return 0;
    }
    return var->value;
}

static long eval_expr(Runtime *rt, Expr *expr, int line) {
    long left;
    long right;

    if (!expr || rt->failed) {
        return 0;
    }

    if (expr->type == EXPR_NUMBER) {
        return expr->as.number;
    }
    if (expr->type == EXPR_VAR) {
        return runtime_get(rt, expr->as.name, line);
    }
    if (expr->type == EXPR_UNARY) {
        right = eval_expr(rt, expr->as.unary.right, line);
        if (expr->as.unary.op == TOK_MINUS) {
            return -right;
        }
        fprintf(stderr, "mycat-lang:%d: unsupported unary operator\n", line);
        rt->failed = 1;
        return 0;
    }

    left = eval_expr(rt, expr->as.binary.left, line);
    right = eval_expr(rt, expr->as.binary.right, line);
    if (rt->failed) {
        return 0;
    }

    switch (expr->as.binary.op) {
        case TOK_PLUS:
            return left + right;
        case TOK_MINUS:
            return left - right;
        case TOK_STAR:
            return left * right;
        case TOK_SLASH:
            if (right == 0) {
                fprintf(stderr, "mycat-lang:%d: division by zero\n", line);
                rt->failed = 1;
                return 0;
            }
            return left / right;
        case TOK_EQ:
            return left == right;
        case TOK_NEQ:
            return left != right;
        case TOK_LT:
            return left < right;
        case TOK_LTE:
            return left <= right;
        case TOK_GT:
            return left > right;
        case TOK_GTE:
            return left >= right;
        default:
            fprintf(stderr, "mycat-lang:%d: unsupported binary operator\n", line);
            rt->failed = 1;
            return 0;
    }
}

static void exec_stmt_list(Runtime *rt, StmtList *list);

static void exec_stmt(Runtime *rt, Stmt *stmt) {
    long condition;
    long value;

    if (rt->failed || !stmt) {
        return;
    }

    if (stmt->type == STMT_LET) {
        value = eval_expr(rt, stmt->as.let_stmt.value, stmt->line);
        if (!rt->failed) {
            runtime_set(rt, stmt->as.let_stmt.name, value);
        }
        return;
    }

    if (stmt->type == STMT_PRINT) {
        if (stmt->as.print_stmt.is_string) {
            if (printf("%s\n", stmt->as.print_stmt.text) < 0) {
                fprintf(stderr, "mycat-lang:%d: stdout write failed\n", stmt->line);
                rt->failed = 1;
            }
        } else {
            value = eval_expr(rt, stmt->as.print_stmt.value, stmt->line);
            if (!rt->failed && printf("%ld\n", value) < 0) {
                fprintf(stderr, "mycat-lang:%d: stdout write failed\n", stmt->line);
                rt->failed = 1;
            }
        }
        return;
    }

    if (stmt->type == STMT_IF) {
        condition = eval_expr(rt, stmt->as.if_stmt.condition, stmt->line);
        if (rt->failed) {
            return;
        }
        if (condition) {
            exec_stmt_list(rt, &stmt->as.if_stmt.then_branch);
        } else if (stmt->as.if_stmt.has_else) {
            exec_stmt_list(rt, &stmt->as.if_stmt.else_branch);
        }
        return;
    }

    if (stmt->type == STMT_WHILE) {
        while (!rt->failed) {
            condition = eval_expr(rt, stmt->as.while_stmt.condition, stmt->line);
            if (rt->failed || !condition) {
                break;
            }
            exec_stmt_list(rt, &stmt->as.while_stmt.body);
        }
    }
}

static void exec_stmt_list(Runtime *rt, StmtList *list) {
    size_t i;
    for (i = 0; i < list->len; i++) {
        exec_stmt(rt, list->items[i]);
        if (rt->failed) {
            return;
        }
    }
}

static void free_runtime(Runtime *rt) {
    Var *cur = rt->vars;
    Var *next;
    while (cur) {
        next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
}

static char *read_file(const char *path) {
    FILE *fp;
    long size;
    size_t read_size;
    char *buffer;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "mycat-lang: cannot open '%s'\n", path);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "mycat-lang: cannot seek '%s'\n", path);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        fprintf(stderr, "mycat-lang: cannot read size for '%s'\n", path);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        fprintf(stderr, "mycat-lang: cannot rewind '%s'\n", path);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        fprintf(stderr, "mycat-lang: out of memory\n");
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, fp);
    if (read_size != (size_t)size) {
        free(buffer);
        fclose(fp);
        fprintf(stderr, "mycat-lang: failed to read '%s'\n", path);
        return NULL;
    }
    buffer[size] = '\0';

    if (fclose(fp) != 0) {
        free(buffer);
        fprintf(stderr, "mycat-lang: failed to close '%s'\n", path);
        return NULL;
    }

    return buffer;
}

int main(int argc, char *argv[]) {
    char *source;
    TokenArray tokens;
    StmtList program;
    Runtime runtime = {0};
    int exit_code = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <script.mcl>\n", argv[0]);
        return EXIT_FAILURE;
    }

    source = read_file(argv[1]);
    if (!source) {
        return EXIT_FAILURE;
    }

    tokens = lex_source(source);
    program = parse_program(&tokens);
    exec_stmt_list(&runtime, &program);
    if (runtime.failed) {
        exit_code = EXIT_FAILURE;
    }

    free_runtime(&runtime);
    free_stmt_list(&program);
    free_tokens(&tokens);
    free(source);

    return exit_code;
}
