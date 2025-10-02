/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include "memory/vaddr.h"

#include <limits.h>
#include <regex.h>

enum {
  TK_NOTYPE = 0,
  TK_NUM,
  TK_REG,

  TK_BINARY_OP_BEGIN,
  TK_EQ,
  TK_NE,
  TK_LE,
  TK_LT,
  TK_GE,
  TK_GT,
  TK_ADD,
  TK_SUB,
  TK_MUL,
  TK_DIV,
  TK_REM,
  TK_AND,
  TK_OR,
  TK_XOR,
  TK_SHL,
  TK_LSHR,
  TK_ASHR,
  TK_LAND,
  TK_LOR,
  TK_BINARY_OP_END,

  TK_UNARY_BEGIN,
  TK_UNARY_NOT,
  TK_UNARY_LNOT,
  TK_UNARY_MINUS,
  TK_UNARY_DEREF,
  TK_UNARY_END,

  TK_LPAR,
  TK_RPAR,
};
bool is_binary_token(int i) {
  return i > TK_BINARY_OP_BEGIN && i < TK_BINARY_OP_END;
}
bool is_unary_token(int i) { return i > TK_UNARY_BEGIN && i < TK_UNARY_END; }

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /*
     * Pay attention to the precedence level of different rules.
     */

    {"[ \t\n]+", TK_NOTYPE},

    {"&&", TK_LAND},
    {"\\|\\|", TK_LOR},

    {">>>", TK_ASHR},
    {">>", TK_LSHR},
    {"<<", TK_SHL},

    {"==", TK_EQ},
    {"!=", TK_NE},
    {"<=", TK_LE},
    {">=", TK_GE},
    {"<", TK_LT},
    {">", TK_GT},

    {"!", TK_UNARY_LNOT},
    {"~", TK_UNARY_NOT},
    {"&", TK_AND},
    {"\\|", TK_OR},
    {"\\^", TK_XOR},

    {"\\+", TK_ADD},
    {"-", TK_SUB},
    {"\\*", TK_MUL},
    {"/", TK_DIV},
    {"%", TK_REM},
    {"\\(", TK_LPAR},
    {"\\)", TK_RPAR},

    {"0[xX][0-9a-fA-F]+[uU]?", TK_NUM},
    {"[0-9]+[uU]?", TK_NUM},

    {"\\$[A-Za-z0-9_]+", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[128];
} Token;

static Token tokens[128] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
        //     rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if (rules[i].token_type == TK_NOTYPE)
          break;

        Assert(nr_token < 128, "too many tokens");
        tokens[nr_token].type = rules[i].token_type;

        switch (rules[i].token_type) {
        case TK_NUM:
        case TK_REG:
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          break;
        default:
          break;
        }

        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool expecting_a_expr(int i) {
  return is_binary_token(i) || is_unary_token(i) || i == TK_LPAR;
}

static bool has_unary(int i) { return i == TK_MUL || i == TK_SUB; }
static int to_unary(int i) {
  return i == TK_MUL ? TK_UNARY_DEREF : TK_UNARY_MINUS;
}
static void match_unary_tokens() {
  for (int i = 0; i < nr_token; i++) {
    if (has_unary(tokens[i].type)) {
      if (i == 0 || expecting_a_expr(tokens[i - 1].type))
        tokens[i].type = to_unary(tokens[i].type);
    }
  }
}

static bool check_parentheses(int p, int q, bool *invalid_expr) {
  if (tokens[p].type != TK_LPAR || tokens[q].type != TK_RPAR)
    return false;

  int cnt = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LPAR)
      cnt++;
    else if (tokens[i].type == TK_RPAR) {
      --cnt;

      // Early match
      if (cnt == 0 && i != q)
        return false;

      // mismatch
      if (cnt < 0) {
        *invalid_expr = true;
        return false;
      }
    }
  }
  return cnt == 0;
}

int get_precedence(int i) {
  switch (i) {
  case TK_UNARY_NOT:
  case TK_UNARY_LNOT:
  case TK_UNARY_MINUS:
  case TK_UNARY_DEREF:
    return 100;
  case TK_MUL:
  case TK_DIV:
  case TK_REM:
    return 90;
  case TK_ADD:
  case TK_SUB:
    return 80;
  case TK_SHL:
  case TK_LSHR:
  case TK_ASHR:
    return 70;
  case TK_LE:
  case TK_GE:
  case TK_LT:
  case TK_GT:
    return 60;
  case TK_EQ:
  case TK_NE:
    return 50;
  case TK_AND:
    return 40;
  case TK_XOR:
    return 30;
  case TK_OR:
    return 20;
  case TK_LAND:
    return 10;
  case TK_LOR:
    return 0;
  default:
    panic("unexpected token");
  }
  return -1;
}

int find_dominant_operator(int p, int q, bool *success) {
  int min_prec = INT_MAX;
  int idx = -1;
  int paren = 0;
  for (int i = p; i <= q; i++) {
    int t = tokens[i].type;
    if (t == TK_LPAR) {
      ++paren;
      continue;
    }
    if (t == TK_RPAR) {
      --paren;
      continue;
    }

    if (paren > 0)
      continue;

    if (is_binary_token(t) || is_unary_token(t)) {
      int prec = get_precedence(t);
      if (prec <= min_prec) {
        min_prec = prec;
        idx = i;
      }
    }
  }

  if (idx == -1) {
    Log("find_dominant_operator: no operator found in range %d..%d", p, q);
    *success = false;
    return -1;
  }

  return idx;
}

static word_t eval(int p, int q, bool *success) {
  if (!success)
    return 0;

  if (p > q) {
    *success = false;
    return 0;
  }

  if (p == q) {
    if (tokens[p].type == TK_NUM) {
      bool base16 = tokens[p].str[0] == '0' &&
                    (tokens[p].str[1] == 'x' || tokens[p].str[1] == 'X');
      char *endptr;
      word_t ret = strtol(tokens[p].str, &endptr, base16 ? 16 : 10);
      if (tokens[p].str == endptr) {
        Log("eval: Bad number: %s", tokens[p].str);
        *success = false;
        return 0;
      }
      return ret;
    }

    if (tokens[p].type == TK_REG) {
      word_t res = isa_reg_str2val(tokens[p].str + 1, success);
      if (!*success)
        Log("eval: Bad register: %s", tokens[p].str);
      return res;
    }

    *success = false;
    Log("eval: unexpected token.");
    return 0;
  }

  bool invalid_expr = false;
  if (check_parentheses(p, q, &invalid_expr)) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  }

  if (invalid_expr) {
    *success = false;
    Log("eval: parentheses mismatch.");
    return 0;
  }

  int op = find_dominant_operator(p, q, success);
  if (!*success)
    return 0;

  if (is_unary_token(tokens[op].type)) {
    word_t val = eval(op + 1, q, success);
    if (!*success)
      return 0;
    switch (tokens[op].type) {
    case TK_UNARY_NOT:
      return ~val;
    case TK_UNARY_LNOT:
      return (word_t)(!val);
    case TK_UNARY_MINUS:
      return (word_t)(-(int64_t)val);
    case TK_UNARY_DEREF: {
      return vaddr_read(val, 4);
    }
    default:
      panic("unexpected unary operator");
    }
  }

  Assert(is_binary_token(tokens[op].type), "Bad find_dominant_operator");

  word_t val1 = eval(p, op - 1, success);

  if (!*success)
    return 0;

  word_t val2 = eval(op + 1, q, success);

  if (!*success)
    return 0;

  switch (tokens[op].type) {
#define MAKE_OP(TOK, OP)                                                       \
  case TOK:                                                                    \
    return val1 OP val2;
    MAKE_OP(TK_EQ, ==)
    MAKE_OP(TK_NE, !=)
    MAKE_OP(TK_LE, <=)
    MAKE_OP(TK_LT, <)
    MAKE_OP(TK_GE, >=)
    MAKE_OP(TK_GT, >)
    MAKE_OP(TK_ADD, +)
    MAKE_OP(TK_SUB, -)
    MAKE_OP(TK_MUL, *)
    MAKE_OP(TK_AND, &)
    MAKE_OP(TK_OR, |)
    MAKE_OP(TK_XOR, ^)
    MAKE_OP(TK_SHL, <<)
    MAKE_OP(TK_LSHR, >>)
    MAKE_OP(TK_LAND, &&)
    MAKE_OP(TK_LOR, ||)
#undef MAKE_OP
  case TK_ASHR:
    return (uint32_t)((int32_t)val1 >> val2);
  case TK_DIV:
    if (val2 == 0) {
      *success = false;
      return 0;
    }
    return val1 / val2;
  case TK_REM:
    if (val2 == 0) {
      *success = false;
      return 0;
    }
    return val1 % val2;
  default:
    panic("unexpected binary operator");
  }
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  match_unary_tokens();

  *success = true;
  return eval(0, nr_token - 1, success);
}
