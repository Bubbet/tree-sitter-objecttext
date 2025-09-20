/**
 * @file Halfling Engine's Object Text. Used in Cosmoteer.
 * @author Bubbet <objecttext@bubbet.dev>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const PREC = {
  assignment: 5,
  func: 4,
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
      $.assignment,
      $.block,
      $.list,
    ),

    assignment: $ => prec(PREC.assignment, seq(field("key", $.identifier), "=", field("value", $._assignment_value))),
    _assignment_value: $ => choice(
      $.value,
      $.bare_word,
    ),
    list: $ => prec.right(PREC.assignment, seq(field("key", $.identifier), optional(choice('=', seq(':', repeat1($.extension)))), "[", repeat($._list_value), "]")),
    block: $ => prec.right(PREC.assignment, seq(field("key", $.identifier), optional(choice('=', seq(':', repeat1($.extension)))), "{", repeat($._block_value), "}")),
    _list_as_value: $ => prec.right(seq(optional(field("key", $.identifier)), optional(choice('=', seq(':', repeat1($.extension)))), "[", repeat($._list_value), "]")),
    _block_as_value: $ => prec.right(seq(optional(field("key", $.identifier)), optional(choice('=', seq(':', repeat1($.extension)))), "{", repeat($._block_value), "}")),
    _list_value: $ => seq(choice($._assignment, $.value), optional(',')),
    _block_value: $ => seq($._assignment, optional(',')),

    extension: $ => seq(choice(
      /<[^>]+>(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/,
      /\/?\.?[a-zA-Z0-9_][a-zA-Z0-9_]*(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/
    ), optional(/[;,]/)),

    reference: $ => choice(
      /&<[^>]+>(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/,
      /&\/?\.?[a-zA-Z0-9_][a-zA-Z0-9_]*(\/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*)*/
    ),

    number: $ => /\d*\.?\d+[dr%]?/,
    expression: $ => choice(
      $.number,
      $.reference,
      $.unary_expression,
      $.binary_expression,
      $.function_call,
    ),
    unary_expression: $ => choice(
      prec.left(PREC.urary, seq('(', $.expression, ')')),
      prec.left(PREC.urary, seq('-', $.expression)),
    ),
    binary_expression: $ => choice(
      prec.left(PREC.mul, seq(field('left', $.expression), field('operator', choice("*", "/")), field('right', $.expression))),
      prec.left(PREC.add, seq(field('left', $.expression), field('operator', choice("+", "-")), field('right', $.expression))),
    ),
    // TODO this techinically isnt part of ObjectText, it just parses the bare_word as a expression using mxparser
    // Though, ObjectText does replace references inside that bareword with the real value.
    function_call: $ => prec.left(PREC.func, seq(/\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*\(/, repeat1(seq($.expression, optional(','))), ')')),

    bare_word: $ => token(prec(-2, /[^\n]+/)),
    identifier: $ => token(prec(-3, /\.?[a-zA-Z0-9_][a-zA-Z0-9_\.]*/)),
    value: $ => choice(
      alias(token(seq('"', /([^"]|(\\"))*/, '"')), $.string),
      alias(token(seq('@"', /([^"]|(\\"))*/, '"')), $.virbatim),
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