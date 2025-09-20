/**
 * @file Halfling Engine's Object Text. Used in Cosmoteer.
 * @author Bubbet <objecttext@bubbet.dev>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const PREC = {
  assignment: 4,
  urary: 3,
  mul: 2,
  add: 1,
}

module.exports = grammar({
  name: "objecttext",
  extras: $ => [
    /\s/,
    $.comment
  ],
  supertypes: $ => [
    $.expression,
  ],
  rules: {
    source_file: $ => repeat1(prec(-1, $._assignment)),
    _assignment: $ => choice(
      alias($._bare_word_assignment, $.assignment),
      $.assignment,
      $.block,
      $.list,
    ),

    _bare_word_assignment: $ => prec(-1, seq(alias(/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*/, $.identifier), '=', alias(/[^\n]+/, $.bare_word))),
    assignment: $ => prec(PREC.assignment, seq(field("key", $.identifier), "=", field("value", $.value))),
    list: $ => prec.right(PREC.assignment, seq(field("key", $.identifier), optional(choice('=', seq(':', repeat1($.extension)))), "[", repeat($._list_value), "]")),
    block: $ => prec.right(PREC.assignment, seq(field("key", $.identifier), optional(choice('=', seq(':', repeat1($.extension)))), "{", repeat($._block_value), "}")),
    _list_as_value: $ => prec.right(seq(optional(field("key", $.identifier)), optional(choice('=', seq(':', repeat1($.extension)))), "[", repeat($._list_value), "]")),
    _block_as_value: $ => prec.right(seq(optional(field("key", $.identifier)), optional(choice('=', seq(':', repeat1($.extension)))), "{", repeat($._block_value), "}")),
    _list_value: $ => choice($.value, $._assignment),
    _block_value: $ => $._assignment,

    extension: $ => seq(choice(
      /<[^>]+>(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/,
      /\/?\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/
    ), optional(/[;,]/)),

    reference: $ => choice(
      /&<[^>]+>(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/,
      /&\/?\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/
    ),

    number: $ => /\d*\.?\d+[d]?/,

    expression: $ => choice(
      $.number,
      $.reference,
      $.unary_expression,
      $.binary_expression,
    ),
    unary_expression: $ => choice(
      prec.left(PREC.urary, seq('(', $.expression, ')')),
      prec.left(PREC.urary, seq('-', $.expression)),
    ),
    binary_expression: $ => choice(
      prec.left(PREC.mul, seq(field('left', $.expression), field('operator', choice("*", "/")), field('right', $.expression))),
      prec.left(PREC.add, seq(field('left', $.expression), field('operator', choice("+", "-")), field('right', $.expression))),
    ),

    identifier: $ => /\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*/,
    value: $ => choice(
      alias(token(seq('"', /[^"]*/, '"')), $.string),
      alias(token(seq('@"', /[^"]*/, '"')), $.virbatim),
      alias($._list_as_value, $.list),
      alias($._block_as_value, $.block),
      $.expression
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