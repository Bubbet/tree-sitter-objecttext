#include <tree_sitter/parser.h>
#include <wchar.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

//#define SCANNER_LOGGING
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
    Start, // 0
    MaybeIdentifierStartsPeriod, // 1
    Identifier, // 2
    Whitespace, // 3
    Slash, // 4
    MaybeCommentStart, // 5
    LineComment, // 6
    BlockComment, // 7
    BlockCommentMaybeEnd, // 8
    Quote, // 9
    QuoteBackslash, // 10
    VerbatimQuoteMaybeStartOrEnd, // 11
    VerbatimQuote, // 12
    BareString, // 13
    Error, // 14
    WhitespaceAfterIdentifier, // 15
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

static bool is_number_char(const int32_t c) {
    return c >= '0' && c <= '9';
}

static bool is_operator_char(const int32_t c) {
    return c == '*' || c == '/' || c == '+' || c == '-';
}

static bool is_identifier_char(const int32_t c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_' || c == '.';
}

static bool is_terminator_char(const int32_t c) {
    return c == '\n' || c == '[' || c == ']' || c == '{' || c == '}' || c == ':' || c == '&' || c == ';' || c == ',';
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
                start:
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
                if (is_terminator_char(lookahead)) {
                    // break;
                    goto break_loop;
                }
                if (lookahead == '-' || lookahead == '(') {
                    lexer_state = Error;
                    goto break_loop;
                }
                if (is_number_char(lookahead) || is_operator_char(lookahead)) {
                    lexer_state = Error;
                    goto break_loop;
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
                    lexer_state = BareString;
                    break;
                }
                break;
            case MaybeIdentifierStartsPeriod:
                if (can_start_identifier(lookahead)) {
                    lexer_state = Identifier;
                    break;
                }
                if (is_terminator_char(lookahead)) {
                    lexer_state = BareString;
                    goto break_loop;
                }
                goto start;
            case Identifier:
                if (is_identifier_char(lookahead)) {
                    break;
                }
                if (lookahead == '(') {
                    lexer_state = Error;
                    goto break_loop;
                }
                if (is_terminator_char(lookahead))
                    goto break_loop;
                if (is_whitespace(lookahead)) {
                    lexer_state = WhitespaceAfterIdentifier;
                    break;
                }
                goto start;
            case WhitespaceAfterIdentifier:
                if (is_whitespace(lookahead)) {
                    break;
                }
                if (is_terminator_char(lookahead)) {
                    goto break_loop;
                }
                if (lookahead >= 0) {
                    lexer_state = BareString;
                    break;
                }
                lexer_state = Error;
                break;
            case Whitespace:
                if (is_whitespace(lookahead)) {
                    break;
                }
                if (lookahead == 47) { // /
                    lexer_state = MaybeCommentStart;
                }
                if (is_terminator_char(lookahead)) {
                    lexer_state = Error;
                    goto break_loop;
                }
                goto start;
            case Quote:
                if (lookahead == 34) { // "
                    goto break_loop;
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
            case BareString:
                if (is_terminator_char(lookahead))
                    goto break_loop;
                break;
            default:
                lexer_state = Error;
                goto break_loop;
        }

        has_stuff = true;
        LEXER_ADVANCE(lexer_state == Whitespace)
    }

break_loop:
    if (!has_stuff) {
        LOG("Returning false: empty\n")
        return false;
    }

    if (lexer_state != BareString) {
        LOG("Returning false: invalid state \"%d\"\n", lexer_state)
        return false;
    }

    lexer->mark_end(lexer);
    lexer->result_symbol = BARE_STRING;
    LOG("Returning true\n")
    return true;
}
