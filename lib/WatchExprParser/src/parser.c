#include "tree_sitter/parser.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define LANGUAGE_VERSION 14
#define STATE_COUNT 63
#define LARGE_STATE_COUNT 2
#define SYMBOL_COUNT 43
#define ALIAS_COUNT 0
#define TOKEN_COUNT 21
#define EXTERNAL_TOKEN_COUNT 0
#define FIELD_COUNT 1
#define MAX_ALIAS_SEQUENCE_LENGTH 4
#define PRODUCTION_ID_COUNT 3

enum ts_symbol_identifiers {
  anon_sym_LPAREN = 1,
  anon_sym_RPAREN = 2,
  anon_sym_LBRACK = 3,
  anon_sym_RBRACK = 4,
  anon_sym_LBRACE = 5,
  anon_sym_RBRACE = 6,
  anon_sym_STAR = 7,
  anon_sym_DASH = 8,
  anon_sym_sizeof = 9,
  sym_sym_dotmember = 10,
  sym_sym_ptrmember = 11,
  sym_sym_takeaddr = 12,
  sym_sym_plus = 13,
  sym_sym_namespace = 14,
  sym_ident = 15,
  anon_sym_0x = 16,
  anon_sym_0b = 17,
  sym_num_dec = 18,
  sym_num_hex = 19,
  sym_num_bin = 20,
  sym_source_file = 21,
  sym_expr = 22,
  sym_offset_expr = 23,
  sym_cast_expr = 24,
  sym_unary_expr = 25,
  sym_postfix_expr = 26,
  sym_postfix_comp = 27,
  sym_pri_expr = 28,
  sym_single_eval_block = 29,
  sym_det_expr = 30,
  sym_mul_det_expr = 31,
  sym_unary_det_expr = 32,
  sym_pri_det_expr = 33,
  sym_sizeof_expr = 34,
  sym_type_ident = 35,
  sym_scoped_ident = 36,
  sym_array_cap = 37,
  sym_sym_offset = 38,
  sym_sym_unary = 39,
  sym_sym_minus = 40,
  sym_sym_deref = 41,
  sym_num_lit = 42,
};

static const char * const ts_symbol_names[] = {
  [ts_builtin_sym_end] = "end",
  [anon_sym_LPAREN] = "(",
  [anon_sym_RPAREN] = ")",
  [anon_sym_LBRACK] = "[",
  [anon_sym_RBRACK] = "]",
  [anon_sym_LBRACE] = "{",
  [anon_sym_RBRACE] = "}",
  [anon_sym_STAR] = "*",
  [anon_sym_DASH] = "-",
  [anon_sym_sizeof] = "sizeof",
  [sym_sym_dotmember] = "sym_dotmember",
  [sym_sym_ptrmember] = "sym_ptrmember",
  [sym_sym_takeaddr] = "sym_takeaddr",
  [sym_sym_plus] = "sym_plus",
  [sym_sym_namespace] = "sym_namespace",
  [sym_ident] = "ident",
  [anon_sym_0x] = "0x",
  [anon_sym_0b] = "0b",
  [sym_num_dec] = "num_dec",
  [sym_num_hex] = "num_hex",
  [sym_num_bin] = "num_bin",
  [sym_source_file] = "source_file",
  [sym_expr] = "expr",
  [sym_offset_expr] = "offset_expr",
  [sym_cast_expr] = "cast_expr",
  [sym_unary_expr] = "unary_expr",
  [sym_postfix_expr] = "postfix_expr",
  [sym_postfix_comp] = "postfix_comp",
  [sym_pri_expr] = "pri_expr",
  [sym_single_eval_block] = "single_eval_block",
  [sym_det_expr] = "det_expr",
  [sym_mul_det_expr] = "mul_det_expr",
  [sym_unary_det_expr] = "unary_det_expr",
  [sym_pri_det_expr] = "pri_det_expr",
  [sym_sizeof_expr] = "sizeof_expr",
  [sym_type_ident] = "type_ident",
  [sym_scoped_ident] = "scoped_ident",
  [sym_array_cap] = "array_cap",
  [sym_sym_offset] = "sym_offset",
  [sym_sym_unary] = "sym_unary",
  [sym_sym_minus] = "sym_minus",
  [sym_sym_deref] = "sym_deref",
  [sym_num_lit] = "num_lit",
};

static const TSSymbol ts_symbol_map[] = {
  [ts_builtin_sym_end] = ts_builtin_sym_end,
  [anon_sym_LPAREN] = anon_sym_LPAREN,
  [anon_sym_RPAREN] = anon_sym_RPAREN,
  [anon_sym_LBRACK] = anon_sym_LBRACK,
  [anon_sym_RBRACK] = anon_sym_RBRACK,
  [anon_sym_LBRACE] = anon_sym_LBRACE,
  [anon_sym_RBRACE] = anon_sym_RBRACE,
  [anon_sym_STAR] = anon_sym_STAR,
  [anon_sym_DASH] = anon_sym_DASH,
  [anon_sym_sizeof] = anon_sym_sizeof,
  [sym_sym_dotmember] = sym_sym_dotmember,
  [sym_sym_ptrmember] = sym_sym_ptrmember,
  [sym_sym_takeaddr] = sym_sym_takeaddr,
  [sym_sym_plus] = sym_sym_plus,
  [sym_sym_namespace] = sym_sym_namespace,
  [sym_ident] = sym_ident,
  [anon_sym_0x] = anon_sym_0x,
  [anon_sym_0b] = anon_sym_0b,
  [sym_num_dec] = sym_num_dec,
  [sym_num_hex] = sym_num_hex,
  [sym_num_bin] = sym_num_bin,
  [sym_source_file] = sym_source_file,
  [sym_expr] = sym_expr,
  [sym_offset_expr] = sym_offset_expr,
  [sym_cast_expr] = sym_cast_expr,
  [sym_unary_expr] = sym_unary_expr,
  [sym_postfix_expr] = sym_postfix_expr,
  [sym_postfix_comp] = sym_postfix_comp,
  [sym_pri_expr] = sym_pri_expr,
  [sym_single_eval_block] = sym_single_eval_block,
  [sym_det_expr] = sym_det_expr,
  [sym_mul_det_expr] = sym_mul_det_expr,
  [sym_unary_det_expr] = sym_unary_det_expr,
  [sym_pri_det_expr] = sym_pri_det_expr,
  [sym_sizeof_expr] = sym_sizeof_expr,
  [sym_type_ident] = sym_type_ident,
  [sym_scoped_ident] = sym_scoped_ident,
  [sym_array_cap] = sym_array_cap,
  [sym_sym_offset] = sym_sym_offset,
  [sym_sym_unary] = sym_sym_unary,
  [sym_sym_minus] = sym_sym_minus,
  [sym_sym_deref] = sym_sym_deref,
  [sym_num_lit] = sym_num_lit,
};

static const TSSymbolMetadata ts_symbol_metadata[] = {
  [ts_builtin_sym_end] = {
    .visible = false,
    .named = true,
  },
  [anon_sym_LPAREN] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_RPAREN] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LBRACK] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_RBRACK] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LBRACE] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_RBRACE] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_STAR] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_DASH] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_sizeof] = {
    .visible = true,
    .named = false,
  },
  [sym_sym_dotmember] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_ptrmember] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_takeaddr] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_plus] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_namespace] = {
    .visible = true,
    .named = true,
  },
  [sym_ident] = {
    .visible = true,
    .named = true,
  },
  [anon_sym_0x] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_0b] = {
    .visible = true,
    .named = false,
  },
  [sym_num_dec] = {
    .visible = true,
    .named = true,
  },
  [sym_num_hex] = {
    .visible = true,
    .named = true,
  },
  [sym_num_bin] = {
    .visible = true,
    .named = true,
  },
  [sym_source_file] = {
    .visible = true,
    .named = true,
  },
  [sym_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_offset_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_cast_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_unary_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_postfix_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_postfix_comp] = {
    .visible = true,
    .named = true,
  },
  [sym_pri_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_single_eval_block] = {
    .visible = true,
    .named = true,
  },
  [sym_det_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_mul_det_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_unary_det_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_pri_det_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_sizeof_expr] = {
    .visible = true,
    .named = true,
  },
  [sym_type_ident] = {
    .visible = true,
    .named = true,
  },
  [sym_scoped_ident] = {
    .visible = true,
    .named = true,
  },
  [sym_array_cap] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_offset] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_unary] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_minus] = {
    .visible = true,
    .named = true,
  },
  [sym_sym_deref] = {
    .visible = true,
    .named = true,
  },
  [sym_num_lit] = {
    .visible = true,
    .named = true,
  },
};

enum ts_field_identifiers {
  field_type_ident = 1,
};

static const char * const ts_field_names[] = {
  [0] = NULL,
  [field_type_ident] = "type_ident",
};

static const TSFieldMapSlice ts_field_map_slices[PRODUCTION_ID_COUNT] = {
  [1] = {.index = 0, .length = 1},
  [2] = {.index = 1, .length = 1},
};

static const TSFieldMapEntry ts_field_map_entries[] = {
  [0] =
    {field_type_ident, 1},
  [1] =
    {field_type_ident, 2},
};

static const TSSymbol ts_alias_sequences[PRODUCTION_ID_COUNT][MAX_ALIAS_SEQUENCE_LENGTH] = {
  [0] = {0},
};

static const uint16_t ts_non_terminal_alias_map[] = {
  0,
};

static const TSStateId ts_primary_state_ids[STATE_COUNT] = {
  [0] = 0,
  [1] = 1,
  [2] = 2,
  [3] = 3,
  [4] = 4,
  [5] = 5,
  [6] = 6,
  [7] = 7,
  [8] = 8,
  [9] = 9,
  [10] = 10,
  [11] = 11,
  [12] = 12,
  [13] = 13,
  [14] = 14,
  [15] = 15,
  [16] = 16,
  [17] = 17,
  [18] = 18,
  [19] = 19,
  [20] = 20,
  [21] = 21,
  [22] = 22,
  [23] = 23,
  [24] = 24,
  [25] = 25,
  [26] = 26,
  [27] = 27,
  [28] = 28,
  [29] = 29,
  [30] = 30,
  [31] = 31,
  [32] = 32,
  [33] = 33,
  [34] = 34,
  [35] = 35,
  [36] = 36,
  [37] = 37,
  [38] = 38,
  [39] = 39,
  [40] = 40,
  [41] = 41,
  [42] = 42,
  [43] = 43,
  [44] = 44,
  [45] = 45,
  [46] = 46,
  [47] = 47,
  [48] = 48,
  [49] = 49,
  [50] = 50,
  [51] = 51,
  [52] = 52,
  [53] = 53,
  [54] = 54,
  [55] = 55,
  [56] = 56,
  [57] = 57,
  [58] = 58,
  [59] = 59,
  [60] = 60,
  [61] = 61,
  [62] = 62,
};

static bool ts_lex(TSLexer *lexer, TSStateId state) {
  START_LEXER();
  eof = lexer->eof(lexer);
  switch (state) {
    case 0:
      if (eof) ADVANCE(11);
      ADVANCE_MAP(
        '&', 25,
        '(', 12,
        ')', 13,
        '*', 18,
        '+', 26,
        '-', 20,
        '.', 23,
        '0', 38,
        '1', 40,
        ':', 3,
        '[', 14,
        ']', 15,
        '_', 33,
        's', 30,
        '{', 16,
        '}', 17,
      );
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(0);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(41);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 1:
      if (lookahead == '&') ADVANCE(25);
      if (lookahead == '(') ADVANCE(12);
      if (lookahead == ')') ADVANCE(13);
      if (lookahead == '*') ADVANCE(18);
      if (lookahead == '[') ADVANCE(14);
      if (lookahead == '{') ADVANCE(16);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(1);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 2:
      if (lookahead == '(') ADVANCE(12);
      if (lookahead == '-') ADVANCE(19);
      if (lookahead == '0') ADVANCE(39);
      if (lookahead == 's') ADVANCE(6);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(2);
      if (('1' <= lookahead && lookahead <= '9') ||
          lookahead == '_') ADVANCE(41);
      END_STATE();
    case 3:
      if (lookahead == ':') ADVANCE(27);
      END_STATE();
    case 4:
      if (lookahead == 'e') ADVANCE(7);
      END_STATE();
    case 5:
      if (lookahead == 'f') ADVANCE(21);
      END_STATE();
    case 6:
      if (lookahead == 'i') ADVANCE(8);
      END_STATE();
    case 7:
      if (lookahead == 'o') ADVANCE(5);
      END_STATE();
    case 8:
      if (lookahead == 'z') ADVANCE(4);
      END_STATE();
    case 9:
      if (lookahead == '0' ||
          lookahead == '1' ||
          lookahead == '_') ADVANCE(43);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(9);
      END_STATE();
    case 10:
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(10);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'F') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'f')) ADVANCE(42);
      END_STATE();
    case 11:
      ACCEPT_TOKEN(ts_builtin_sym_end);
      END_STATE();
    case 12:
      ACCEPT_TOKEN(anon_sym_LPAREN);
      END_STATE();
    case 13:
      ACCEPT_TOKEN(anon_sym_RPAREN);
      END_STATE();
    case 14:
      ACCEPT_TOKEN(anon_sym_LBRACK);
      END_STATE();
    case 15:
      ACCEPT_TOKEN(anon_sym_RBRACK);
      END_STATE();
    case 16:
      ACCEPT_TOKEN(anon_sym_LBRACE);
      END_STATE();
    case 17:
      ACCEPT_TOKEN(anon_sym_RBRACE);
      END_STATE();
    case 18:
      ACCEPT_TOKEN(anon_sym_STAR);
      END_STATE();
    case 19:
      ACCEPT_TOKEN(anon_sym_DASH);
      END_STATE();
    case 20:
      ACCEPT_TOKEN(anon_sym_DASH);
      if (lookahead == '>') ADVANCE(24);
      END_STATE();
    case 21:
      ACCEPT_TOKEN(anon_sym_sizeof);
      END_STATE();
    case 22:
      ACCEPT_TOKEN(anon_sym_sizeof);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 23:
      ACCEPT_TOKEN(sym_sym_dotmember);
      END_STATE();
    case 24:
      ACCEPT_TOKEN(sym_sym_ptrmember);
      END_STATE();
    case 25:
      ACCEPT_TOKEN(sym_sym_takeaddr);
      END_STATE();
    case 26:
      ACCEPT_TOKEN(sym_sym_plus);
      END_STATE();
    case 27:
      ACCEPT_TOKEN(sym_sym_namespace);
      END_STATE();
    case 28:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'e') ADVANCE(31);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 29:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'f') ADVANCE(22);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 30:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'i') ADVANCE(32);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 31:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'o') ADVANCE(29);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 32:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'z') ADVANCE(28);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'y')) ADVANCE(35);
      END_STATE();
    case 33:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == '0' ||
          lookahead == '1' ||
          lookahead == '_') ADVANCE(33);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(34);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 34:
      ACCEPT_TOKEN(sym_ident);
      if (('0' <= lookahead && lookahead <= '9') ||
          lookahead == '_') ADVANCE(34);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 35:
      ACCEPT_TOKEN(sym_ident);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(35);
      END_STATE();
    case 36:
      ACCEPT_TOKEN(anon_sym_0x);
      END_STATE();
    case 37:
      ACCEPT_TOKEN(anon_sym_0b);
      END_STATE();
    case 38:
      ACCEPT_TOKEN(sym_num_dec);
      if (lookahead == 'b') ADVANCE(37);
      if (lookahead == 'x') ADVANCE(36);
      if (lookahead == '0' ||
          lookahead == '1' ||
          lookahead == '_') ADVANCE(40);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(41);
      END_STATE();
    case 39:
      ACCEPT_TOKEN(sym_num_dec);
      if (lookahead == 'b') ADVANCE(37);
      if (lookahead == 'x') ADVANCE(36);
      if (('0' <= lookahead && lookahead <= '9') ||
          lookahead == '_') ADVANCE(41);
      END_STATE();
    case 40:
      ACCEPT_TOKEN(sym_num_dec);
      if (lookahead == '0' ||
          lookahead == '1' ||
          lookahead == '_') ADVANCE(40);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(41);
      END_STATE();
    case 41:
      ACCEPT_TOKEN(sym_num_dec);
      if (('0' <= lookahead && lookahead <= '9') ||
          lookahead == '_') ADVANCE(41);
      END_STATE();
    case 42:
      ACCEPT_TOKEN(sym_num_hex);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'F') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'f')) ADVANCE(42);
      END_STATE();
    case 43:
      ACCEPT_TOKEN(sym_num_bin);
      if (lookahead == '0' ||
          lookahead == '1' ||
          lookahead == '_') ADVANCE(43);
      END_STATE();
    default:
      return false;
  }
}

static const TSLexMode ts_lex_modes[STATE_COUNT] = {
  [0] = {.lex_state = 0},
  [1] = {.lex_state = 1},
  [2] = {.lex_state = 1},
  [3] = {.lex_state = 1},
  [4] = {.lex_state = 1},
  [5] = {.lex_state = 1},
  [6] = {.lex_state = 2},
  [7] = {.lex_state = 2},
  [8] = {.lex_state = 2},
  [9] = {.lex_state = 2},
  [10] = {.lex_state = 2},
  [11] = {.lex_state = 0},
  [12] = {.lex_state = 0},
  [13] = {.lex_state = 0},
  [14] = {.lex_state = 0},
  [15] = {.lex_state = 0},
  [16] = {.lex_state = 0},
  [17] = {.lex_state = 0},
  [18] = {.lex_state = 0},
  [19] = {.lex_state = 0},
  [20] = {.lex_state = 0},
  [21] = {.lex_state = 2},
  [22] = {.lex_state = 0},
  [23] = {.lex_state = 0},
  [24] = {.lex_state = 0},
  [25] = {.lex_state = 0},
  [26] = {.lex_state = 0},
  [27] = {.lex_state = 0},
  [28] = {.lex_state = 0},
  [29] = {.lex_state = 0},
  [30] = {.lex_state = 1},
  [31] = {.lex_state = 0},
  [32] = {.lex_state = 0},
  [33] = {.lex_state = 0},
  [34] = {.lex_state = 0},
  [35] = {.lex_state = 0},
  [36] = {.lex_state = 0},
  [37] = {.lex_state = 2},
  [38] = {.lex_state = 0},
  [39] = {.lex_state = 2},
  [40] = {.lex_state = 1},
  [41] = {.lex_state = 0},
  [42] = {.lex_state = 0},
  [43] = {.lex_state = 0},
  [44] = {.lex_state = 0},
  [45] = {.lex_state = 0},
  [46] = {.lex_state = 0},
  [47] = {.lex_state = 0},
  [48] = {.lex_state = 0},
  [49] = {.lex_state = 0},
  [50] = {.lex_state = 0},
  [51] = {.lex_state = 1},
  [52] = {.lex_state = 0},
  [53] = {.lex_state = 0},
  [54] = {.lex_state = 0},
  [55] = {.lex_state = 0},
  [56] = {.lex_state = 9},
  [57] = {.lex_state = 10},
  [58] = {.lex_state = 0},
  [59] = {.lex_state = 0},
  [60] = {.lex_state = 1},
  [61] = {.lex_state = 1},
  [62] = {.lex_state = 0},
};

static const uint16_t ts_parse_table[LARGE_STATE_COUNT][SYMBOL_COUNT] = {
  [0] = {
    [ts_builtin_sym_end] = ACTIONS(1),
    [anon_sym_LPAREN] = ACTIONS(1),
    [anon_sym_RPAREN] = ACTIONS(1),
    [anon_sym_LBRACK] = ACTIONS(1),
    [anon_sym_RBRACK] = ACTIONS(1),
    [anon_sym_LBRACE] = ACTIONS(1),
    [anon_sym_RBRACE] = ACTIONS(1),
    [anon_sym_STAR] = ACTIONS(1),
    [anon_sym_DASH] = ACTIONS(1),
    [anon_sym_sizeof] = ACTIONS(1),
    [sym_sym_dotmember] = ACTIONS(1),
    [sym_sym_ptrmember] = ACTIONS(1),
    [sym_sym_takeaddr] = ACTIONS(1),
    [sym_sym_plus] = ACTIONS(1),
    [sym_sym_namespace] = ACTIONS(1),
    [sym_ident] = ACTIONS(1),
    [anon_sym_0x] = ACTIONS(1),
    [anon_sym_0b] = ACTIONS(1),
    [sym_num_dec] = ACTIONS(1),
    [sym_num_bin] = ACTIONS(1),
  },
  [1] = {
    [sym_source_file] = STATE(55),
    [sym_expr] = STATE(53),
    [sym_offset_expr] = STATE(34),
    [sym_cast_expr] = STATE(46),
    [sym_unary_expr] = STATE(41),
    [sym_postfix_expr] = STATE(14),
    [sym_pri_expr] = STATE(18),
    [sym_single_eval_block] = STATE(19),
    [sym_scoped_ident] = STATE(13),
    [sym_sym_unary] = STATE(5),
    [sym_sym_deref] = STATE(40),
    [anon_sym_LPAREN] = ACTIONS(3),
    [anon_sym_LBRACE] = ACTIONS(5),
    [anon_sym_STAR] = ACTIONS(7),
    [sym_sym_takeaddr] = ACTIONS(9),
    [sym_ident] = ACTIONS(11),
  },
};

static const uint16_t ts_small_parse_table[] = {
  [0] = 16,
    ACTIONS(3), 1,
      anon_sym_LPAREN,
    ACTIONS(5), 1,
      anon_sym_LBRACE,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(9), 1,
      sym_sym_takeaddr,
    ACTIONS(11), 1,
      sym_ident,
    STATE(5), 1,
      sym_sym_unary,
    STATE(14), 1,
      sym_postfix_expr,
    STATE(18), 1,
      sym_pri_expr,
    STATE(19), 1,
      sym_single_eval_block,
    STATE(22), 1,
      sym_scoped_ident,
    STATE(34), 1,
      sym_offset_expr,
    STATE(40), 1,
      sym_sym_deref,
    STATE(41), 1,
      sym_unary_expr,
    STATE(44), 1,
      sym_type_ident,
    STATE(46), 1,
      sym_cast_expr,
    STATE(54), 1,
      sym_expr,
  [49] = 15,
    ACTIONS(3), 1,
      anon_sym_LPAREN,
    ACTIONS(5), 1,
      anon_sym_LBRACE,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(9), 1,
      sym_sym_takeaddr,
    ACTIONS(11), 1,
      sym_ident,
    STATE(5), 1,
      sym_sym_unary,
    STATE(13), 1,
      sym_scoped_ident,
    STATE(14), 1,
      sym_postfix_expr,
    STATE(18), 1,
      sym_pri_expr,
    STATE(19), 1,
      sym_single_eval_block,
    STATE(34), 1,
      sym_offset_expr,
    STATE(40), 1,
      sym_sym_deref,
    STATE(41), 1,
      sym_unary_expr,
    STATE(46), 1,
      sym_cast_expr,
    STATE(62), 1,
      sym_expr,
  [95] = 13,
    ACTIONS(3), 1,
      anon_sym_LPAREN,
    ACTIONS(5), 1,
      anon_sym_LBRACE,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(9), 1,
      sym_sym_takeaddr,
    ACTIONS(11), 1,
      sym_ident,
    STATE(5), 1,
      sym_sym_unary,
    STATE(13), 1,
      sym_scoped_ident,
    STATE(14), 1,
      sym_postfix_expr,
    STATE(18), 1,
      sym_pri_expr,
    STATE(19), 1,
      sym_single_eval_block,
    STATE(40), 1,
      sym_sym_deref,
    STATE(41), 1,
      sym_unary_expr,
    STATE(45), 1,
      sym_cast_expr,
  [135] = 13,
    ACTIONS(3), 1,
      anon_sym_LPAREN,
    ACTIONS(5), 1,
      anon_sym_LBRACE,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(9), 1,
      sym_sym_takeaddr,
    ACTIONS(11), 1,
      sym_ident,
    STATE(5), 1,
      sym_sym_unary,
    STATE(13), 1,
      sym_scoped_ident,
    STATE(14), 1,
      sym_postfix_expr,
    STATE(18), 1,
      sym_pri_expr,
    STATE(19), 1,
      sym_single_eval_block,
    STATE(40), 1,
      sym_sym_deref,
    STATE(41), 1,
      sym_unary_expr,
    STATE(43), 1,
      sym_cast_expr,
  [175] = 11,
    ACTIONS(13), 1,
      anon_sym_LPAREN,
    ACTIONS(15), 1,
      anon_sym_DASH,
    ACTIONS(17), 1,
      anon_sym_sizeof,
    ACTIONS(19), 1,
      anon_sym_0x,
    ACTIONS(21), 1,
      anon_sym_0b,
    ACTIONS(23), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_pri_det_expr,
    STATE(28), 1,
      sym_unary_det_expr,
    STATE(29), 1,
      sym_mul_det_expr,
    STATE(49), 1,
      sym_det_expr,
    STATE(36), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [210] = 11,
    ACTIONS(13), 1,
      anon_sym_LPAREN,
    ACTIONS(15), 1,
      anon_sym_DASH,
    ACTIONS(17), 1,
      anon_sym_sizeof,
    ACTIONS(19), 1,
      anon_sym_0x,
    ACTIONS(21), 1,
      anon_sym_0b,
    ACTIONS(23), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_pri_det_expr,
    STATE(28), 1,
      sym_unary_det_expr,
    STATE(29), 1,
      sym_mul_det_expr,
    STATE(38), 1,
      sym_det_expr,
    STATE(36), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [245] = 11,
    ACTIONS(13), 1,
      anon_sym_LPAREN,
    ACTIONS(15), 1,
      anon_sym_DASH,
    ACTIONS(17), 1,
      anon_sym_sizeof,
    ACTIONS(19), 1,
      anon_sym_0x,
    ACTIONS(21), 1,
      anon_sym_0b,
    ACTIONS(23), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_pri_det_expr,
    STATE(28), 1,
      sym_unary_det_expr,
    STATE(29), 1,
      sym_mul_det_expr,
    STATE(48), 1,
      sym_det_expr,
    STATE(36), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [280] = 10,
    ACTIONS(13), 1,
      anon_sym_LPAREN,
    ACTIONS(15), 1,
      anon_sym_DASH,
    ACTIONS(17), 1,
      anon_sym_sizeof,
    ACTIONS(19), 1,
      anon_sym_0x,
    ACTIONS(21), 1,
      anon_sym_0b,
    ACTIONS(23), 1,
      sym_num_dec,
    STATE(24), 1,
      sym_mul_det_expr,
    STATE(26), 1,
      sym_pri_det_expr,
    STATE(28), 1,
      sym_unary_det_expr,
    STATE(36), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [312] = 9,
    ACTIONS(13), 1,
      anon_sym_LPAREN,
    ACTIONS(15), 1,
      anon_sym_DASH,
    ACTIONS(17), 1,
      anon_sym_sizeof,
    ACTIONS(19), 1,
      anon_sym_0x,
    ACTIONS(21), 1,
      anon_sym_0b,
    ACTIONS(23), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_pri_det_expr,
    STATE(35), 1,
      sym_unary_det_expr,
    STATE(36), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [341] = 2,
    ACTIONS(27), 1,
      anon_sym_DASH,
    ACTIONS(25), 9,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
      sym_sym_namespace,
  [356] = 2,
    ACTIONS(31), 1,
      anon_sym_DASH,
    ACTIONS(29), 9,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
      sym_sym_namespace,
  [371] = 3,
    ACTIONS(35), 1,
      anon_sym_DASH,
    ACTIONS(37), 1,
      sym_sym_namespace,
    ACTIONS(33), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [387] = 5,
    ACTIONS(41), 1,
      anon_sym_LBRACK,
    ACTIONS(43), 1,
      anon_sym_DASH,
    STATE(16), 1,
      sym_postfix_comp,
    ACTIONS(45), 2,
      sym_sym_dotmember,
      sym_sym_ptrmember,
    ACTIONS(39), 4,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      sym_sym_plus,
  [407] = 2,
    ACTIONS(49), 1,
      anon_sym_DASH,
    ACTIONS(47), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [420] = 2,
    ACTIONS(53), 1,
      anon_sym_DASH,
    ACTIONS(51), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [433] = 2,
    ACTIONS(57), 1,
      anon_sym_DASH,
    ACTIONS(55), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [446] = 2,
    ACTIONS(61), 1,
      anon_sym_DASH,
    ACTIONS(59), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [459] = 2,
    ACTIONS(35), 1,
      anon_sym_DASH,
    ACTIONS(33), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [472] = 2,
    ACTIONS(65), 1,
      anon_sym_DASH,
    ACTIONS(63), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [485] = 7,
    ACTIONS(13), 1,
      anon_sym_LPAREN,
    ACTIONS(17), 1,
      anon_sym_sizeof,
    ACTIONS(19), 1,
      anon_sym_0x,
    ACTIONS(21), 1,
      anon_sym_0b,
    ACTIONS(23), 1,
      sym_num_dec,
    STATE(25), 1,
      sym_pri_det_expr,
    STATE(36), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [508] = 4,
    ACTIONS(35), 1,
      anon_sym_DASH,
    ACTIONS(37), 1,
      sym_sym_namespace,
    ACTIONS(67), 1,
      anon_sym_STAR,
    ACTIONS(33), 5,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [525] = 2,
    ACTIONS(71), 1,
      anon_sym_DASH,
    ACTIONS(69), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [538] = 2,
    ACTIONS(75), 1,
      anon_sym_STAR,
    ACTIONS(73), 6,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [550] = 1,
    ACTIONS(77), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [560] = 1,
    ACTIONS(79), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [570] = 1,
    ACTIONS(81), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [580] = 1,
    ACTIONS(83), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [590] = 2,
    ACTIONS(75), 1,
      anon_sym_STAR,
    ACTIONS(85), 6,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [602] = 1,
    ACTIONS(87), 7,
      anon_sym_LPAREN,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_LBRACE,
      anon_sym_STAR,
      sym_sym_takeaddr,
      sym_ident,
  [612] = 1,
    ACTIONS(89), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [622] = 1,
    ACTIONS(91), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [632] = 1,
    ACTIONS(93), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [642] = 5,
    ACTIONS(97), 1,
      anon_sym_DASH,
    ACTIONS(99), 1,
      sym_sym_plus,
    STATE(7), 1,
      sym_sym_offset,
    STATE(39), 1,
      sym_sym_minus,
    ACTIONS(95), 3,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
  [660] = 1,
    ACTIONS(101), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [670] = 1,
    ACTIONS(103), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [680] = 2,
    ACTIONS(107), 1,
      sym_num_dec,
    ACTIONS(105), 5,
      anon_sym_LPAREN,
      anon_sym_DASH,
      anon_sym_sizeof,
      anon_sym_0x,
      anon_sym_0b,
  [691] = 2,
    STATE(9), 1,
      sym_sym_minus,
    ACTIONS(109), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [702] = 2,
    ACTIONS(113), 1,
      sym_num_dec,
    ACTIONS(111), 5,
      anon_sym_LPAREN,
      anon_sym_DASH,
      anon_sym_sizeof,
      anon_sym_0x,
      anon_sym_0b,
  [713] = 1,
    ACTIONS(115), 5,
      anon_sym_LPAREN,
      anon_sym_LBRACE,
      anon_sym_STAR,
      sym_sym_takeaddr,
      sym_ident,
  [721] = 1,
    ACTIONS(117), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [729] = 4,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(119), 1,
      anon_sym_RPAREN,
    ACTIONS(121), 1,
      anon_sym_LBRACK,
    STATE(52), 2,
      sym_array_cap,
      sym_sym_deref,
  [743] = 1,
    ACTIONS(123), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [751] = 4,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(121), 1,
      anon_sym_LBRACK,
    ACTIONS(125), 1,
      anon_sym_RPAREN,
    STATE(52), 2,
      sym_array_cap,
      sym_sym_deref,
  [765] = 1,
    ACTIONS(127), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [773] = 1,
    ACTIONS(129), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [781] = 2,
    ACTIONS(37), 1,
      sym_sym_namespace,
    ACTIONS(67), 3,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
  [790] = 4,
    ACTIONS(97), 1,
      anon_sym_DASH,
    ACTIONS(131), 1,
      anon_sym_RBRACK,
    ACTIONS(133), 1,
      sym_sym_plus,
    STATE(9), 1,
      sym_sym_minus,
  [803] = 4,
    ACTIONS(97), 1,
      anon_sym_DASH,
    ACTIONS(133), 1,
      sym_sym_plus,
    ACTIONS(135), 1,
      anon_sym_RPAREN,
    STATE(9), 1,
      sym_sym_minus,
  [816] = 1,
    ACTIONS(137), 3,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
  [822] = 3,
    ACTIONS(11), 1,
      sym_ident,
    STATE(42), 1,
      sym_type_ident,
    STATE(47), 1,
      sym_scoped_ident,
  [832] = 1,
    ACTIONS(139), 3,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
  [838] = 1,
    ACTIONS(141), 1,
      ts_builtin_sym_end,
  [842] = 1,
    ACTIONS(143), 1,
      anon_sym_RPAREN,
  [846] = 1,
    ACTIONS(145), 1,
      ts_builtin_sym_end,
  [850] = 1,
    ACTIONS(147), 1,
      sym_num_bin,
  [854] = 1,
    ACTIONS(147), 1,
      sym_num_hex,
  [858] = 1,
    ACTIONS(149), 1,
      anon_sym_LPAREN,
  [862] = 1,
    ACTIONS(151), 1,
      anon_sym_RBRACK,
  [866] = 1,
    ACTIONS(153), 1,
      sym_ident,
  [870] = 1,
    ACTIONS(155), 1,
      sym_ident,
  [874] = 1,
    ACTIONS(157), 1,
      anon_sym_RBRACE,
};

static const uint32_t ts_small_parse_table_map[] = {
  [SMALL_STATE(2)] = 0,
  [SMALL_STATE(3)] = 49,
  [SMALL_STATE(4)] = 95,
  [SMALL_STATE(5)] = 135,
  [SMALL_STATE(6)] = 175,
  [SMALL_STATE(7)] = 210,
  [SMALL_STATE(8)] = 245,
  [SMALL_STATE(9)] = 280,
  [SMALL_STATE(10)] = 312,
  [SMALL_STATE(11)] = 341,
  [SMALL_STATE(12)] = 356,
  [SMALL_STATE(13)] = 371,
  [SMALL_STATE(14)] = 387,
  [SMALL_STATE(15)] = 407,
  [SMALL_STATE(16)] = 420,
  [SMALL_STATE(17)] = 433,
  [SMALL_STATE(18)] = 446,
  [SMALL_STATE(19)] = 459,
  [SMALL_STATE(20)] = 472,
  [SMALL_STATE(21)] = 485,
  [SMALL_STATE(22)] = 508,
  [SMALL_STATE(23)] = 525,
  [SMALL_STATE(24)] = 538,
  [SMALL_STATE(25)] = 550,
  [SMALL_STATE(26)] = 560,
  [SMALL_STATE(27)] = 570,
  [SMALL_STATE(28)] = 580,
  [SMALL_STATE(29)] = 590,
  [SMALL_STATE(30)] = 602,
  [SMALL_STATE(31)] = 612,
  [SMALL_STATE(32)] = 622,
  [SMALL_STATE(33)] = 632,
  [SMALL_STATE(34)] = 642,
  [SMALL_STATE(35)] = 660,
  [SMALL_STATE(36)] = 670,
  [SMALL_STATE(37)] = 680,
  [SMALL_STATE(38)] = 691,
  [SMALL_STATE(39)] = 702,
  [SMALL_STATE(40)] = 713,
  [SMALL_STATE(41)] = 721,
  [SMALL_STATE(42)] = 729,
  [SMALL_STATE(43)] = 743,
  [SMALL_STATE(44)] = 751,
  [SMALL_STATE(45)] = 765,
  [SMALL_STATE(46)] = 773,
  [SMALL_STATE(47)] = 781,
  [SMALL_STATE(48)] = 790,
  [SMALL_STATE(49)] = 803,
  [SMALL_STATE(50)] = 816,
  [SMALL_STATE(51)] = 822,
  [SMALL_STATE(52)] = 832,
  [SMALL_STATE(53)] = 838,
  [SMALL_STATE(54)] = 842,
  [SMALL_STATE(55)] = 846,
  [SMALL_STATE(56)] = 850,
  [SMALL_STATE(57)] = 854,
  [SMALL_STATE(58)] = 858,
  [SMALL_STATE(59)] = 862,
  [SMALL_STATE(60)] = 866,
  [SMALL_STATE(61)] = 870,
  [SMALL_STATE(62)] = 874,
};

static const TSParseActionEntry ts_parse_actions[] = {
  [0] = {.entry = {.count = 0, .reusable = false}},
  [1] = {.entry = {.count = 1, .reusable = false}}, RECOVER(),
  [3] = {.entry = {.count = 1, .reusable = true}}, SHIFT(2),
  [5] = {.entry = {.count = 1, .reusable = true}}, SHIFT(3),
  [7] = {.entry = {.count = 1, .reusable = true}}, SHIFT(30),
  [9] = {.entry = {.count = 1, .reusable = true}}, SHIFT(40),
  [11] = {.entry = {.count = 1, .reusable = true}}, SHIFT(11),
  [13] = {.entry = {.count = 1, .reusable = true}}, SHIFT(6),
  [15] = {.entry = {.count = 1, .reusable = true}}, SHIFT(21),
  [17] = {.entry = {.count = 1, .reusable = true}}, SHIFT(58),
  [19] = {.entry = {.count = 1, .reusable = true}}, SHIFT(57),
  [21] = {.entry = {.count = 1, .reusable = true}}, SHIFT(56),
  [23] = {.entry = {.count = 1, .reusable = false}}, SHIFT(31),
  [25] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_scoped_ident, 1, 0, 0),
  [27] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_scoped_ident, 1, 0, 0),
  [29] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_scoped_ident, 3, 0, 0),
  [31] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_scoped_ident, 3, 0, 0),
  [33] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_expr, 1, 0, 0),
  [35] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_pri_expr, 1, 0, 0),
  [37] = {.entry = {.count = 1, .reusable = true}}, SHIFT(60),
  [39] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_expr, 1, 0, 0),
  [41] = {.entry = {.count = 1, .reusable = true}}, SHIFT(8),
  [43] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_unary_expr, 1, 0, 0),
  [45] = {.entry = {.count = 1, .reusable = true}}, SHIFT(61),
  [47] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_expr, 3, 0, 0),
  [49] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_pri_expr, 3, 0, 0),
  [51] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_expr, 2, 0, 0),
  [53] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_expr, 2, 0, 0),
  [55] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_comp, 2, 0, 0),
  [57] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_comp, 2, 0, 0),
  [59] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_expr, 1, 0, 0),
  [61] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_expr, 1, 0, 0),
  [63] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_comp, 3, 0, 0),
  [65] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_comp, 3, 0, 0),
  [67] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_type_ident, 1, 0, 0),
  [69] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_single_eval_block, 3, 0, 0),
  [71] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_single_eval_block, 3, 0, 0),
  [73] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_det_expr, 3, 0, 0),
  [75] = {.entry = {.count = 1, .reusable = true}}, SHIFT(10),
  [77] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_det_expr, 2, 0, 0),
  [79] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_det_expr, 1, 0, 0),
  [81] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_num_lit, 2, 0, 0),
  [83] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_mul_det_expr, 1, 0, 0),
  [85] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_det_expr, 1, 0, 0),
  [87] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_deref, 1, 0, 0),
  [89] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_num_lit, 1, 0, 0),
  [91] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_det_expr, 3, 0, 0),
  [93] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sizeof_expr, 4, 0, 2),
  [95] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_expr, 1, 0, 0),
  [97] = {.entry = {.count = 1, .reusable = true}}, SHIFT(37),
  [99] = {.entry = {.count = 1, .reusable = true}}, SHIFT(39),
  [101] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_mul_det_expr, 3, 0, 0),
  [103] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_det_expr, 1, 0, 0),
  [105] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_minus, 1, 0, 0),
  [107] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_sym_minus, 1, 0, 0),
  [109] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_offset_expr, 3, 0, 0),
  [111] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_offset, 1, 0, 0),
  [113] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_sym_offset, 1, 0, 0),
  [115] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_unary, 1, 0, 0),
  [117] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_cast_expr, 1, 0, 0),
  [119] = {.entry = {.count = 1, .reusable = true}}, SHIFT(33),
  [121] = {.entry = {.count = 1, .reusable = true}}, SHIFT(59),
  [123] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_expr, 2, 0, 0),
  [125] = {.entry = {.count = 1, .reusable = true}}, SHIFT(4),
  [127] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_cast_expr, 4, 0, 1),
  [129] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_offset_expr, 1, 0, 0),
  [131] = {.entry = {.count = 1, .reusable = true}}, SHIFT(20),
  [133] = {.entry = {.count = 1, .reusable = true}}, SHIFT(9),
  [135] = {.entry = {.count = 1, .reusable = true}}, SHIFT(32),
  [137] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_array_cap, 2, 0, 0),
  [139] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_type_ident, 2, 0, 0),
  [141] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_source_file, 1, 0, 0),
  [143] = {.entry = {.count = 1, .reusable = true}}, SHIFT(15),
  [145] = {.entry = {.count = 1, .reusable = true}},  ACCEPT_INPUT(),
  [147] = {.entry = {.count = 1, .reusable = true}}, SHIFT(27),
  [149] = {.entry = {.count = 1, .reusable = true}}, SHIFT(51),
  [151] = {.entry = {.count = 1, .reusable = true}}, SHIFT(50),
  [153] = {.entry = {.count = 1, .reusable = true}}, SHIFT(12),
  [155] = {.entry = {.count = 1, .reusable = true}}, SHIFT(17),
  [157] = {.entry = {.count = 1, .reusable = true}}, SHIFT(23),
};

#ifdef __cplusplus
extern "C" {
#endif
#ifdef TREE_SITTER_HIDE_SYMBOLS
#define TS_PUBLIC
#elif defined(_WIN32)
#define TS_PUBLIC __declspec(dllexport)
#else
#define TS_PUBLIC __attribute__((visibility("default")))
#endif

TS_PUBLIC const TSLanguage *tree_sitter_ProbeScope_Watch_Expr(void) {
  static const TSLanguage language = {
    .version = LANGUAGE_VERSION,
    .symbol_count = SYMBOL_COUNT,
    .alias_count = ALIAS_COUNT,
    .token_count = TOKEN_COUNT,
    .external_token_count = EXTERNAL_TOKEN_COUNT,
    .state_count = STATE_COUNT,
    .large_state_count = LARGE_STATE_COUNT,
    .production_id_count = PRODUCTION_ID_COUNT,
    .field_count = FIELD_COUNT,
    .max_alias_sequence_length = MAX_ALIAS_SEQUENCE_LENGTH,
    .parse_table = &ts_parse_table[0][0],
    .small_parse_table = ts_small_parse_table,
    .small_parse_table_map = ts_small_parse_table_map,
    .parse_actions = ts_parse_actions,
    .symbol_names = ts_symbol_names,
    .field_names = ts_field_names,
    .field_map_slices = ts_field_map_slices,
    .field_map_entries = ts_field_map_entries,
    .symbol_metadata = ts_symbol_metadata,
    .public_symbol_map = ts_symbol_map,
    .alias_map = ts_non_terminal_alias_map,
    .alias_sequences = &ts_alias_sequences[0][0],
    .lex_modes = ts_lex_modes,
    .lex_fn = ts_lex,
    .primary_state_ids = ts_primary_state_ids,
  };
  return &language;
}
#ifdef __cplusplus
}
#endif
