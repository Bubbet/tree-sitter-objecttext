#include <tree_sitter/parser.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

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

static bool is_identifier_char(int32_t c) {
    return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == '_' || c == '.';
}

bool tree_sitter_objecttext_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
  printf("Found char: %c %d %c\n", lexer->lookahead, lexer->lookahead, valid_symbols[BARE_STRING] ? 'y' : 'n');
  if (!valid_symbols[BARE_STRING] || valid_symbols[ERROR_SENTINEL]) return false;

  bool in_quotes = false;
  int identifier_state = NOT_IDENTIFIER;
  bool has_stuff = false;
  while (!lexer->eof(lexer) && lexer->lookahead != '\n') { //(!lexer->eof && lexer->lookahead != '\n') {
    int32_t lookahead = lexer->lookahead;
    printf("lookahead: %c %d\n", lookahead, lookahead);
    if (('\t' <= lookahead && lookahead <= '\r') || lookahead == ' ') {
      if (identifier_state == MAYBE_IDENTIFIER) identifier_state = SPACE_AFTER_IDENTIFIER;
      printf("found space\n");
      lexer->advance(lexer, true);
      continue;
    }

    if (!in_quotes) {
      if (lookahead == '"') {
        in_quotes = true;
        lexer->advance(lexer, false);
        printf("found quote\n");
        continue;
      }
      if (lookahead == '[' || lookahead == ']' || lookahead == '{' || lookahead == ':' || lookahead == '&' || lookahead == '"' || lookahead == ';' || lookahead == ',') { printf("returning false: terminator char found\n"); return false; }
      if (identifier_state == NOT_IDENTIFIER && is_identifier_char(lookahead)) identifier_state = MAYBE_IDENTIFIER;
      if (identifier_state == SPACE_AFTER_IDENTIFIER) identifier_state = MORE_AFTER_IDENTIFIER;
    } else if (lookahead == '"') {
      printf("returning false: found string or virbatim\n");
      return false;
    }

    has_stuff = true;
    lexer->advance(lexer, false);
  }

  if (!has_stuff) { printf("returning false: empty\n"); return false; }

  if (identifier_state == MAYBE_IDENTIFIER || identifier_state == SPACE_AFTER_IDENTIFIER) { printf("returning false: identifier found\n"); return false; }

  lexer->mark_end(lexer);
  lexer->result_symbol = BARE_STRING;
  printf("returning true\n");
  return true;
}
