#include <tree_sitter/parser.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>

enum {
  BARE_STRING,
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

bool tree_sitter_objecttext_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
  if (!valid_symbols[BARE_STRING]) return false;
  return false;

  bool maybe_identifier = false;
  bool only_an_identifier = false;
  bool in_quotes = false;
  while (!lexer->eof && lexer->lookahead != '\n') {
    int32_t lookahead = lexer->lookahead;
    if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') {
      if (maybe_identifier && !only_an_identifier)
            only_an_identifier = true;
      lexer->advance(lexer, true);
      continue;
    }

    if (!in_quotes) {
      if (lookahead == "\"") {
        in_quotes = true;
        lexer->advance(lexer, false);
        continue;
      }
      if (lookahead == "[" || lookahead == "{" || lookahead == ":" || lookahead == "&" || lookahead == "\"" || lookahead == ";" || lookahead == ",")
        return false;
    }

    maybe_identifier = true;

    if (only_an_identifier) only_an_identifier = false;
  }

  if (only_an_identifier) return false;
  
  lexer->mark_end(lexer);
  lexer->result_symbol = BARE_STRING;
  return true;
}
