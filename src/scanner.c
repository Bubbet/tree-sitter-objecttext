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

static bool is_identifier_char_ascii(int32_t c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == '_' || c == '.';
}

static bool is_number_char(int32_t c) {
  return (c >= '0' && c <= '9') || c == '.';
}

static bool isspace(int32_t c) {
  return c == ' ' || c == '\t' || c == ';' || c ==',';
}

static bool contains_operator(const int32_t *buf) {
    for (const int32_t *p = buf; *p; p++) {
        int32_t c = *p;

        if (c == '+' || c == '-' || c == '*' || c == '/') {
            // Check the character before the operator
            const int32_t *left = p - 1;
            while (left >= buf && isspace(*left)) left--;
            bool valid_left = (left >= buf && (is_number_char(*left) || *left == ')' || *left == ']'));

            // Check the character after the operator
            const int32_t *right = p + 1;
            while (*right && isspace(*right)) right++;
            bool valid_right = (*right && (is_number_char(*right) || *right == '(' || *right == '['));

            if (valid_left && valid_right) {
                return true;
            }
        }
    }
    return false;
}

bool tree_sitter_objecttext_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
  if (!valid_symbols[BARE_STRING]) return false;

  // Skip leading whitespace
  while (isspace(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }

  // Record buffer
  int32_t buf[1024];
  unsigned len = 0;

  // Read until newline or delimiters
  bool inside_string = false;
  bool inside_verbatim = false;

  if (lexer->lookahead == '"') {
    inside_string = true;
  } else if (lexer->lookahead == '@') {
    // Check for @"..." verbatim
    lexer->advance(lexer, false);
    if (lexer->lookahead == '"') {
      inside_verbatim = true;
    } else {
      lexer->advance(lexer, true); // rollback, wasn't verbatim
    }
  }

  while (lexer->lookahead != 0) {
    int32_t c = lexer->lookahead;

    // Stop if unquoted and hit newline, ; or ,
    if (!inside_string && !inside_verbatim && (c == '\n' || c == '\r' || c == ';' || c == ',')) {
      break;
    }

    if (len < sizeof(buf)/sizeof(buf[0]) - 1) {
      buf[len++] = c;
    }

    // Track string boundaries
    if (inside_string && c == '"' && (len == 1 || buf[len-2] != '\\')) {
      inside_string = false; // close string
    }

    if (inside_verbatim && c == '"') {
      inside_verbatim = false; // close verbatim
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
    if (iswalnum((wint_t)buf[i]) || buf[i] == '_' || buf[i] == '.') {
      while (i < len && is_identifier_char_ascii(buf[i])) {
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
    if (!is_identifier_char_ascii(buf[i])) { all_ident = false; break; }
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
