// scanner.c
#include <tree_sitter/parser.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>

enum {
  BARE_STRING, // index 0 -> must match the external token order in grammar.js
};

void *tree_sitter_objecttext_external_scanner_create() { return NULL; }
void tree_sitter_objecttext_external_scanner_destroy(void *p) { (void)p; }

unsigned tree_sitter_objecttext_external_scanner_serialize(void *p, char *buffer) {
  (void)p; (void)buffer;
  return 0;
}
void tree_sitter_objecttext_external_scanner_deserialize(void *p, const char *b, unsigned n) {
  (void)p; (void)b; (void)n;
}

static bool is_identifier_char(int32_t c) {
  return iswalnum(c) || c == '_' || c == '.';
}

static bool is_number_char(int32_t c) {
  return iswdigit(c) || c == '.';
}

static bool contains_operator(const char *buf) {
    for (const char *p = buf; *p; p++) {
        char c = *p;
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            return true;
        }
    }
    return false;
}

bool tree_sitter_objecttext_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
  if (!valid_symbols[BARE_STRING]) return false;

  // Skip leading whitespace
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, true);
  }

  // Record buffer
  char buf[1024]; // adjust if needed
  unsigned len = 0;

  // Read until newline or EOF
  while (lexer->lookahead != 0 && lexer->lookahead != '\n' && lexer->lookahead != '\r' && lexer->lookahead != ';' && lexer->lookahead != ',') {
    if (len < sizeof(buf) - 1) {
      buf[len++] = (char)lexer->lookahead;
    }
    lexer->advance(lexer, false);
  }
  buf[len] = '\0';

  if (len == 0) return false;

  // ==== Checks against other grammar ====

  // Quoted string
  if (buf[0] == '"' && buf[len-1] == '"' && len > 1) return false;

  // Verbatim @"..."
  if (buf[0] == '@' && buf[1] == '"' && buf[len-1] == '"') return false;
  
  // Reference
  if (buf[0] == '&') return false;

  // Braced or bracketed
  if (buf[0] == '{' || buf[0] == '[' || buf[0] == '(') return false;

  {
    bool valid_func = false;
    unsigned i = 0;

    // Leading identifier
    if (iswalnum(buf[i]) || buf[i] == '_' || buf[i] == '.') {
        while (i < len && is_identifier_char(buf[i])) {
        i++;
        }
        if (i < len && buf[i] == '(') {
        valid_func = true;
        }
    }
    if (valid_func) return false;
  }


  // Pure identifier
  bool all_ident = true;
  for (unsigned i = 0; i < len; i++) {
    if (!is_identifier_char(buf[i])) { all_ident = false; break; }
  }
  if (all_ident) return false;

  // Pure number
  bool all_num = true;
  for (unsigned i = 0; i < len; i++) {
    if (!is_number_char(buf[i])) { all_num = false; break; }
  }
  if (all_num) return false;

  // Looks like an expression (contains operator)
  if (contains_operator(buf)) {
    return false;
  }

  // ==== Otherwise, treat as bare_string ====
  lexer->mark_end(lexer);
  lexer->result_symbol = BARE_STRING;
  return true;
}
