/**
 * @file Halfling Engine's Object Text. Used in Cosmoteer.
 * @author Bubbet <objecttext@bubbet.dev>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const PREC = {
    func: 4,
    unary: 3,
    mul: 2,
    add: 1,
}
const IDENTIFIER = "\\.?[a-zA-Z0-9_][a-zA-Z0-9_\\.]*"
const PATH_FILE = "<[^>]+>(\/" + IDENTIFIER + "\)*" //"<[^>]+>(\/" + IDENTIFIER + ")*"
const LEADING_GROUP = "\(\([.~]\/\)|\/\)?"
const INTERNAL_IDENTIFIER = "\(" + IDENTIFIER + "|\\^|\(\\.\\.\))"
const PATH_INTERNAL = LEADING_GROUP + INTERNAL_IDENTIFIER + "\(\\/" + INTERNAL_IDENTIFIER + "\)*" //"(~|\.|(\.\.))?\/?(" + IDENTIFIER + ")|\^(\/(" + IDENTIFIER + ")|\^)*"
const TERMINATOR_INTERNAL = INTERNAL_IDENTIFIER + "\\\.[a-zA-Z_]+";
const LOOSE_PATH_FILE = LEADING_GROUP + "\(" + TERMINATOR_INTERNAL + "|((" + INTERNAL_IDENTIFIER + "\/\)+" + TERMINATOR_INTERNAL + "\)\)";
//LEADING_GROUP + "\(" + INTERNAL_IDENTIFIER + "\/|" + INTERNAL_IDENTIFIER + "\)";
// ([.~]/)?(Identifier/|Identifier)*Identifier\.a-zA-Z_
// (TerminatorInternal|((Identifier/)+TerminatorInternal))

module.exports = grammar({
    name: "objecttext",
    extras: $ => [
        /\s/,
        $.comment
    ],
    supertypes: $ => [
        $.expression,
    ],
    externals: $ => [
        $.bare_string, $.error_sentinel
    ],
    rules: {
        source_file: $ => repeat1($._assignment),
        _assignment: $ => choice(
            $.assignment,
            $.block,
            $.list,
        ),
        assignment: $ => seq(field("key", $.identifier), "=", field("value", $.value)),

        _equal_or_extension: $ => prec(1, choice("=", $._extension_chain)),
        _extension_chain: $ => seq(":", repeat1($.extension)),
        _value_extension_chain: $ => prec(1, choice(
          seq($.identifier, '='),
          seq($.identifier, optional($._extension_chain)),
          $._extension_chain,
        )),
        block: $ => seq(field("key", $.identifier), optional($._equal_or_extension), $._block_group),
        list: $ => seq(field("key", $.identifier), optional($._equal_or_extension), $._list_group),
        _block_as_value_def: $ => seq(optional($._value_extension_chain), $._block_group),
        _list_as_value_def: $ => seq(optional($._value_extension_chain), $._list_group),
        _block_as_value: $ => alias($._block_as_value_def, $.block),
        _list_as_value: $ => alias($._list_as_value_def, $.list),
        _block_group: $ => seq("{", repeat($._block_value), "}"),
        _list_group: $ => seq("[", repeat($._list_value), "]"),
        _block_value: $ => seq($._assignment, optional($._split)),
        _list_value: $ => seq($.value, optional($._split)),
        _split: $ => choice(",", ";"),

        extension: $ => seq(choice(
            new RegExp("&?" + PATH_FILE),
            new RegExp("&?" + PATH_INTERNAL),
        ), optional($._split)),

        reference: $ => choice(
            new RegExp("&?" + PATH_FILE),
            new RegExp("&" + PATH_INTERNAL),
            //new RegExp(LOOSE_PATH_FILE),
        ),

        number: _ => {
            const decimalDigits = /\d(_?\d)*/;
            const signedInteger = seq(optional(choice('-', '+')), decimalDigits);
            const exponentPart = seq(choice('e', 'E'), signedInteger);
            const decimalIntegerLiteral = choice(
                '0',
                seq(optional('0'), /[1-9]/, optional(seq(optional('_'), decimalDigits))),
            );

            const decimalLiteral = choice(
                seq(decimalIntegerLiteral, '.', optional(decimalDigits), optional(exponentPart)),
                seq('.', decimalDigits, optional(exponentPart)),
                seq(decimalIntegerLiteral, exponentPart),
                decimalDigits,
            );

            return token(choice(
                decimalLiteral,
                /\d*\.?\d+[dr%]?/,
            ));
        },
        expression: $ => choice(
            $.number,
            $.reference,
            $.unary_expression,
            $.binary_expression,
        ),
        unary_expression: $ => choice(
            prec.left(PREC.unary, seq('(', $.expression, ')')),
            prec.left(PREC.unary, seq('-', $.expression)),
        ),
        mul: _ => '*',
        div: _ => '/',
        add: _ => '+',
        sub: _ => '-',
        binary_expression: $ => choice(
            prec.left(PREC.mul, seq(field('left', $.expression), field('operator', choice($.mul, $.div)), field('right', $.expression))),
            prec.left(PREC.add, seq(field('left', $.expression), field('operator', choice($.add, $.sub)), field('right', $.expression))),
        ),
        // TODO this techinically isnt part of ObjectText, it just parses the bare_word as a expression using mxparser
        // Though, ObjectText does replace references inside that bareword with the real value.
        function_call: $ => prec.left(PREC.func, seq($.identifier, token.immediate('('), repeat1(seq($.expression, optional(','))), ')')),

        true: _ => 'true',
        false: _ => 'false',
        bool: $ => choice($.true, $.false),
        //bare_string: $ => token(prec(-2, /[^\n]+/)),
        //bare_string: $ => /[^\n(){}\[\]:;"&]*\n/,
        identifier: $ => new RegExp(IDENTIFIER),
        _string_fragment: $ => seq(
            '"',
            repeat(choice(
                /[^"\\]/,      // any char except " or \
                /\\"/,         // escaped quote
                /\\./          // other escapes
            )),
            '"'
        ),
        string: $ => choice(
            seq(
                $._string_fragment,
                repeat(seq(
                    /\\\r?\n/,  // line continuation
                    $._string_fragment
                ))
            ),
            /".*"\s*\n/
        ),

        _verbatim_fragment: $ => seq(
            '@"',
            repeat(choice(
                /[^"\\]/,      // any char except " or \
                /\\"/,         // escaped quote
                /\\./          // other escapes
            )),
            '"'
        ),
        verbatim: $ => seq(
            $._verbatim_fragment,
            repeat(seq(
                /\\\r?\n/,  // line continuation
                choice($._verbatim_fragment, $._string_fragment)
            ))
        ),
        //string: $ => seq('"', /([^"]|(\\")|("\\\s*"))*/, '"'),
        //verbatim: $ => seq('@"', /([^"]|(\\")|("\\\s*"))*/, '"'),
        value: $ => choice(
            $.string,
            $.verbatim,
            $._block_as_value,
            $._list_as_value,
            $.expression,
            $.function_call,
            $.identifier,
            $.bool,
            $.bare_string,
        ),
        comment: (_) => token(choice(
            seq("//", /[^\n]*/),
            seq("/*", /([^*]|\*+[^/*])*\*+/, "/")
        )),
    }
});

/**
 * @param {RuleOrLiteral} rule
 */
function commaSep(rule) {
    return optional(seq(rule, repeat(seq(',', rule))));
}