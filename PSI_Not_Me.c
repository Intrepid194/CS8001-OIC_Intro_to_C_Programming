/*
 * psi.c -- A tree-walking interpreter for the PSI Lisp dialect.
 *
 * Single translation unit, C99.  Build with:
 *
 *     cc -std=c99 -O2 -Wall -Wextra -o psi psi.c
 *
 * Usage:
 *     ./psi                 interactive REPL
 *     ./psi file.psi ...    run files, then exit
 *     ./psi -i file.psi     run files, then drop into the REPL
 *
 * On startup, "__start.psi" is loaded from the working directory if present.
 *
 * Implementation notes
 * --------------------
 *  * Every pval is heap allocated and reference counted.  Copying a value is
 *    just an rc bump: the base types and lists are immutable, so sharing is
 *    indistinguishable from copying, while cells and closures are *required*
 *    by the spec to be shared rather than copied.
 *  * Environments are the one thing genuinely deep-copied: `fn` snapshots the
 *    whole environment chain (bindings duplicated, values rc-shared) so that
 *    later `def`s can never reach back into a live closure.
 *  * Errors are ordinary values of type T_ERR; "throwing" is returning.
 *
 * Sections, in order:
 *    1. utilities            5. errors
 *    2. values / refcounts   6. parser & reader
 *    3. environments         7. evaluator
 *    4. printing             8. builtins
 *                            9. REPL / main
 */

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ===================================================================== */
/* 1. utilities                                                          */
/* ===================================================================== */

static void oom(void) { fputs("psi: out of memory\n", stderr); exit(1); }

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) oom();
    return p;
}

static void *xrealloc(void *q, size_t n) {
    void *p = realloc(q, n ? n : 1);
    if (!p) oom();
    return p;
}

/* Growable byte buffer.  Always kept NUL-terminated so that it can be handed
 * to C string functions, even though PSI strings may contain embedded NULs. */
typedef struct {
    char  *p;
    size_t n, cap;
} SB;

static void sb_init(SB *b) {
    b->cap = 32;
    b->p = xmalloc(b->cap);
    b->n = 0;
    b->p[0] = 0;
}

static void sb_free(SB *b) { free(b->p); b->p = NULL; b->n = b->cap = 0; }

static void sb_need(SB *b, size_t extra) {
    if (b->n + extra + 1 > b->cap) {
        while (b->n + extra + 1 > b->cap) b->cap *= 2;
        b->p = xrealloc(b->p, b->cap);
    }
}

static void sb_putc(SB *b, int c) {
    sb_need(b, 1);
    b->p[b->n++] = (char)c;
    b->p[b->n] = 0;
}

static void sb_write(SB *b, const char *s, size_t n) {
    sb_need(b, n);
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}

static void sb_puts(SB *b, const char *s) { sb_write(b, s, strlen(s)); }

static void sb_printf(SB *b, const char *fmt, ...) {
    char tmp[128];
    va_list ap;
    va_start(ap, fmt);
    int k = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (k > 0) sb_write(b, tmp, (size_t)k < sizeof tmp ? (size_t)k : sizeof tmp - 1);
}

/* ===================================================================== */
/* 2. values                                                             */
/* ===================================================================== */

typedef enum {
    T_BOOL, T_NUM, T_STR, T_SYM, T_LIST, T_CELL, T_ERR, T_FUN
} Type;

typedef struct PVal PVal;
typedef struct Env  Env;

typedef PVal *(*Builtin)(PVal **argv, int argc, Env *env);

struct PVal {
    Type t;
    long rc;
    union {
        int       b;                                  /* T_BOOL */
        long long n;                                  /* T_NUM  */
        struct { char *s; size_t len; } str;          /* T_STR, T_SYM */
        struct { PVal *head, *tail; } list;           /* T_LIST, head==NULL => () */
        PVal *inner;                                  /* T_CELL, T_ERR */
        struct {                                      /* T_FUN */
            int         is_builtin;
            int         is_macro;
            Builtin     cfn;
            const char *cname;   /* builtins only */
            PVal       *name;    /* closures: symbol or NULL */
            PVal       *args;
            PVal       *body;
            Env        *env;
        } fn;
    } u;
};

static PVal *NIL, *TRUE_V, *FALSE_V;   /* immortal singletons */

static void env_unref(Env *e);
static Env *env_ref(Env *e);

static PVal *ref(PVal *v) { if (v) v->rc++; return v; }

static void unref(PVal *v) {
    /* Iterative down the list spine so that freeing a long list cannot blow
     * the C stack. */
    while (v) {
        if (--v->rc > 0) return;
        PVal *next = NULL;
        switch (v->t) {
        case T_BOOL: case T_NUM:
            break;
        case T_STR: case T_SYM:
            free(v->u.str.s);
            break;
        case T_LIST:
            if (v->u.list.head) { unref(v->u.list.head); next = v->u.list.tail; }
            break;
        case T_CELL: case T_ERR:
            unref(v->u.inner);
            break;
        case T_FUN:
            if (!v->u.fn.is_builtin) {
                unref(v->u.fn.name);
                unref(v->u.fn.args);
                unref(v->u.fn.body);
                env_unref(v->u.fn.env);
            }
            break;
        }
        free(v);
        v = next;
    }
}

static PVal *alloc(Type t) {
    PVal *v = xmalloc(sizeof *v);
    memset(v, 0, sizeof *v);
    v->t  = t;
    v->rc = 1;
    return v;
}

static PVal *mk_bool(int b) { return ref(b ? TRUE_V : FALSE_V); }
static PVal *mk_num(long long n) { PVal *v = alloc(T_NUM); v->u.n = n; return v; }

static PVal *mk_strn(const char *s, size_t len) {
    PVal *v = alloc(T_STR);
    v->u.str.s = xmalloc(len + 1);
    if (len) memcpy(v->u.str.s, s, len);
    v->u.str.s[len] = 0;
    v->u.str.len = len;
    return v;
}
static PVal *mk_str(const char *s) { return mk_strn(s, strlen(s)); }

static PVal *mk_symn(const char *s, size_t len) {
    PVal *v = mk_strn(s, len);
    v->t = T_SYM;
    return v;
}
static PVal *mk_sym(const char *s) { return mk_symn(s, strlen(s)); }

/* Steals both references. */
static PVal *mk_cons(PVal *head, PVal *tail) {
    PVal *v = alloc(T_LIST);
    v->u.list.head = head;
    v->u.list.tail = tail;
    return v;
}

static PVal *mk_cell(PVal *inner) { PVal *v = alloc(T_CELL); v->u.inner = inner; return v; }
static PVal *mk_err(PVal *inner)  { PVal *v = alloc(T_ERR);  v->u.inner = inner; return v; }

static int is_err(PVal *v)   { return v && v->t == T_ERR; }
static int is_true(PVal *v)  { return !(v->t == T_BOOL && !v->u.b); }
static int is_nil(PVal *v)   { return v->t == T_LIST && v->u.list.head == NULL; }

static int list_len(PVal *l) {
    int k = 0;
    for (; l && l->t == T_LIST && l->u.list.head; l = l->u.list.tail) k++;
    return k;
}

static PVal *nth(PVal *l, int i) {          /* borrowed, NULL if out of range */
    for (; l && l->t == T_LIST && l->u.list.head; l = l->u.list.tail) {
        if (i-- == 0) return l->u.list.head;
    }
    return NULL;
}

/* Steals every element reference. */
static PVal *list_from(PVal **items, int n) {
    PVal *r = ref(NIL);
    for (int i = n - 1; i >= 0; i--) r = mk_cons(items[i], r);
    return r;
}

/* NULL-terminated varargs list constructor; steals every reference. */
static PVal *pl(PVal *first, ...) {
    PVal   *items[16];
    int     n = 0;
    va_list ap;
    va_start(ap, first);
    for (PVal *v = first; v != NULL && n < 16; v = va_arg(ap, PVal *)) items[n++] = v;
    va_end(ap);
    return list_from(items, n);
}

static int sym_is(PVal *v, const char *name) {
    return v && v->t == T_SYM && strcmp(v->u.str.s, name) == 0;
}

/* Value equality: base types and lists compare by value, cells and functions
 * by object identity. */
static int pval_equal(PVal *a, PVal *b) {
    if (a == b) return 1;
    if (a->t != b->t) return 0;
    switch (a->t) {
    case T_BOOL: return (!a->u.b) == (!b->u.b);
    case T_NUM:  return a->u.n == b->u.n;
    case T_STR:  return a->u.str.len == b->u.str.len &&
                        memcmp(a->u.str.s, b->u.str.s, a->u.str.len) == 0;
    case T_SYM:  return strcmp(a->u.str.s, b->u.str.s) == 0;
    case T_ERR:  return pval_equal(a->u.inner, b->u.inner);
    case T_LIST:
        while (a->u.list.head && b->u.list.head) {
            if (!pval_equal(a->u.list.head, b->u.list.head)) return 0;
            a = a->u.list.tail;
            b = b->u.list.tail;
        }
        return !a->u.list.head && !b->u.list.head;
    case T_CELL: case T_FUN:
        return 0;                       /* identity already checked above */
    }
    return 0;
}

/* ===================================================================== */
/* 3. environments                                                       */
/* ===================================================================== */

typedef struct Binding {
    PVal *sym, *val;
    struct Binding *next;
} Binding;

struct Env {
    long     rc;
    Binding *head;
    Env     *parent;
};

static Env *env_ref(Env *e) { if (e) e->rc++; return e; }

static void env_unref(Env *e) {
    while (e) {
        if (--e->rc > 0) return;
        Binding *b = e->head;
        while (b) {
            Binding *n = b->next;
            unref(b->sym);
            unref(b->val);
            free(b);
            b = n;
        }
        Env *p = e->parent;
        free(e);
        e = p;
    }
}

static Env *env_new(Env *parent) {
    Env *e = xmalloc(sizeof *e);
    e->rc = 1;
    e->head = NULL;
    e->parent = env_ref(parent);
    return e;
}

static PVal *env_get(Env *e, const char *name) {     /* borrowed or NULL */
    for (; e; e = e->parent)
        for (Binding *b = e->head; b; b = b->next)
            if (strcmp(b->sym->u.str.s, name) == 0) return b->val;
    return NULL;
}

/* Steals `val`, borrows `sym`.  Rebinding in the same frame replaces. */
static void env_put(Env *e, PVal *sym, PVal *val) {
    for (Binding *b = e->head; b; b = b->next) {
        if (strcmp(b->sym->u.str.s, sym->u.str.s) == 0) {
            unref(b->val);
            b->val = val;
            return;
        }
    }
    Binding *b = xmalloc(sizeof *b);
    b->sym  = ref(sym);
    b->val  = val;
    b->next = e->head;
    e->head = b;
}

/* Deep-copies the whole chain: bindings are duplicated, values are shared.
 * This is what makes a closure's captured environment "frozen". */
static Env *env_copy(Env *e) {
    if (!e) return NULL;
    Env *n = xmalloc(sizeof *n);
    n->rc = 1;
    n->head = NULL;
    n->parent = env_copy(e->parent);
    Binding **tail = &n->head;
    for (Binding *b = e->head; b; b = b->next) {
        Binding *c = xmalloc(sizeof *c);
        c->sym  = ref(b->sym);
        c->val  = ref(b->val);
        c->next = NULL;
        *tail = c;
        tail = &c->next;
    }
    return n;
}

static Env *g_global;

/* ===================================================================== */
/* 4. printing                                                           */
/* ===================================================================== */

static void write_val(SB *b, PVal *v);

static void write_string_literal(SB *b, const char *s, size_t len) {
    sb_putc(b, '"');
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if      (c == '"')  sb_puts(b, "\\\"");
        else if (c == '\\') sb_puts(b, "\\\\");
        else if (c == '\n') sb_puts(b, "\\n");
        else if (c >= 32 && c < 127) sb_putc(b, c);
        else sb_printf(b, "\\x%02x", c);
    }
    sb_putc(b, '"');
}

static void write_list(SB *b, PVal *v) {
    /* (quote x) prints as 'x */
    if (list_len(v) == 2 && sym_is(nth(v, 0), "quote")) {
        sb_putc(b, '\'');
        write_val(b, nth(v, 1));
        return;
    }
    sb_putc(b, '(');
    int first = 1;
    for (PVal *p = v; p->u.list.head; p = p->u.list.tail) {
        if (!first) sb_putc(b, ' ');
        first = 0;
        write_val(b, p->u.list.head);
    }
    sb_putc(b, ')');
}

static void write_val(SB *b, PVal *v) {
    switch (v->t) {
    case T_BOOL: sb_puts(b, v->u.b ? "#t" : "#f"); break;
    case T_NUM:  sb_printf(b, "%lld", v->u.n); break;
    case T_STR:  write_string_literal(b, v->u.str.s, v->u.str.len); break;
    case T_SYM:  sb_puts(b, v->u.str.s); break;
    case T_LIST: write_list(b, v); break;
    case T_CELL:
        sb_puts(b, "$cell{");
        write_val(b, v->u.inner);
        sb_printf(b, "}@%p", (void *)v);
        break;
    case T_ERR:
        sb_puts(b, "$error{");
        write_val(b, v->u.inner);
        sb_putc(b, '}');
        break;
    case T_FUN:
        if (v->u.fn.is_builtin) {
            sb_printf(b, "$builtin{%s}", v->u.fn.cname);
        } else {
            sb_puts(b, v->u.fn.is_macro ? "$macro{" : "$lambda{");
            write_val(b, v->u.fn.args);
            sb_putc(b, ' ');
            write_val(b, v->u.fn.body);
            sb_putc(b, '}');
            if (v->u.fn.name) sb_printf(b, "@%s", v->u.fn.name->u.str.s);
        }
        break;
    }
}

/* Canonical representation of v, as a fresh PSI string. */
static PVal *to_pstring(PVal *v) {
    SB b;
    sb_init(&b);
    write_val(&b, v);
    PVal *r = mk_strn(b.p, b.n);
    sb_free(&b);
    return r;
}

static void fwrite_val(FILE *f, PVal *v) {
    SB b;
    sb_init(&b);
    write_val(&b, v);
    fwrite(b.p, 1, b.n, f);
    sb_free(&b);
}

/* ===================================================================== */
/* 5. errors                                                             */
/* ===================================================================== */

static PVal *simple_error(const char *sym) { return mk_err(mk_sym(sym)); }

static PVal *type_error(const char *fn, int pos, const char *want, PVal *got) {
    return mk_err(pl(mk_sym("type-error"), mk_sym(fn), mk_num(pos),
                     mk_sym(want), ref(got), NULL));
}

static PVal *value_error(const char *fn, PVal *v) {
    return mk_err(pl(mk_sym("value-error"), mk_sym(fn), ref(v), NULL));
}

/* (arity-error <name> (<cmp> <k>) <got>); name omitted when NULL. */
static PVal *arity_error_sym(PVal *name, const char *cmp, long long k, int got) {
    PVal *spec = pl(mk_sym(cmp), mk_num(k), NULL);
    if (name)
        return mk_err(pl(mk_sym("arity-error"), ref(name), spec, mk_num(got), NULL));
    return mk_err(pl(mk_sym("arity-error"), spec, mk_num(got), NULL));
}

static PVal *arity_error(const char *fn, const char *cmp, long long k, int got) {
    PVal *s = mk_sym(fn);
    PVal *e = arity_error_sym(s, cmp, k, got);
    unref(s);
    return e;
}

#define NEED_EXACT(name, k) \
    do { if (argc != (k)) return arity_error(name, "=", (k), argc); } while (0)
#define NEED_MIN(name, k) \
    do { if (argc < (k)) return arity_error(name, ">=", (k), argc); } while (0)

/* ===================================================================== */
/* 6. parser and reader                                                  */
/* ===================================================================== */

enum { PS_OK = 0, PS_END, PS_CLOSER, PS_INVALID, PS_INCOMPLETE };

typedef struct {
    const char *p;      /* cursor */
    const char *bad;    /* start of the offending token or form */
    size_t      bad_len;/* length of an invalid token (0 => to end of input) */
    int         status;
} Parser;

static int sym_char(int c) {
    return c && (isalnum((unsigned char)c) || strchr("+-*/%=<>!:&_$", c) != NULL);
}

/* Legal symbol names, as seen by the *reader*: '$' is reader-reject, and '_'
 * and '&' are legal only as complete symbols. */
static int valid_symbol(const char *s, size_t len) {
    if (len == 0) return 0;
    if (len == 1 && (s[0] == '_' || s[0] == '&')) return 1;
    if (isdigit((unsigned char)s[0])) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (!sym_char((unsigned char)c)) return 0;
        if (c == '_' || c == '&' || c == '$') return 0;
    }
    if (!isalpha((unsigned char)s[0])) {
        /* a symbol starting non-alphanumeric must be entirely non-alphanumeric */
        for (size_t i = 0; i < len; i++)
            if (isalnum((unsigned char)s[i])) return 0;
    }
    return 1;
}

static int looks_numeric(const char *s, size_t len) {
    size_t i = 0;
    if (len && (s[0] == '+' || s[0] == '-')) i = 1;
    if (i >= len) return 0;
    for (; i < len; i++)
        if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

static void skip_inactive(Parser *ps) {
    for (;;) {
        while (isspace((unsigned char)*ps->p)) ps->p++;
        if (*ps->p == ';') {
            while (*ps->p && *ps->p != '\n') ps->p++;
            continue;                       /* the newline is eaten by isspace */
        }
        break;
    }
}

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static PVal *parse_one(Parser *ps);

static PVal *parse_string_literal(Parser *ps, const char *start) {
    SB b;
    sb_init(&b);
    ps->p++;                                /* opening quote */
    for (;;) {
        unsigned char c = (unsigned char)*ps->p;
        if (c == 0) {
            sb_free(&b);
            ps->status = PS_INCOMPLETE;
            ps->bad = start;
            ps->bad_len = 0;
            return NULL;
        }
        if (c == '"') { ps->p++; break; }
        if (c == '\\') {
            unsigned char e = (unsigned char)ps->p[1];
            int h1, h2;
            if      (e == '0')  { sb_putc(&b, 0);   ps->p += 2; }
            else if (e == 'n')  { sb_putc(&b, 10);  ps->p += 2; }
            else if (e == '"')  { sb_putc(&b, 34);  ps->p += 2; }
            else if (e == '\\') { sb_putc(&b, 92);  ps->p += 2; }
            else if (e == 'x' && (h1 = hexval((unsigned char)ps->p[2])) >= 0 &&
                                 (h2 = hexval((unsigned char)ps->p[3])) >= 0) {
                sb_putc(&b, h1 * 16 + h2);
                ps->p += 4;
            } else {
                sb_putc(&b, '\\');          /* a lone backslash is literal */
                ps->p += 1;
            }
            continue;
        }
        sb_putc(&b, c);
        ps->p++;
    }
    PVal *v = mk_strn(b.p, b.n);
    sb_free(&b);
    ps->status = PS_OK;
    return v;
}

static PVal *parse_list(Parser *ps, const char *start) {
    PVal **items = NULL;
    int    n = 0, cap = 0;
    ps->p++;                                /* '(' */
    for (;;) {
        skip_inactive(ps);
        if (*ps->p == 0) {
            ps->status = PS_INCOMPLETE;
            ps->bad = start;
            ps->bad_len = 0;
            goto fail;
        }
        if (*ps->p == ')') { ps->p++; break; }
        PVal *item = parse_one(ps);
        if (!item) {
            if (ps->status == PS_END || ps->status == PS_INCOMPLETE) {
                ps->status = PS_INCOMPLETE;
                ps->bad = start;            /* report from the outermost form */
                ps->bad_len = 0;
            }
            goto fail;
        }
        if (n == cap) {
            cap = cap ? cap * 2 : 8;
            items = xrealloc(items, (size_t)cap * sizeof *items);
        }
        items[n++] = item;
    }
    {
        PVal *r = list_from(items, n);
        free(items);
        ps->status = PS_OK;
        return r;
    }
fail:
    for (int i = 0; i < n; i++) unref(items[i]);
    free(items);
    return NULL;
}

/* Parses exactly one pval.  Returns NULL with ps->status set otherwise. */
static PVal *parse_one(Parser *ps) {
    skip_inactive(ps);
    const char *start = ps->p;
    unsigned char c = (unsigned char)*ps->p;

    if (c == 0)   { ps->status = PS_END;    ps->bad = start; ps->bad_len = 0; return NULL; }
    if (c == ')') { ps->status = PS_CLOSER; ps->bad = start; ps->bad_len = 1; return NULL; }
    if (c == '(') return parse_list(ps, start);
    if (c == '"') return parse_string_literal(ps, start);

    if (c == '\'') {                        /* shorthand quote */
        ps->p++;
        PVal *x = parse_one(ps);
        if (!x) {
            if (ps->status != PS_INVALID) {   /* dangling quote */
                ps->status = PS_INCOMPLETE;
                ps->bad = start;
                ps->bad_len = 0;
            }
            return NULL;
        }
        ps->status = PS_OK;
        return pl(mk_sym("quote"), x, NULL);
    }

    if (c == '#') {                         /* boolean literal */
        const char *q = ps->p + 1;
        while (sym_char((unsigned char)*q)) q++;
        size_t len = (size_t)(q - ps->p);
        if (len == 2 && (ps->p[1] == 't' || ps->p[1] == 'f')) {
            int val = ps->p[1] == 't';
            ps->p = q;
            ps->status = PS_OK;
            return mk_bool(val);
        }
        ps->status = PS_INVALID;
        ps->bad = start;
        ps->bad_len = len;
        return NULL;
    }

    if (sym_char(c)) {                      /* maximal munch, then classify */
        const char *q = ps->p;
        while (sym_char((unsigned char)*q)) q++;
        size_t len = (size_t)(q - ps->p);
        if (looks_numeric(ps->p, len)) {
            char   buf[32];
            size_t k = len < sizeof buf - 1 ? len : sizeof buf - 1;
            memcpy(buf, ps->p, k);
            buf[k] = 0;
            long long n = strtoll(buf, NULL, 10);
            ps->p = q;
            ps->status = PS_OK;
            return mk_num(n);
        }
        if (valid_symbol(ps->p, len)) {
            PVal *v = mk_symn(ps->p, len);
            ps->p = q;
            ps->status = PS_OK;
            return v;
        }
        ps->status = PS_INVALID;
        ps->bad = start;
        ps->bad_len = len;
        return NULL;
    }

    ps->status = PS_INVALID;         /* a character legal nowhere at all */
    ps->bad = start;
    ps->bad_len = 1;
    return NULL;
}

static PVal *parse_error_val(Parser *ps) {
    const char *at = ps->bad ? ps->bad : ps->p;
    /* incomplete-parse reports the whole unfinished form, since the point is
     * that the text is a prefix of something valid; invalid-token reports
     * only the token that could not be tokenized. */
    if (ps->status == PS_INCOMPLETE)
        return mk_err(pl(mk_sym("incomplete-parse"), mk_str(at), NULL));
    size_t len = ps->bad_len ? ps->bad_len : strlen(at);
    return mk_err(pl(mk_sym("invalid-token"), mk_strn(at, len), NULL));
}

/* ===================================================================== */
/* 7. evaluator                                                          */
/* ===================================================================== */

enum { SF_QUOTE, SF_IF, SF_DEF, SF_FN, SF_MACRO, SF_DO,
       SF_LET, SF_AND, SF_OR, SF_TRY, SF_LOOP, SF_COUNT };

static const char *sf_names[SF_COUNT] = {
    "quote", "if", "def", "fn", "macro", "do", "let", "and", "or", "try", "loop"
};

static int special_id(const char *name) {
    for (int i = 0; i < SF_COUNT; i++)
        if (strcmp(name, sf_names[i]) == 0) return i;
    return -1;
}

static PVal *eval(PVal *form, Env *env);
static PVal *apply(PVal *f, PVal **argv, int argc, Env *env);

/* Guards against blowing the C stack on runaway PSI recursion. */
#define MAX_DEPTH 9000
static int g_depth = 0;

static PVal *check_arglist(PVal *al) {      /* NULL if legal, else the error */
    PVal *bad = mk_err(pl(mk_sym("arglist-error"), ref(al), NULL));
    if (al->t != T_LIST) return bad;
    int n = list_len(al), amp = -1, i = 0;
    for (PVal *p = al; p->u.list.head; p = p->u.list.tail, i++) {
        PVal *e = p->u.list.head;
        if (e->t != T_SYM) return bad;
        if (strcmp(e->u.str.s, "&") == 0) {
            if (amp >= 0) return bad;
            amp = i;
        }
        for (int j = 0; j < i; j++)
            if (pval_equal(nth(al, j), e)) return bad;
    }
    if (amp >= 0 && amp != n - 2) return bad;
    unref(bad);
    return NULL;
}

static PVal *make_closure(PVal *name, PVal *args, PVal *body, Env *env, int is_macro) {
    if (name && name->t != T_SYM)
        return type_error(is_macro ? "macro" : "fn", 1, "symbol", name);
    PVal *bad = check_arglist(args);
    if (bad) return bad;
    PVal *f = alloc(T_FUN);
    f->u.fn.is_builtin = 0;
    f->u.fn.is_macro   = is_macro;
    f->u.fn.name = name ? ref(name) : NULL;
    f->u.fn.args = ref(args);
    f->u.fn.body = ref(body);
    f->u.fn.env  = env_copy(env);           /* the frozen snapshot */
    return f;
}

/* The names of builtins may not be rebound in the global environment. */
static int is_protected(const char *name) {
    PVal *v = NULL;
    for (Binding *b = g_global->head; b; b = b->next)
        if (strcmp(b->sym->u.str.s, name) == 0) { v = b->val; break; }
    return v && v->t == T_FUN && v->u.fn.is_builtin;
}

static PVal *eval_special(int id, PVal *form, Env *env) {
    int   n = list_len(form) - 1;           /* argument count */
    PVal *a1 = nth(form, 1), *a2 = nth(form, 2), *a3 = nth(form, 3);

    switch (id) {
    case SF_QUOTE:
        if (n != 1) return arity_error("quote", "=", 1, n);
        return ref(a1);

    case SF_IF: {
        if (n < 2 || n > 3) return arity_error("if", ">=", 2, n);
        PVal *c = eval(a1, env);
        if (is_err(c)) return c;
        int t = is_true(c);
        unref(c);
        if (t) return eval(a2, env);
        return a3 ? eval(a3, env) : mk_bool(0);
    }

    case SF_DEF: {
        if (n != 2 && n != 3) return arity_error("def", ">=", 2, n);
        if (a1->t != T_SYM) return type_error("def", 1, "symbol", a1);
        if (is_protected(a1->u.str.s))
            return mk_err(pl(mk_sym("protected-symbol"), ref(a1), NULL));
        PVal *v = (n == 2) ? eval(a2, env)
                           : make_closure(a1, a2, a3, env, 0);
        if (is_err(v)) return v;
        env_put(g_global, a1, ref(v));
        return v;
    }

    case SF_FN:
        if (n != 2 && n != 3) return arity_error("fn", ">=", 2, n);
        if (n == 2) return make_closure(NULL, a1, a2, env, 0);
        return make_closure(a1, a2, a3, env, 0);

    case SF_MACRO: {
        if (n != 3) return arity_error("macro", "=", 3, n);
        PVal *m = make_closure(a1, a2, a3, env, 1);
        if (is_err(m)) return m;
        if (is_protected(a1->u.str.s)) {
            unref(m);
            return mk_err(pl(mk_sym("protected-symbol"), ref(a1), NULL));
        }
        env_put(g_global, a1, ref(m));
        return m;
    }

    case SF_DO: {
        PVal *last = mk_bool(1);
        for (PVal *p = form->u.list.tail; p->u.list.head; p = p->u.list.tail) {
            unref(last);
            last = eval(p->u.list.head, env);
            if (is_err(last)) return last;
        }
        return last;
    }

    case SF_LET: {
        if (n % 2 == 0)
            return mk_err(pl(mk_sym("arity-error"), mk_sym("let"),
                             mk_sym("odd"), mk_num(n), NULL));
        Env *local = env_new(env);
        PVal *r = NULL;
        for (int i = 1; i + 1 < n; i += 2) {
            PVal *name = nth(form, i), *expr = nth(form, i + 1);
            if (name->t != T_SYM) { r = type_error("let", i, "symbol", name); break; }
            PVal *v = eval(expr, local);
            if (is_err(v)) { r = v; break; }
            env_put(local, name, v);
        }
        if (!r) r = eval(nth(form, n), local);
        env_unref(local);
        return r;
    }

    case SF_AND: {
        PVal *last = mk_bool(1);
        for (PVal *p = form->u.list.tail; p->u.list.head; p = p->u.list.tail) {
            unref(last);
            last = eval(p->u.list.head, env);
            if (is_err(last)) return last;
            if (!is_true(last)) return last;        /* short circuit on #f */
        }
        return last;
    }

    case SF_OR: {
        for (PVal *p = form->u.list.tail; p->u.list.head; p = p->u.list.tail) {
            PVal *v = eval(p->u.list.head, env);
            if (is_err(v)) return v;
            if (is_true(v)) return v;
            unref(v);
        }
        return mk_bool(0);
    }

    case SF_TRY: {
        if (n != 1) return arity_error("try", "=", 1, n);
        PVal *v = eval(a1, env);
        if (is_err(v)) {
            PVal *r = pl(mk_bool(0), ref(v->u.inner), NULL);
            unref(v);
            return r;
        }
        return pl(mk_bool(1), v, NULL);
    }

    case SF_LOOP: {
        if (n != 1) return arity_error("loop", "=", 1, n);
        for (;;) {
            PVal *v = eval(a1, env);
            if (is_err(v)) return v;
            if (!is_true(v)) { unref(v); return mk_bool(1); }
            unref(v);
        }
    }
    }
    return simple_error("internal-error");
}

static PVal *eval_standard(PVal *form, Env *env) {
    int    n = list_len(form);
    PVal **argv = xmalloc((size_t)n * sizeof *argv);
    int    got = 0;
    PVal  *err = NULL;

    for (PVal *p = form; p->u.list.head; p = p->u.list.tail) {
        PVal *v = eval(p->u.list.head, env);
        if (is_err(v)) { err = v; break; }   /* leftmost error wins; stop here */
        argv[got++] = v;
    }
    if (!err) {
        if (argv[0]->t != T_FUN) err = simple_error("inapplicable-head");
        else                     err = apply(argv[0], argv + 1, n - 1, env);
    }
    for (int i = 0; i < got; i++) unref(argv[i]);
    free(argv);
    return err;
}

static PVal *eval(PVal *form, Env *env) {
    if (!form) return ref(NIL);
    if (++g_depth > MAX_DEPTH) { g_depth--; return simple_error("stack-overflow"); }

    PVal *r;
    switch (form->t) {
    case T_SYM: {
        PVal *v = env_get(env, form->u.str.s);
        r = v ? ref(v) : mk_err(pl(mk_sym("unbound"), ref(form), NULL));
        break;
    }
    case T_LIST: {
        if (!form->u.list.head) { r = ref(form); break; }    /* () self-evaluates */
        PVal *head = form->u.list.head;
        if (head->t == T_SYM) {
            int id = special_id(head->u.str.s);
            if (id >= 0) { r = eval_special(id, form, env); break; }

            PVal *bound = env_get(env, head->u.str.s);
            if (bound && bound->t == T_FUN && bound->u.fn.is_macro) {
                /* stage 1: expand on the *unevaluated* subforms */
                PVal  *m = ref(bound);
                int    n = list_len(form) - 1;
                PVal **argv = xmalloc((size_t)(n ? n : 1) * sizeof *argv);
                int    i = 0;
                for (PVal *p = form->u.list.tail; p->u.list.head; p = p->u.list.tail)
                    argv[i++] = p->u.list.head;              /* borrowed */
                PVal *mex = apply(m, argv, n, env);
                free(argv);
                unref(m);
                /* stage 2: evaluate the expansion */
                if (is_err(mex)) { r = mex; break; }
                r = eval(mex, env);
                unref(mex);
                break;
            }
        }
        r = eval_standard(form, env);
        break;
    }
    default:
        r = ref(form);
        break;
    }
    g_depth--;
    return r;
}

static PVal *apply(PVal *f, PVal **argv, int argc, Env *env) {
    if (f->u.fn.is_builtin) return f->u.fn.cfn(argv, argc, env);

    PVal *al = f->u.fn.args;
    int   n = list_len(al), amp = -1;
    for (int i = 0; i < n; i++)
        if (sym_is(nth(al, i), "&")) { amp = i; break; }
    int mandatory = (amp >= 0) ? amp : n;

    if (amp >= 0) {
        if (argc < mandatory)
            return arity_error_sym(f->u.fn.name, ">=", mandatory, argc);
    } else if (argc != mandatory) {
        return arity_error_sym(f->u.fn.name, "=", mandatory, argc);
    }

    Env *local = env_new(f->u.fn.env);      /* parent is the frozen copy */
    for (int i = 0; i < mandatory; i++)
        env_put(local, nth(al, i), ref(argv[i]));
    if (amp >= 0) {
        PVal *rest = ref(NIL);
        for (int i = argc - 1; i >= mandatory; i--) rest = mk_cons(ref(argv[i]), rest);
        env_put(local, nth(al, n - 1), rest);
    }
    if (f->u.fn.name) env_put(local, f->u.fn.name, ref(f));   /* recursion */

    PVal *r = eval(f->u.fn.body, local);
    env_unref(local);
    return r;
}

/* ===================================================================== */
/* 8. builtins                                                           */
/* ===================================================================== */

static const char *type_name(PVal *v) {
    switch (v->t) {
    case T_BOOL: return "bool";
    case T_NUM:  return "number";
    case T_STR:  return "string";
    case T_SYM:  return "symbol";
    case T_LIST: return "list";
    case T_CELL: return "cell";
    case T_ERR:  return "error";
    case T_FUN:  return "function";
    }
    return "unknown";
}

/* Returns an error if any argument is not a number, checking left to right. */
static PVal *all_numbers(const char *fn, PVal **argv, int argc) {
    for (int i = 0; i < argc; i++)
        if (argv[i]->t != T_NUM) return type_error(fn, i + 1, "number", argv[i]);
    return NULL;
}

static PVal *bi_add(PVal **argv, int argc, Env *env) {
    (void)env;
    PVal *e = all_numbers("+", argv, argc);
    if (e) return e;
    unsigned long long s = 0;
    for (int i = 0; i < argc; i++) s += (unsigned long long)argv[i]->u.n;
    return mk_num((long long)s);
}

static PVal *bi_sub(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_MIN("-", 1);
    PVal *e = all_numbers("-", argv, argc);
    if (e) return e;
    if (argc == 1) return mk_num((long long)(0ULL - (unsigned long long)argv[0]->u.n));
    unsigned long long s = (unsigned long long)argv[0]->u.n;
    for (int i = 1; i < argc; i++) s -= (unsigned long long)argv[i]->u.n;
    return mk_num((long long)s);
}

static PVal *bi_mul(PVal **argv, int argc, Env *env) {
    (void)env;
    PVal *e = all_numbers("*", argv, argc);
    if (e) return e;
    unsigned long long s = 1;
    for (int i = 0; i < argc; i++) s *= (unsigned long long)argv[i]->u.n;
    return mk_num((long long)s);
}

/* Floor division, per the spec: results round toward negative infinity. */
static long long floordiv(long long a, long long b) {
    if (a == LLONG_MIN && b == -1) return LLONG_MIN;
    long long q = a / b, r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) q--;
    return q;
}

static PVal *bi_div(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_MIN("/", 2);
    PVal *e = all_numbers("/", argv, argc);
    if (e) return e;
    long long acc = argv[0]->u.n;
    for (int i = 1; i < argc; i++) {
        if (argv[i]->u.n == 0) return simple_error("division-by-zero");
        acc = floordiv(acc, argv[i]->u.n);
    }
    return mk_num(acc);
}

static PVal *bi_mod(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("%", 2);
    PVal *e = all_numbers("%", argv, argc);
    if (e) return e;
    long long b = argv[1]->u.n;
    if (b == 0) return simple_error("division-by-zero");
    if (b == LLONG_MIN) {
        long long r = argv[0]->u.n;
        if (r == LLONG_MIN) return mk_num(0);
        if (r < 0) return mk_num((long long)((unsigned long long)r - (unsigned long long)b));
        return mk_num(r);
    }
    long long r = argv[0]->u.n % b;
    if (r < 0) r += (b < 0 ? -b : b);
    return mk_num(r);
}

static PVal *bi_eq(PVal **argv, int argc, Env *env) {
    (void)env;
    for (int i = 1; i < argc; i++)
        if (!pval_equal(argv[i - 1], argv[i])) return mk_bool(0);
    return mk_bool(1);
}

static PVal *bi_neq(PVal **argv, int argc, Env *env) {
    PVal *v = bi_eq(argv, argc, env);
    int b = is_true(v);
    unref(v);
    return mk_bool(!b);
}

static PVal *cmp_chain(const char *fn, PVal **argv, int argc, int lo, int hi) {
    PVal *e = all_numbers(fn, argv, argc);
    if (e) return e;
    for (int i = 1; i < argc; i++) {
        long long a = argv[i - 1]->u.n, b = argv[i]->u.n;
        int c = (a < b) ? -1 : (a > b) ? 1 : 0;
        if (c < lo || c > hi) return mk_bool(0);
    }
    return mk_bool(1);
}

static PVal *bi_lt(PVal **a, int n, Env *e) { (void)e; return cmp_chain("<",  a, n, -1, -1); }
static PVal *bi_le(PVal **a, int n, Env *e) { (void)e; return cmp_chain("<=", a, n, -1,  0); }
static PVal *bi_gt(PVal **a, int n, Env *e) { (void)e; return cmp_chain(">",  a, n,  1,  1); }
static PVal *bi_ge(PVal **a, int n, Env *e) { (void)e; return cmp_chain(">=", a, n,  0,  1); }

static PVal *bi_deref(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("!", 1);
    if (argv[0]->t != T_CELL) return type_error("!", 1, "cell", argv[0]);
    return ref(argv[0]->u.inner);
}

static PVal *bi_setcell(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT(":=", 2);
    if (argv[0]->t != T_CELL) return type_error(":=", 1, "cell", argv[0]);
    PVal *old = argv[0]->u.inner;
    argv[0]->u.inner = ref(argv[1]);
    unref(old);
    return ref(argv[1]);
}

static PVal *bi_cell(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("cell", 1);
    return mk_cell(ref(argv[0]));
}

static PVal *bi_chr(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("chr", 1);
    if (argv[0]->t != T_LIST) return type_error("chr", 1, "list", argv[0]);
    SB b;
    sb_init(&b);
    for (PVal *p = argv[0]; p->u.list.head; p = p->u.list.tail) {
        PVal *e = p->u.list.head;
        if (e->t != T_NUM) { sb_free(&b); return value_error("chr", e); }
        if (e->u.n < 0 || e->u.n > 255) { sb_free(&b); return value_error("chr", e); }
        sb_putc(&b, (int)e->u.n);
    }
    PVal *r = mk_strn(b.p, b.n);
    sb_free(&b);
    return r;
}

static PVal *bi_ord(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("ord", 1);
    if (argv[0]->t != T_STR) return type_error("ord", 1, "string", argv[0]);
    PVal *r = ref(NIL);
    for (size_t i = argv[0]->u.str.len; i-- > 0;)
        r = mk_cons(mk_num((unsigned char)argv[0]->u.str.s[i]), r);
    return r;
}

static PVal *bi_cons(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("cons", 2);
    if (argv[1]->t != T_LIST) return type_error("cons", 2, "list", argv[1]);
    return mk_cons(ref(argv[0]), ref(argv[1]));
}

static PVal *bi_head(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("head", 1);
    if (argv[0]->t != T_LIST) return type_error("head", 1, "list", argv[0]);
    if (is_nil(argv[0])) return value_error("head", argv[0]);
    return ref(argv[0]->u.list.head);
}

static PVal *bi_tail(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("tail", 1);
    if (argv[0]->t != T_LIST) return type_error("tail", 1, "list", argv[0]);
    if (is_nil(argv[0])) return value_error("tail", argv[0]);
    return ref(argv[0]->u.list.tail);
}

static PVal *bi_list(PVal **argv, int argc, Env *env) {
    (void)env;
    PVal *r = ref(NIL);
    for (int i = argc - 1; i >= 0; i--) r = mk_cons(ref(argv[i]), r);
    return r;
}

static PVal *bi_length(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("length", 1);
    if (argv[0]->t == T_STR) return mk_num((long long)argv[0]->u.str.len);
    if (argv[0]->t != T_LIST) return type_error("length", 1, "list", argv[0]);
    return mk_num(list_len(argv[0]));
}

static PVal *bi_not(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("not", 1);
    return mk_bool(!is_true(argv[0]));
}

static PVal *bi_type(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("type", 1);
    return mk_sym(type_name(argv[0]));
}

static PVal *bi_str(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("str", 1);
    return to_pstring(argv[0]);
}

static PVal *bi_print(PVal **argv, int argc, Env *env) {
    (void)env;
    for (int i = 0; i < argc; i++) {
        if (i) fputc(' ', stdout);
        fwrite_val(stdout, argv[i]);
    }
    fputc('\n', stdout);
    fflush(stdout);
    return mk_bool(1);
}

static PVal *bi_output(PVal **argv, int argc, Env *env) {
    (void)env;
    for (int i = 0; i < argc; i++)
        if (argv[i]->t != T_STR) return type_error("output", i + 1, "string", argv[i]);
    for (int i = 0; i < argc; i++)
        fwrite(argv[i]->u.str.s, 1, argv[i]->u.str.len, stdout);
    fflush(stdout);
    return mk_bool(1);
}

static PVal *bi_input(PVal **argv, int argc, Env *env) {
    (void)argv; (void)env;
    NEED_EXACT("input", 0);
    SB b;
    sb_init(&b);
    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n') sb_putc(&b, c);
    PVal *r = mk_strn(b.p, b.n);
    sb_free(&b);
    return r;
}

static PVal *bi_error(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("error", 1);
    return mk_err(ref(argv[0]));
}

static long long g_gensym = 137;

static PVal *bi_new_symbol(PVal **argv, int argc, Env *env) {
    (void)argv; (void)env;
    NEED_EXACT("new-symbol", 0);
    char buf[32];
    snprintf(buf, sizeof buf, "g$%lld", g_gensym++);
    return mk_sym(buf);
}

static PVal *bi_quit(PVal **argv, int argc, Env *env) {
    (void)argv; (void)argc; (void)env;
    exit(0);
}

/* Slurps a whole file; returns NULL on failure. */
static char *read_whole_file(const char *path, size_t *lenp) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    SB b;
    sb_init(&b);
    char chunk[4096];
    size_t k;
    while ((k = fread(chunk, 1, sizeof chunk, f)) > 0) sb_write(&b, chunk, k);
    fclose(f);
    *lenp = b.n;
    return b.p;
}

static PVal *bi_get_file(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("get-file", 1);
    if (argv[0]->t != T_STR) return type_error("get-file", 1, "string", argv[0]);
    size_t len;
    char *data = read_whole_file(argv[0]->u.str.s, &len);
    if (!data) return mk_err(pl(mk_sym("bad-filename"), ref(argv[0]), NULL));
    PVal *r = mk_strn(data, len);
    free(data);
    return r;
}

static PVal *bi_put_file(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("put-file", 2);
    if (argv[0]->t != T_STR) return type_error("put-file", 1, "string", argv[0]);
    if (argv[1]->t != T_STR) return type_error("put-file", 2, "string", argv[1]);
    FILE *f = fopen(argv[0]->u.str.s, "wb");
    if (!f) return mk_err(pl(mk_sym("bad-filename"), ref(argv[0]), NULL));
    fwrite(argv[1]->u.str.s, 1, argv[1]->u.str.len, f);
    fclose(f);
    return mk_bool(1);
}

static PVal *bi_eval(PVal **argv, int argc, Env *env) {
    NEED_EXACT("eval", 1);
    return eval(argv[0], env);
}

/* parse: see the "Parsing and Reading" section of the spec. */
static PVal *bi_parse(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("parse", 1);
    if (argv[0]->t != T_STR) return type_error("parse", 1, "string", argv[0]);
    const char *s = argv[0]->u.str.s;

    if (*s == 0) return mk_bool(0);                       /* (1) */

    if (isspace((unsigned char)*s)) {                     /* (2) whitespace token */
        const char *q = s;
        while (isspace((unsigned char)*q)) q++;
        return mk_str(q);
    }
    if (*s == ';') {                                      /* (2) comment token */
        const char *q = s;
        while (*q && *q != '\n') q++;
        if (*q == '\n') q++;
        return mk_str(q);
    }

    Parser ps = { s, NULL, 0, PS_OK };                       /* (3) */
    PVal *v = parse_one(&ps);
    if (!v) {
        if (ps.status == PS_CLOSER) ps.status = PS_INVALID;
        return parse_error_val(&ps);
    }
    return pl(v, mk_str(ps.p), NULL);                     /* (3a) */
}

static PVal *bi_read(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("read", 1);
    if (argv[0]->t != T_STR) return type_error("read", 1, "string", argv[0]);
    Parser ps = { argv[0]->u.str.s, NULL, 0, PS_OK };
    PVal *v = parse_one(&ps);
    if (!v) {
        if (ps.status == PS_END) return value_error("read", argv[0]);
        if (ps.status == PS_CLOSER) ps.status = PS_INVALID;
        return parse_error_val(&ps);
    }
    skip_inactive(&ps);
    if (*ps.p != 0) { unref(v); return value_error("read", argv[0]); }
    return v;
}

/* Evaluates every S-expression in `src`; returns the count, or the first
 * error thrown. */
static PVal *run_source(const char *src, const char *what) {
    Parser ps = { src, NULL, 0, PS_OK };
    long long count = 0;
    for (;;) {
        skip_inactive(&ps);
        if (*ps.p == 0) break;
        PVal *form = parse_one(&ps);
        if (!form) {
            if (ps.status == PS_END) break;
            if (ps.status == PS_CLOSER) ps.status = PS_INVALID;
            return parse_error_val(&ps);
        }
        PVal *v = eval(form, g_global);
        unref(form);
        if (is_err(v)) return v;
        unref(v);
        count++;
    }
    (void)what;
    return mk_num(count);
}

static PVal *bi_load(PVal **argv, int argc, Env *env) {
    (void)env;
    NEED_EXACT("load", 1);
    if (argv[0]->t != T_STR) return type_error("load", 1, "string", argv[0]);
    size_t len;
    char *src = read_whole_file(argv[0]->u.str.s, &len);
    if (!src) return mk_err(pl(mk_sym("bad-filename"), ref(argv[0]), NULL));
    PVal *r = run_source(src, argv[0]->u.str.s);
    free(src);
    return r;
}

typedef struct { const char *name; Builtin fn; } BuiltinDef;

static const BuiltinDef builtins[] = {
    { "+", bi_add }, { "-", bi_sub }, { "*", bi_mul }, { "/", bi_div },
    { "%", bi_mod }, { "=", bi_eq },  { "!=", bi_neq },
    { "<", bi_lt },  { "<=", bi_le }, { ">", bi_gt },  { ">=", bi_ge },
    { "!", bi_deref }, { ":=", bi_setcell }, { "cell", bi_cell },
    { "chr", bi_chr }, { "ord", bi_ord }, { "cons", bi_cons },
    { "head", bi_head }, { "tail", bi_tail }, { "list", bi_list },
    { "length", bi_length }, { "not", bi_not }, { "type", bi_type },
    { "str", bi_str }, { "print", bi_print }, { "output", bi_output },
    { "input", bi_input }, { "error", bi_error },
    { "new-symbol", bi_new_symbol }, { "quit", bi_quit },
    { "get-file", bi_get_file }, { "put-file", bi_put_file },
    { "eval", bi_eval }, { "parse", bi_parse }, { "read", bi_read },
    { "load", bi_load },
    { NULL, NULL }
};

/* ===================================================================== */
/* 9. REPL and main                                                      */
/* ===================================================================== */

static void globals_init(void) {
    NIL = alloc(T_LIST);
    TRUE_V = alloc(T_BOOL);  TRUE_V->u.b  = 1;
    FALSE_V = alloc(T_BOOL); FALSE_V->u.b = 0;
    NIL->rc = TRUE_V->rc = FALSE_V->rc = 1L << 40;   /* immortal */

    g_global = env_new(NULL);
    for (const BuiltinDef *d = builtins; d->name; d++) {
        PVal *f = alloc(T_FUN);
        f->u.fn.is_builtin = 1;
        f->u.fn.cfn   = d->fn;
        f->u.fn.cname = d->name;
        PVal *s = mk_sym(d->name);
        env_put(g_global, s, f);
        unref(s);
    }
}

static void print_result(PVal *v) {
    fwrite_val(stdout, v);
    fputc('\n', stdout);
    fflush(stdout);
}

static int read_line_into(SB *b) {
    int c, any = 0;
    while ((c = fgetc(stdin)) != EOF) {
        any = 1;
        sb_putc(b, c);
        if (c == '\n') break;
    }
    return any;
}

static void repl(void) {
    SB buf;
    sb_init(&buf);
    int tty = isatty(0);
    PVal *underscore = mk_sym("_");

    for (;;) {
        if (tty) {
            fputs(buf.n ? "...> " : "psi> ", stdout);
            fflush(stdout);
        }
        if (!read_line_into(&buf)) break;              /* EOF */

        for (;;) {
            Parser ps = { buf.p, NULL, 0, PS_OK };
            skip_inactive(&ps);
            if (*ps.p == 0) { buf.n = 0; buf.p[0] = 0; break; }

            PVal *form = parse_one(&ps);
            if (!form) {
                if (ps.status == PS_INCOMPLETE) break;  /* wait for more input */
                if (ps.status == PS_CLOSER) ps.status = PS_INVALID;
                PVal *e = parse_error_val(&ps);
                print_result(e);
                unref(e);
                buf.n = 0;
                buf.p[0] = 0;
                break;
            }
            /* consume the text we just parsed */
            size_t used = (size_t)(ps.p - buf.p);
            memmove(buf.p, buf.p + used, buf.n - used + 1);
            buf.n -= used;

            PVal *v = eval(form, g_global);
            unref(form);
            print_result(v);
            if (!is_err(v)) env_put(g_global, underscore, ref(v));
            unref(v);
        }
    }
    if (tty) fputc('\n', stdout);
    unref(underscore);
    sb_free(&buf);
}

static int run_file(const char *path, int quiet) {
    size_t len;
    char *src = read_whole_file(path, &len);
    if (!src) {
        if (!quiet) fprintf(stderr, "psi: cannot read %s\n", path);
        return quiet ? 0 : 1;
    }
    PVal *r = run_source(src, path);
    free(src);
    int bad = 0;
    if (is_err(r)) {
        fprintf(stderr, "psi: error in %s: ", path);
        fwrite_val(stderr, r);
        fputc('\n', stderr);
        bad = 1;
    }
    unref(r);
    return bad;
}

int main(int argc, char **argv) {
    globals_init();

    // run_file("__start.psi", 1);                 /* optional prelude */

    int files = 0, force_repl = 0, status = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) { force_repl = 1; continue; }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [-i] [file.psi ...]\n", argv[0]);
            return 0;
        }
        files++;
        status |= run_file(argv[i], 0);
    }
    if (!files || force_repl) repl();

    env_unref(g_global);
    return status;
}
