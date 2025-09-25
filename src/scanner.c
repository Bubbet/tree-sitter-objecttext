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
#define LOG(...) printf(__VA_ARGS__);
#endif

#define LEXER_ADVANCE(...) LOG("Advancing from c:\"%c\" d:\"%d\" s:\"%d\"\n", lexer->lookahead, lexer->lookahead, lexer_state) \
lexer->advance(lexer, __VA_ARGS__); \
LOG("New lookahead c:\"%c\" d:\"%d\"\n", lexer->lookahead, lexer->lookahead)

enum {
    BARE_STRING,
    ERROR_SENTINEL,
};


// FState
enum {
    Start,
    MaybeIdentifierStartsPeriod,
    Identifier,
    Whitespace,
    Slash,
    MaybeCommentStart,
    LineComment,
    BlockComment,
    BlockCommentMaybeEnd,
    Quote,
    QuoteBackslash,
    VerbatimQuoteMaybeStartOrEnd,
    VerbatimQuote,
    AddToken,
    Error,
};

void *tree_sitter_objecttext_external_scanner_create() { return NULL; }
void tree_sitter_objecttext_external_scanner_destroy(void *payload) { (void) payload; }

unsigned tree_sitter_objecttext_external_scanner_serialize(void *payload, char *buffer) {
    (void) payload;
    (void) buffer;
    return 0;
}

void tree_sitter_objecttext_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    (void) payload;
    (void) buffer;
    (void) length;
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

static bool is_whitespace_char(const int32_t c) {
    return c == '\t' || c == ' ';
}

static bool is_line_break_char(const int32_t c) {
    return c == '\n' || c == '\r';
}

static bool is_whitespace(const int32_t c) {
    return c == 9 || c == 10 || c == 13 || c == 32 || c == 92;
}

static bool can_start_identifier(const int32_t c) {
    return c != '.' && is_identifier_char(c);
}

bool tree_sitter_objecttext_external_scanner_scan(void *payload, TSLexer *lexer,
                                                  const bool *valid_symbols) {
    if (!valid_symbols[BARE_STRING] || valid_symbols[ERROR_SENTINEL]) return false;

    int lexer_state = Start;
    bool has_stuff = false;
    while (!lexer->eof(lexer)) {
        const int32_t lookahead = lexer->lookahead;

        switch (lexer_state) {
            case Start:
                if (can_start_identifier(lookahead)) {
                    lexer_state = Identifier;
                    break;
                }
                if (lookahead == '.') {
                    lexer_state = MaybeIdentifierStartsPeriod;
                    break;
                }
                if (is_whitespace(lookahead)) {
                    lexer_state = Whitespace;
                    break;
                }
                if (lookahead == 47) { // /
                    lexer_state = Slash;
                    break;
                }
                if (lookahead == 34) { // "
                    lexer_state = Quote;
                    break;
                }
                if (lookahead == 64) { // @
                    lexer_state = VerbatimQuoteMaybeStartOrEnd;
                    break;
                }
                if (lookahead >= 0) { // else
                    lexer_state = AddToken;
                    break; // TODO maybe break out of loop
                }
                break;
            case MaybeIdentifierStartsPeriod:
                if (can_start_identifier(lookahead)) {
                    lexer_state = Identifier;
                    break;
                }
                goto break_loop; // Was GOTO AddToken
            case Identifier:
                if (is_identifier_char(lookahead)) {
                    break;
                } // TODO dont goto break loop, we need to check for spaces followed by more identifiers AKA lexer_state = Start
                goto break_loop; // Was GOTO AddToken
            case Whitespace:
                if (is_whitespace(lookahead)) break;
                if (lookahead == 47) { // /
                    lexer_state = MaybeCommentStart;
                } // TODO dont goto break loop, we need to check for spaces followed by more identifiers AKA lexer_state = Start
                goto break_loop; // Was GOTO AddToken
            // Start Probably Not Needed
            case Slash:
                if (lookahead == 42) { // *
                    lexer_state = BlockComment;
                    break;
                }
                if (lookahead == 47) { // /
                    lexer_state = LineComment;
                    break;
                }
                goto break_loop; // Was GOTO AddToken
            case LineComment:
                if (lookahead == 10) { // \n
                    lexer_state = Whitespace;
                    break;
                }
                if (lookahead >= 0) { // else
                    break;
                }
                goto break_loop; // Was GOTO AddToken
            case BlockComment:
                if (lookahead == 42) { // *
                    lexer_state = BlockCommentMaybeEnd;
                    break;
                }
                if (lookahead >= 0) { // else
                    break;
                }
                lexer_state = Error;
                break; // Was GOTO Error
            case BlockCommentMaybeEnd:
                if (lookahead == 47) { // /
                    lexer_state = Whitespace;
                    break;
                }
                if (lookahead >= 0) { // else
                    lexer_state = BlockComment;
                    break;
                }
                goto break_loop; // Was GOTO AddToken
            // End Probably Not Needed
            case Quote:
                if (lookahead == 34) { // "
                    lexer_state = AddToken; // TODO this should probably just break out
                    break;
                }
                if (lookahead == 92) { // \ backslash
                    lexer_state = QuoteBackslash;
                    break;
                }
                if (lookahead >= 0 && lookahead != 10) { // anything but \n
                    break;
                }
                lexer_state = Error;
                break;
            case QuoteBackslash:
                if (lookahead >= 0 && lookahead != 10) { // anything but \n
                    lexer_state = Quote;
                    break;
                }
                lexer_state = Error;
                break; // Was GOTO Error
            case VerbatimQuoteMaybeStartOrEnd:
                if (lookahead == 34) { // "
                    lexer_state = VerbatimQuote;
                    break;
                }
                goto break_loop; // Was GOTO AddToken
            case VerbatimQuote:
                if (lookahead == 34) { // "
                    lexer_state = VerbatimQuoteMaybeStartOrEnd;
                    break;
                }
                if (lookahead >= 0) { // else
                    break;
                }
                lexer_state = Error;
                break; // Was GOTO Error
            case AddToken:
                goto break_loop;
            default:
                lexer_state = Error;
                goto break_loop;
        }

        lexer->mark_end(lexer);
        has_stuff = true;
        LEXER_ADVANCE(lexer_state != Whitespace)
    }

break_loop:
    if (!has_stuff) {
        LOG("Returning false: empty\n")
        return false;
    }

    if (lexer_state != AddToken) {
        LOG("Returning false: invalid state \"%d\"\n", lexer_state)
        return false;
    }

    lexer->result_symbol = BARE_STRING;
    LOG("Returning true\n")
    return true;
}
