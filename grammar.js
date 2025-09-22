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
  unary: 3,
  mul: 2,
  add: 1,
}
const IDENTIFIER = "\\.?[a-zA-Z0-9_][a-zA-Z0-9_\\.]*"
const PATH_FILE = "<[^>]+>(\/" + IDENTIFIER + "\)*" //"<[^>]+>(\/" + IDENTIFIER + ")*"
const LEADING_GROUP = "\(\([.~]\/\)|\/\)?"
const INTERNAL_IDENTIFIER = "\(" + IDENTIFIER + "|\\^|\(\\.\\.\))"
const PATH_INTERNAL = LEADING_GROUP + INTERNAL_IDENTIFIER + "\(\\/" + INTERNAL_IDENTIFIER + "\)*" //"(~|\.|(\.\.))?\/?(" + IDENTIFIER + ")|\^(\/(" + IDENTIFIER + ")|\^)*"

module.exports = grammar({
  name: "objecttext",
  extras: $ => [
    /\s/,
    $.comment
  ],
  supertypes: $ => [
    $.expression,
  ],
  conflicts: $ => [
    [$.value, $._block_as_value, $._list_as_value],
    [$.value, $._block_as_value],
    [$.value, $._list_as_value],
  ],
  externals: $ => [
    $.bare_string,
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
      $.bare_string,
    ),

    list: $ => prec.right(PREC.assignment, seq(field("key", $.identifier), optional(choice('=', seq(':', repeat1($.extension)))), $._list)),
    block: $ => prec.right(PREC.assignment, seq(field("key", $.identifier), optional(choice('=', seq(':', repeat1($.extension)))), $._block)),
    _list_as_value: $ => prec.right(seq(optional(field("key", $.identifier)), optional(choice('=', seq(':', repeat1($.extension)))), $._list)),
    _block_as_value: $ => prec.right(seq(optional(field("key", $.identifier)), optional(choice('=', seq(':', repeat1($.extension)))), $._block)),
    _list: $ => seq("[", repeat($._list_value), "]"),
    _block: $ => seq("{", repeat($._block_value), "}"),
    _list_value: $ => seq(choice($._assignment, $.value), optional(choice(',', ';'))),
    _block_value: $ => seq($._assignment, optional(choice(',', ';'))),

    extension: $ => seq(choice(
      new RegExp(PATH_FILE),
      new RegExp(PATH_INTERNAL),
    ), optional(/[;,]/)),

    reference: $ => choice(
      new RegExp("&" + PATH_FILE),
      new RegExp("&" + PATH_INTERNAL),
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
      prec.left(PREC.unary, seq('(', $.expression, ')')),
      prec.left(PREC.unary, seq('-', $.expression)),
    ),
    binary_expression: $ => choice(
      prec.left(PREC.mul, seq(field('left', $.expression), field('operator', choice("*", "/")), field('right', $.expression))),
      prec.left(PREC.add, seq(field('left', $.expression), field('operator', choice("+", "-")), field('right', $.expression))),
    ),
    // TODO this techinically isnt part of ObjectText, it just parses the bare_word as a expression using mxparser
    // Though, ObjectText does replace references inside that bareword with the real value.
    function_call: $ => prec.left(PREC.func, seq($.identifier, token.immediate('('), repeat1(seq($.expression, optional(','))), ')')),

    bool: $ => choice('true', 'false'),
    //bare_string: $ => token(prec(-2, /[^\n]+/)),
    identifier: $ => token(prec(-2, new RegExp(IDENTIFIER))),
    value: $ => choice(
      alias(token(seq('"', /([^"]|(\\"))*/, '"')), $.string),
      alias(token(seq('@"', /([^"]|(\\"))*/, '"')), $.virbatim),
      alias($._list_as_value, $.list),
      alias($._block_as_value, $.block),
      $.expression,
      $.identifier,
      $.bool,
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