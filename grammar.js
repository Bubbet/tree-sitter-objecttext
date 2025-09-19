/**
 * @file Halfling Engine's Object Text. Used in Cosmoteer.
 * @author Bubbet <objecttext@bubbet.dev>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "objecttext",
  extras: $ => [
    /[\t\r\n \\]/,
    $.comment,
  ],
  supertypes: $ => [
    $.expression,
  ],
  rules: {
    // TODO: add the actual grammar rules
    source_file: $ => $._assignment,

    // Periods can start identifiers but only when followed by another char two dots in a row at the start is not allowed.
    identifier: $ => /\.?[A-Za-z0-9_][A-Za-z0-9_\.]*/,
    comment: $ => token(
      choice(seq("//", /.*/), seq("/*", /[^*]*\*+([^/*][^*]*\*+)*/, "/")),
    ),
    internal_reference: $ => prec.right(seq($.identifier, repeat(seq('/', $.identifier)))),
    path_reference: $ => prec.right(seq('<', /.+/, '>', repeat(seq('/', $.identifier)))),
    reference: $ => seq('&', choice(
      $.internal_reference,
      $.path_reference,
    )),
    value: $ => choice(
      $.bare_word,
      $.expression,
      $.string,
      $.verbatim,
    ),
    bare_word: $ => token.immediate(prec(-1, /\s*[^\[{"].*/)),
    string: $ => choice(
      '""',
      seq('"', repeat(token.immediate(choice(/[^\n"]/, /\\"/))), token.immediate('"')),
    ),
    verbatim: $ => seq('@"', /[^"]*/, '"'),

    assignment: $ => seq($.identifier, '=', $.value),
    _assignment: $ => choice(
      $.assignment,
      $.group,
      $.list,
    ),

    group: $ => seq(field("key", $.identifier), choice(optional('='), seq(':', repeat($.extension))), $._group),
    list: $ => seq($.identifier, choice(optional('='), seq(':', repeat($.extension))), $._list),

    _block_value: $ => choice($._assignment,
      alias(seq(optional(seq(':', repeat($.extension))), $._group), $.group),
      alias(seq(optional(seq(':', repeat($.extension))), $._list), $.list),
    ),

    extension: $ => choice(
      seq($.path_reference, /[;,]?/),
      seq($.internal_reference, /[;,]?/),
    ),

    _list: $ => seq('[', repeat($._block_value), ']'),
    _group: $ => seq('{', repeat($._block_value), '}'),

    expression: $ => choice(
      $.number,
      $.reference,
      $.binary_expression,
      $.urinary_expression,
      $.parenthesized_expression,
      //$.function_call, // TODO breaks bare_word for some reason
    ),
    urinary_expression: $ => choice(
      prec.left(seq('-', $.expression))
    ),
    binary_expression: $ => choice(
      prec.left(seq($.expression, choice('*', '/'), $.expression)),
      prec.left(seq($.expression, choice('+', '-'), $.expression)),
    ),
    parenthesized_expression: $ => seq(
      '(',
      $.expression,
      ')',
    ),
    function_call: $ => seq(
      field("name", $.identifier),
      '(',
      commaSep($.expression),
      ')',
    ),
    number: $ => /-?\d*\.?\d+%?d?/,
  }
});

/**
 * @param {RuleOrLiteral} rule
 */
function commaSep(rule) {
  return optional(seq(rule, repeat(seq(',', rule))));
}