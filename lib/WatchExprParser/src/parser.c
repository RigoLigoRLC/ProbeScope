#include "tree_sitter/parser.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define LANGUAGE_VERSION 14
#define STATE_COUNT 62
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
};

static bool ts_lex(TSLexer *lexer, TSStateId state) {
  START_LEXER();
  eof = lexer->eof(lexer);
  switch (state) {
    case 0:
      if (eof) ADVANCE(10);
      ADVANCE_MAP(
        '&', 24,
        '(', 11,
        ')', 12,
        '*', 17,
        '+', 25,
        '-', 19,
        '.', 22,
        '0', 35,
        '1', 37,
        ':', 3,
        '[', 13,
        ']', 14,
        's', 29,
        '{', 15,
        '}', 16,
      );
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(0);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(38);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 1:
      if (lookahead == '&') ADVANCE(24);
      if (lookahead == '(') ADVANCE(11);
      if (lookahead == ')') ADVANCE(12);
      if (lookahead == '*') ADVANCE(17);
      if (lookahead == '[') ADVANCE(13);
      if (lookahead == '{') ADVANCE(15);
      if (lookahead == '0' ||
          lookahead == '1') ADVANCE(40);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(1);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 2:
      if (lookahead == '(') ADVANCE(11);
      if (lookahead == '-') ADVANCE(18);
      if (lookahead == '0') ADVANCE(36);
      if (lookahead == 's') ADVANCE(6);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(2);
      if (('1' <= lookahead && lookahead <= '9')) ADVANCE(38);
      END_STATE();
    case 3:
      if (lookahead == ':') ADVANCE(26);
      END_STATE();
    case 4:
      if (lookahead == 'e') ADVANCE(7);
      END_STATE();
    case 5:
      if (lookahead == 'f') ADVANCE(20);
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
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') SKIP(9);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'F') ||
          ('a' <= lookahead && lookahead <= 'f')) ADVANCE(39);
      END_STATE();
    case 10:
      ACCEPT_TOKEN(ts_builtin_sym_end);
      END_STATE();
    case 11:
      ACCEPT_TOKEN(anon_sym_LPAREN);
      END_STATE();
    case 12:
      ACCEPT_TOKEN(anon_sym_RPAREN);
      END_STATE();
    case 13:
      ACCEPT_TOKEN(anon_sym_LBRACK);
      END_STATE();
    case 14:
      ACCEPT_TOKEN(anon_sym_RBRACK);
      END_STATE();
    case 15:
      ACCEPT_TOKEN(anon_sym_LBRACE);
      END_STATE();
    case 16:
      ACCEPT_TOKEN(anon_sym_RBRACE);
      END_STATE();
    case 17:
      ACCEPT_TOKEN(anon_sym_STAR);
      END_STATE();
    case 18:
      ACCEPT_TOKEN(anon_sym_DASH);
      END_STATE();
    case 19:
      ACCEPT_TOKEN(anon_sym_DASH);
      if (lookahead == '>') ADVANCE(23);
      END_STATE();
    case 20:
      ACCEPT_TOKEN(anon_sym_sizeof);
      END_STATE();
    case 21:
      ACCEPT_TOKEN(anon_sym_sizeof);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 22:
      ACCEPT_TOKEN(sym_sym_dotmember);
      END_STATE();
    case 23:
      ACCEPT_TOKEN(sym_sym_ptrmember);
      END_STATE();
    case 24:
      ACCEPT_TOKEN(sym_sym_takeaddr);
      END_STATE();
    case 25:
      ACCEPT_TOKEN(sym_sym_plus);
      END_STATE();
    case 26:
      ACCEPT_TOKEN(sym_sym_namespace);
      END_STATE();
    case 27:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'e') ADVANCE(30);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 28:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'f') ADVANCE(21);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 29:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'i') ADVANCE(31);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 30:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'o') ADVANCE(28);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 31:
      ACCEPT_TOKEN(sym_ident);
      if (lookahead == 'z') ADVANCE(27);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'y')) ADVANCE(32);
      END_STATE();
    case 32:
      ACCEPT_TOKEN(sym_ident);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(32);
      END_STATE();
    case 33:
      ACCEPT_TOKEN(anon_sym_0x);
      END_STATE();
    case 34:
      ACCEPT_TOKEN(anon_sym_0b);
      END_STATE();
    case 35:
      ACCEPT_TOKEN(sym_num_dec);
      if (lookahead == 'b') ADVANCE(34);
      if (lookahead == 'x') ADVANCE(33);
      if (lookahead == '0' ||
          lookahead == '1') ADVANCE(37);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(38);
      END_STATE();
    case 36:
      ACCEPT_TOKEN(sym_num_dec);
      if (lookahead == 'b') ADVANCE(34);
      if (lookahead == 'x') ADVANCE(33);
      if (('0' <= lookahead && lookahead <= '9')) ADVANCE(38);
      END_STATE();
    case 37:
      ACCEPT_TOKEN(sym_num_dec);
      if (lookahead == '0' ||
          lookahead == '1') ADVANCE(37);
      if (('2' <= lookahead && lookahead <= '9')) ADVANCE(38);
      END_STATE();
    case 38:
      ACCEPT_TOKEN(sym_num_dec);
      if (('0' <= lookahead && lookahead <= '9')) ADVANCE(38);
      END_STATE();
    case 39:
      ACCEPT_TOKEN(sym_num_hex);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'F') ||
          ('a' <= lookahead && lookahead <= 'f')) ADVANCE(39);
      END_STATE();
    case 40:
      ACCEPT_TOKEN(sym_num_bin);
      if (lookahead == '0' ||
          lookahead == '1') ADVANCE(40);
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
  [4] = {.lex_state = 2},
  [5] = {.lex_state = 2},
  [6] = {.lex_state = 2},
  [7] = {.lex_state = 1},
  [8] = {.lex_state = 1},
  [9] = {.lex_state = 2},
  [10] = {.lex_state = 2},
  [11] = {.lex_state = 0},
  [12] = {.lex_state = 0},
  [13] = {.lex_state = 0},
  [14] = {.lex_state = 2},
  [15] = {.lex_state = 0},
  [16] = {.lex_state = 0},
  [17] = {.lex_state = 0},
  [18] = {.lex_state = 0},
  [19] = {.lex_state = 0},
  [20] = {.lex_state = 0},
  [21] = {.lex_state = 0},
  [22] = {.lex_state = 0},
  [23] = {.lex_state = 1},
  [24] = {.lex_state = 0},
  [25] = {.lex_state = 0},
  [26] = {.lex_state = 0},
  [27] = {.lex_state = 0},
  [28] = {.lex_state = 0},
  [29] = {.lex_state = 0},
  [30] = {.lex_state = 0},
  [31] = {.lex_state = 0},
  [32] = {.lex_state = 0},
  [33] = {.lex_state = 0},
  [34] = {.lex_state = 2},
  [35] = {.lex_state = 2},
  [36] = {.lex_state = 0},
  [37] = {.lex_state = 0},
  [38] = {.lex_state = 0},
  [39] = {.lex_state = 0},
  [40] = {.lex_state = 0},
  [41] = {.lex_state = 1},
  [42] = {.lex_state = 0},
  [43] = {.lex_state = 0},
  [44] = {.lex_state = 0},
  [45] = {.lex_state = 0},
  [46] = {.lex_state = 0},
  [47] = {.lex_state = 0},
  [48] = {.lex_state = 0},
  [49] = {.lex_state = 0},
  [50] = {.lex_state = 1},
  [51] = {.lex_state = 0},
  [52] = {.lex_state = 0},
  [53] = {.lex_state = 0},
  [54] = {.lex_state = 1},
  [55] = {.lex_state = 0},
  [56] = {.lex_state = 9},
  [57] = {.lex_state = 0},
  [58] = {.lex_state = 1},
  [59] = {.lex_state = 1},
  [60] = {.lex_state = 0},
  [61] = {.lex_state = 0},
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
    [sym_source_file] = STATE(61),
    [sym_expr] = STATE(60),
    [sym_offset_expr] = STATE(21),
    [sym_cast_expr] = STATE(39),
    [sym_unary_expr] = STATE(42),
    [sym_postfix_expr] = STATE(11),
    [sym_pri_expr] = STATE(13),
    [sym_single_eval_block] = STATE(20),
    [sym_sym_unary] = STATE(8),
    [sym_sym_deref] = STATE(41),
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
    ACTIONS(13), 1,
      sym_ident,
    STATE(8), 1,
      sym_sym_unary,
    STATE(11), 1,
      sym_postfix_expr,
    STATE(13), 1,
      sym_pri_expr,
    STATE(20), 1,
      sym_single_eval_block,
    STATE(21), 1,
      sym_offset_expr,
    STATE(39), 1,
      sym_cast_expr,
    STATE(41), 1,
      sym_sym_deref,
    STATE(42), 1,
      sym_unary_expr,
    STATE(43), 1,
      sym_type_ident,
    STATE(47), 1,
      sym_scoped_ident,
    STATE(57), 1,
      sym_expr,
  [49] = 14,
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
    STATE(8), 1,
      sym_sym_unary,
    STATE(11), 1,
      sym_postfix_expr,
    STATE(13), 1,
      sym_pri_expr,
    STATE(20), 1,
      sym_single_eval_block,
    STATE(21), 1,
      sym_offset_expr,
    STATE(39), 1,
      sym_cast_expr,
    STATE(41), 1,
      sym_sym_deref,
    STATE(42), 1,
      sym_unary_expr,
    STATE(55), 1,
      sym_expr,
  [92] = 11,
    ACTIONS(15), 1,
      anon_sym_LPAREN,
    ACTIONS(17), 1,
      anon_sym_DASH,
    ACTIONS(19), 1,
      anon_sym_sizeof,
    ACTIONS(21), 1,
      anon_sym_0x,
    ACTIONS(23), 1,
      anon_sym_0b,
    ACTIONS(25), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_mul_det_expr,
    STATE(28), 1,
      sym_pri_det_expr,
    STATE(29), 1,
      sym_unary_det_expr,
    STATE(44), 1,
      sym_det_expr,
    STATE(27), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [127] = 11,
    ACTIONS(15), 1,
      anon_sym_LPAREN,
    ACTIONS(17), 1,
      anon_sym_DASH,
    ACTIONS(19), 1,
      anon_sym_sizeof,
    ACTIONS(21), 1,
      anon_sym_0x,
    ACTIONS(23), 1,
      anon_sym_0b,
    ACTIONS(25), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_mul_det_expr,
    STATE(28), 1,
      sym_pri_det_expr,
    STATE(29), 1,
      sym_unary_det_expr,
    STATE(46), 1,
      sym_det_expr,
    STATE(27), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [162] = 11,
    ACTIONS(15), 1,
      anon_sym_LPAREN,
    ACTIONS(17), 1,
      anon_sym_DASH,
    ACTIONS(19), 1,
      anon_sym_sizeof,
    ACTIONS(21), 1,
      anon_sym_0x,
    ACTIONS(23), 1,
      anon_sym_0b,
    ACTIONS(25), 1,
      sym_num_dec,
    STATE(26), 1,
      sym_mul_det_expr,
    STATE(28), 1,
      sym_pri_det_expr,
    STATE(29), 1,
      sym_unary_det_expr,
    STATE(36), 1,
      sym_det_expr,
    STATE(27), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [197] = 12,
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
    STATE(8), 1,
      sym_sym_unary,
    STATE(11), 1,
      sym_postfix_expr,
    STATE(13), 1,
      sym_pri_expr,
    STATE(20), 1,
      sym_single_eval_block,
    STATE(37), 1,
      sym_cast_expr,
    STATE(41), 1,
      sym_sym_deref,
    STATE(42), 1,
      sym_unary_expr,
  [234] = 12,
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
    STATE(8), 1,
      sym_sym_unary,
    STATE(11), 1,
      sym_postfix_expr,
    STATE(13), 1,
      sym_pri_expr,
    STATE(20), 1,
      sym_single_eval_block,
    STATE(38), 1,
      sym_cast_expr,
    STATE(41), 1,
      sym_sym_deref,
    STATE(42), 1,
      sym_unary_expr,
  [271] = 10,
    ACTIONS(15), 1,
      anon_sym_LPAREN,
    ACTIONS(17), 1,
      anon_sym_DASH,
    ACTIONS(19), 1,
      anon_sym_sizeof,
    ACTIONS(21), 1,
      anon_sym_0x,
    ACTIONS(23), 1,
      anon_sym_0b,
    ACTIONS(25), 1,
      sym_num_dec,
    STATE(28), 1,
      sym_pri_det_expr,
    STATE(29), 1,
      sym_unary_det_expr,
    STATE(30), 1,
      sym_mul_det_expr,
    STATE(27), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [303] = 9,
    ACTIONS(15), 1,
      anon_sym_LPAREN,
    ACTIONS(17), 1,
      anon_sym_DASH,
    ACTIONS(19), 1,
      anon_sym_sizeof,
    ACTIONS(21), 1,
      anon_sym_0x,
    ACTIONS(23), 1,
      anon_sym_0b,
    ACTIONS(25), 1,
      sym_num_dec,
    STATE(28), 1,
      sym_pri_det_expr,
    STATE(31), 1,
      sym_unary_det_expr,
    STATE(27), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [332] = 5,
    ACTIONS(29), 1,
      anon_sym_LBRACK,
    ACTIONS(31), 1,
      anon_sym_DASH,
    STATE(18), 1,
      sym_postfix_comp,
    ACTIONS(33), 2,
      sym_sym_dotmember,
      sym_sym_ptrmember,
    ACTIONS(27), 4,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      sym_sym_plus,
  [352] = 2,
    ACTIONS(37), 1,
      anon_sym_DASH,
    ACTIONS(35), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [365] = 2,
    ACTIONS(41), 1,
      anon_sym_DASH,
    ACTIONS(39), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [378] = 7,
    ACTIONS(15), 1,
      anon_sym_LPAREN,
    ACTIONS(19), 1,
      anon_sym_sizeof,
    ACTIONS(21), 1,
      anon_sym_0x,
    ACTIONS(23), 1,
      anon_sym_0b,
    ACTIONS(25), 1,
      sym_num_dec,
    STATE(22), 1,
      sym_pri_det_expr,
    STATE(27), 2,
      sym_sizeof_expr,
      sym_num_lit,
  [401] = 3,
    ACTIONS(47), 1,
      anon_sym_DASH,
    ACTIONS(45), 2,
      anon_sym_STAR,
      sym_sym_namespace,
    ACTIONS(43), 5,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [416] = 2,
    ACTIONS(51), 1,
      anon_sym_DASH,
    ACTIONS(49), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [429] = 2,
    ACTIONS(55), 1,
      anon_sym_DASH,
    ACTIONS(53), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [442] = 2,
    ACTIONS(59), 1,
      anon_sym_DASH,
    ACTIONS(57), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [455] = 2,
    ACTIONS(63), 1,
      anon_sym_DASH,
    ACTIONS(61), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [468] = 2,
    ACTIONS(47), 1,
      anon_sym_DASH,
    ACTIONS(43), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_RBRACE,
      sym_sym_dotmember,
      sym_sym_ptrmember,
      sym_sym_plus,
  [481] = 5,
    ACTIONS(67), 1,
      anon_sym_DASH,
    ACTIONS(69), 1,
      sym_sym_plus,
    STATE(6), 1,
      sym_sym_offset,
    STATE(35), 1,
      sym_sym_minus,
    ACTIONS(65), 3,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
  [499] = 1,
    ACTIONS(71), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [509] = 1,
    ACTIONS(73), 7,
      anon_sym_LPAREN,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_LBRACE,
      anon_sym_STAR,
      sym_sym_takeaddr,
      sym_ident,
  [519] = 1,
    ACTIONS(75), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [529] = 1,
    ACTIONS(77), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [539] = 2,
    ACTIONS(81), 1,
      anon_sym_STAR,
    ACTIONS(79), 6,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [551] = 1,
    ACTIONS(83), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [561] = 1,
    ACTIONS(85), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [571] = 1,
    ACTIONS(87), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [581] = 2,
    ACTIONS(81), 1,
      anon_sym_STAR,
    ACTIONS(89), 6,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [593] = 1,
    ACTIONS(91), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [603] = 1,
    ACTIONS(93), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [613] = 1,
    ACTIONS(95), 7,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACK,
      anon_sym_RBRACE,
      anon_sym_STAR,
      anon_sym_DASH,
      sym_sym_plus,
  [623] = 2,
    ACTIONS(99), 1,
      sym_num_dec,
    ACTIONS(97), 5,
      anon_sym_LPAREN,
      anon_sym_DASH,
      anon_sym_sizeof,
      anon_sym_0x,
      anon_sym_0b,
  [634] = 2,
    ACTIONS(103), 1,
      sym_num_dec,
    ACTIONS(101), 5,
      anon_sym_LPAREN,
      anon_sym_DASH,
      anon_sym_sizeof,
      anon_sym_0x,
      anon_sym_0b,
  [645] = 2,
    STATE(9), 1,
      sym_sym_minus,
    ACTIONS(105), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [656] = 1,
    ACTIONS(107), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [664] = 1,
    ACTIONS(109), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [672] = 1,
    ACTIONS(111), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [680] = 4,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(113), 1,
      anon_sym_RPAREN,
    ACTIONS(115), 1,
      anon_sym_LBRACK,
    STATE(51), 2,
      sym_array_cap,
      sym_sym_deref,
  [694] = 1,
    ACTIONS(117), 5,
      anon_sym_LPAREN,
      anon_sym_LBRACE,
      anon_sym_STAR,
      sym_sym_takeaddr,
      sym_ident,
  [702] = 1,
    ACTIONS(119), 5,
      ts_builtin_sym_end,
      anon_sym_RPAREN,
      anon_sym_RBRACE,
      anon_sym_DASH,
      sym_sym_plus,
  [710] = 4,
    ACTIONS(7), 1,
      anon_sym_STAR,
    ACTIONS(115), 1,
      anon_sym_LBRACK,
    ACTIONS(121), 1,
      anon_sym_RPAREN,
    STATE(51), 2,
      sym_array_cap,
      sym_sym_deref,
  [724] = 4,
    ACTIONS(67), 1,
      anon_sym_DASH,
    ACTIONS(123), 1,
      anon_sym_RBRACK,
    ACTIONS(125), 1,
      sym_sym_plus,
    STATE(9), 1,
      sym_sym_minus,
  [737] = 1,
    ACTIONS(127), 4,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
      sym_sym_namespace,
  [744] = 4,
    ACTIONS(67), 1,
      anon_sym_DASH,
    ACTIONS(125), 1,
      sym_sym_plus,
    ACTIONS(129), 1,
      anon_sym_RPAREN,
    STATE(9), 1,
      sym_sym_minus,
  [757] = 2,
    ACTIONS(133), 1,
      sym_sym_namespace,
    ACTIONS(131), 3,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
  [766] = 1,
    ACTIONS(45), 4,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
      sym_sym_namespace,
  [773] = 1,
    ACTIONS(135), 3,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
  [779] = 3,
    ACTIONS(137), 1,
      sym_ident,
    STATE(40), 1,
      sym_type_ident,
    STATE(47), 1,
      sym_scoped_ident,
  [789] = 1,
    ACTIONS(139), 3,
      anon_sym_RPAREN,
      anon_sym_LBRACK,
      anon_sym_STAR,
  [795] = 1,
    ACTIONS(141), 1,
      anon_sym_LPAREN,
  [799] = 1,
    ACTIONS(143), 1,
      anon_sym_RBRACK,
  [803] = 1,
    ACTIONS(145), 1,
      sym_ident,
  [807] = 1,
    ACTIONS(147), 1,
      anon_sym_RBRACE,
  [811] = 1,
    ACTIONS(149), 1,
      sym_num_hex,
  [815] = 1,
    ACTIONS(151), 1,
      anon_sym_RPAREN,
  [819] = 1,
    ACTIONS(153), 1,
      sym_ident,
  [823] = 1,
    ACTIONS(149), 1,
      sym_num_bin,
  [827] = 1,
    ACTIONS(155), 1,
      ts_builtin_sym_end,
  [831] = 1,
    ACTIONS(157), 1,
      ts_builtin_sym_end,
};

static const uint32_t ts_small_parse_table_map[] = {
  [SMALL_STATE(2)] = 0,
  [SMALL_STATE(3)] = 49,
  [SMALL_STATE(4)] = 92,
  [SMALL_STATE(5)] = 127,
  [SMALL_STATE(6)] = 162,
  [SMALL_STATE(7)] = 197,
  [SMALL_STATE(8)] = 234,
  [SMALL_STATE(9)] = 271,
  [SMALL_STATE(10)] = 303,
  [SMALL_STATE(11)] = 332,
  [SMALL_STATE(12)] = 352,
  [SMALL_STATE(13)] = 365,
  [SMALL_STATE(14)] = 378,
  [SMALL_STATE(15)] = 401,
  [SMALL_STATE(16)] = 416,
  [SMALL_STATE(17)] = 429,
  [SMALL_STATE(18)] = 442,
  [SMALL_STATE(19)] = 455,
  [SMALL_STATE(20)] = 468,
  [SMALL_STATE(21)] = 481,
  [SMALL_STATE(22)] = 499,
  [SMALL_STATE(23)] = 509,
  [SMALL_STATE(24)] = 519,
  [SMALL_STATE(25)] = 529,
  [SMALL_STATE(26)] = 539,
  [SMALL_STATE(27)] = 551,
  [SMALL_STATE(28)] = 561,
  [SMALL_STATE(29)] = 571,
  [SMALL_STATE(30)] = 581,
  [SMALL_STATE(31)] = 593,
  [SMALL_STATE(32)] = 603,
  [SMALL_STATE(33)] = 613,
  [SMALL_STATE(34)] = 623,
  [SMALL_STATE(35)] = 634,
  [SMALL_STATE(36)] = 645,
  [SMALL_STATE(37)] = 656,
  [SMALL_STATE(38)] = 664,
  [SMALL_STATE(39)] = 672,
  [SMALL_STATE(40)] = 680,
  [SMALL_STATE(41)] = 694,
  [SMALL_STATE(42)] = 702,
  [SMALL_STATE(43)] = 710,
  [SMALL_STATE(44)] = 724,
  [SMALL_STATE(45)] = 737,
  [SMALL_STATE(46)] = 744,
  [SMALL_STATE(47)] = 757,
  [SMALL_STATE(48)] = 766,
  [SMALL_STATE(49)] = 773,
  [SMALL_STATE(50)] = 779,
  [SMALL_STATE(51)] = 789,
  [SMALL_STATE(52)] = 795,
  [SMALL_STATE(53)] = 799,
  [SMALL_STATE(54)] = 803,
  [SMALL_STATE(55)] = 807,
  [SMALL_STATE(56)] = 811,
  [SMALL_STATE(57)] = 815,
  [SMALL_STATE(58)] = 819,
  [SMALL_STATE(59)] = 823,
  [SMALL_STATE(60)] = 827,
  [SMALL_STATE(61)] = 831,
};

static const TSParseActionEntry ts_parse_actions[] = {
  [0] = {.entry = {.count = 0, .reusable = false}},
  [1] = {.entry = {.count = 1, .reusable = false}}, RECOVER(),
  [3] = {.entry = {.count = 1, .reusable = true}}, SHIFT(2),
  [5] = {.entry = {.count = 1, .reusable = true}}, SHIFT(3),
  [7] = {.entry = {.count = 1, .reusable = true}}, SHIFT(23),
  [9] = {.entry = {.count = 1, .reusable = true}}, SHIFT(41),
  [11] = {.entry = {.count = 1, .reusable = true}}, SHIFT(20),
  [13] = {.entry = {.count = 1, .reusable = true}}, SHIFT(15),
  [15] = {.entry = {.count = 1, .reusable = true}}, SHIFT(5),
  [17] = {.entry = {.count = 1, .reusable = true}}, SHIFT(14),
  [19] = {.entry = {.count = 1, .reusable = true}}, SHIFT(52),
  [21] = {.entry = {.count = 1, .reusable = true}}, SHIFT(56),
  [23] = {.entry = {.count = 1, .reusable = true}}, SHIFT(59),
  [25] = {.entry = {.count = 1, .reusable = false}}, SHIFT(33),
  [27] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_expr, 1, 0, 0),
  [29] = {.entry = {.count = 1, .reusable = true}}, SHIFT(4),
  [31] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_unary_expr, 1, 0, 0),
  [33] = {.entry = {.count = 1, .reusable = true}}, SHIFT(54),
  [35] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_comp, 2, 0, 0),
  [37] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_comp, 2, 0, 0),
  [39] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_expr, 1, 0, 0),
  [41] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_expr, 1, 0, 0),
  [43] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_expr, 1, 0, 0),
  [45] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_scoped_ident, 1, 0, 0),
  [47] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_pri_expr, 1, 0, 0),
  [49] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_comp, 3, 0, 0),
  [51] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_comp, 3, 0, 0),
  [53] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_expr, 3, 0, 0),
  [55] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_pri_expr, 3, 0, 0),
  [57] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_postfix_expr, 2, 0, 0),
  [59] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_postfix_expr, 2, 0, 0),
  [61] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_single_eval_block, 3, 0, 0),
  [63] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_single_eval_block, 3, 0, 0),
  [65] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_expr, 1, 0, 0),
  [67] = {.entry = {.count = 1, .reusable = true}}, SHIFT(34),
  [69] = {.entry = {.count = 1, .reusable = true}}, SHIFT(35),
  [71] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_det_expr, 2, 0, 0),
  [73] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_deref, 1, 0, 0),
  [75] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_num_lit, 2, 0, 0),
  [77] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_det_expr, 3, 0, 0),
  [79] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_det_expr, 1, 0, 0),
  [81] = {.entry = {.count = 1, .reusable = true}}, SHIFT(10),
  [83] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_pri_det_expr, 1, 0, 0),
  [85] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_det_expr, 1, 0, 0),
  [87] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_mul_det_expr, 1, 0, 0),
  [89] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_det_expr, 3, 0, 0),
  [91] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_mul_det_expr, 3, 0, 0),
  [93] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sizeof_expr, 4, 0, 2),
  [95] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_num_lit, 1, 0, 0),
  [97] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_minus, 1, 0, 0),
  [99] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_sym_minus, 1, 0, 0),
  [101] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_offset, 1, 0, 0),
  [103] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_sym_offset, 1, 0, 0),
  [105] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_offset_expr, 3, 0, 0),
  [107] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_cast_expr, 4, 0, 1),
  [109] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_unary_expr, 2, 0, 0),
  [111] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_offset_expr, 1, 0, 0),
  [113] = {.entry = {.count = 1, .reusable = true}}, SHIFT(32),
  [115] = {.entry = {.count = 1, .reusable = true}}, SHIFT(53),
  [117] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_sym_unary, 1, 0, 0),
  [119] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_cast_expr, 1, 0, 0),
  [121] = {.entry = {.count = 1, .reusable = true}}, SHIFT(7),
  [123] = {.entry = {.count = 1, .reusable = true}}, SHIFT(16),
  [125] = {.entry = {.count = 1, .reusable = true}}, SHIFT(9),
  [127] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_scoped_ident, 3, 0, 0),
  [129] = {.entry = {.count = 1, .reusable = true}}, SHIFT(25),
  [131] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_type_ident, 1, 0, 0),
  [133] = {.entry = {.count = 1, .reusable = true}}, SHIFT(58),
  [135] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_array_cap, 2, 0, 0),
  [137] = {.entry = {.count = 1, .reusable = true}}, SHIFT(48),
  [139] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_type_ident, 2, 0, 0),
  [141] = {.entry = {.count = 1, .reusable = true}}, SHIFT(50),
  [143] = {.entry = {.count = 1, .reusable = true}}, SHIFT(49),
  [145] = {.entry = {.count = 1, .reusable = true}}, SHIFT(12),
  [147] = {.entry = {.count = 1, .reusable = true}}, SHIFT(19),
  [149] = {.entry = {.count = 1, .reusable = true}}, SHIFT(24),
  [151] = {.entry = {.count = 1, .reusable = true}}, SHIFT(17),
  [153] = {.entry = {.count = 1, .reusable = true}}, SHIFT(45),
  [155] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_source_file, 1, 0, 0),
  [157] = {.entry = {.count = 1, .reusable = true}},  ACCEPT_INPUT(),
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
