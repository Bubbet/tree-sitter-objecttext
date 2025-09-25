#include <tree_sitter/parser.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define SCANNER_LOGGING
#if defined(__EMSCRIPTEN__) || defined(__wasm__) || !defined(SCANNER_LOGGING)
  // WASM build: define as no-op
  #define LOG(...)
#else
  // Non-WASM build: real definition
  #define LOG(...) printf(__VA_ARGS__)
#endif

#define ADVANCE(...) LOG("Advancing from char: %c %d\n", lexer->lookahead, lexer->lookahead); \
lexer->advance(lexer, __VA_ARGS__); \
LOG("New lookahead %c %d\n", lexer->lookahead, lexer->lookahead);

enum {
  BARE_STRING,
  ERROR_SENTINEL,
};

enum {
    NOT_IDENTIFIER,
    MAYBE_IDENTIFIER,
    SPACE_AFTER_IDENTIFIER,
    MORE_AFTER_IDENTIFIER,
};

void * tree_sitter_objecttext_external_scanner_create() { return NULL; }
void tree_sitter_objecttext_external_scanner_destroy(void *payload) { (void)payload; }

unsigned tree_sitter_objecttext_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload; (void)buffer;
  return 0;
}
void tree_sitter_objecttext_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  (void)payload; (void)buffer; (void)length;
}

static bool is_identifier_char(const int32_t c) {
    return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == '_' || c == '.';
}

static bool is_terminator_char(const int32_t c) {
  return c == '[' || c == ']' || c == '{' || c == '}' || c == ':' || c == '&' || c == '"' || c == ';' || c == ',';
}

bool tree_sitter_objecttext_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
  char error = valid_symbols[ERROR_SENTINEL] ? 'y' : 'n';
  LOG("Found char: %c %d %c\n", lexer->lookahead, lexer->lookahead, error);
  if (!valid_symbols[BARE_STRING] || valid_symbols[ERROR_SENTINEL]) return false;

  bool in_quotes = false;
  int identifier_state = NOT_IDENTIFIER;
  bool has_stuff = false;
  while (!lexer->eof(lexer)) {// && lexer->lookahead != '\n') { //(!lexer->eof && lexer->lookahead != '\n') {
    int32_t lookahead = lexer->lookahead;
    LOG("lookahead: %c %d %c\n", lookahead, lookahead, error);
    if (('\t' <= lookahead && lookahead <= '\r' && lookahead != '\n') || lookahead == ' ') {
      if (identifier_state == MAYBE_IDENTIFIER) identifier_state = SPACE_AFTER_IDENTIFIER;
      LOG("found space %c\n", error);
      ADVANCE(true)
      continue;
    }

    if (!in_quotes) {
      if (lookahead == '"') {
        in_quotes = true;
        ADVANCE(false)
        LOG("found quote %c\n", error);
        continue;
      }
      if ((identifier_state == NOT_IDENTIFIER || identifier_state == MORE_AFTER_IDENTIFIER) && (lookahead == '\n' || is_terminator_char(lookahead))) break;
      if (is_terminator_char(lookahead)) { LOG("returning false: terminator char found %c\n", error); return false; }
      if (identifier_state == NOT_IDENTIFIER && is_identifier_char(lookahead)) identifier_state = MAYBE_IDENTIFIER;
      if (identifier_state == MAYBE_IDENTIFIER && !is_identifier_char(lookahead)) identifier_state = MORE_AFTER_IDENTIFIER;
      if (identifier_state == SPACE_AFTER_IDENTIFIER) identifier_state = MORE_AFTER_IDENTIFIER;
    } else if (lookahead == '"') {
      LOG("returning false: found string or verbatim %c\n", error);
      return false;
    }

    has_stuff = true;
    ADVANCE(false)
  }

  if (!has_stuff) { LOG("returning false: empty %c\n", error); return false; }

  if (identifier_state == MAYBE_IDENTIFIER || identifier_state == SPACE_AFTER_IDENTIFIER) { LOG("returning false: identifier found %c\n", error); return false; }

  lexer->mark_end(lexer);
  lexer->result_symbol = BARE_STRING;
  LOG("returning true %c\n", error);
  return true;
}
