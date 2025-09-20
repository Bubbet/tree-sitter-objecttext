/**
 * @file Halfling Engine's Object Text. Used in Cosmoteer.
 * @author Bubbet <objecttext@bubbet.dev>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const PREC = {
  call: 5,
  parenthesis: 4,
  urinary: 3,
  multiplicative: 2,
  additive: 1,
}

module.exports = grammar({
  name: "objecttext",
  extras: $ => [
    /[\s]/,
    $.comment,
  ],
  supertypes: $ => [
    $.expression,
  ],
  rules: {
    // TODO: add the actual grammar rules
    source_file: $ => repeat1($._assignment),

    // Periods can start identifiers but only when followed by another char two dots in a row at the start is not allowed.
    identifier: $ => prec(-1, /\.?[A-Za-z0-9_][A-Za-z0-9_\.]*/),
    comment: $ => token(
      choice(seq("//", /.*/), seq("/*", /[^*]*\*+([^/*][^*]*\*+)*/, "/")),
    ),
    _path: $ => alias(token.immediate(/\/\.?[A-Za-z0-9_][A-Za-z0-9_\.]*/), $.identifier),
  

    internal_reference: $ => seq(alias(/&\/?\.?[A-Za-z0-9_][A-Za-z0-9_\.]*/, $.identifier), repeat($._path)),
    path_reference: $ => seq(/&<[^>]+>/, repeat($._path)),
    reference: $ => prec.right(choice(
      $.internal_reference,
      $.path_reference,
    )),
    value: $ => choice(
      $.bare_word,
      $.expression,
      $.string,
      $.verbatim,
    ),
    bare_word: $ => token.immediate(prec(-2, /\s*[^\[{"].*/)),
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

    group: $ => seq($.identifier, optional(choice('=', seq(':', repeat1($.extension)))), $._group),
    list: $ => seq($.identifier, optional(choice('=', seq(':', repeat1($.extension)))), $._list),


    internal_extension: $ => seq(alias(/\/?\.?[A-Za-z0-9_][A-Za-z0-9_\.]*/, $.identifier), repeat($._path)),
    path_extension: $ => seq(/<[^>]+>/, repeat($._path)),
    extension: $ => prec.right(choice(
      seq($.internal_extension, /[;,]?/),
      seq($.path_extension, /[;,]?/),
    )),

    _list: $ => seq('[', repeat(seq(choice(
      $._assignment,
      $.value,
      alias(seq(optional(seq(':', repeat1($.extension))), $._group), $.group),
      alias(seq(optional(seq(':', repeat1($.extension))), $._list), $.list),
    ), /[;,]?/)), ']'),
    _group: $ => seq('{', repeat(seq(choice(
      $._assignment,
      alias(seq(optional(seq(':', repeat1($.extension))), $._group), $.group),
      alias(seq(optional(seq(':', repeat1($.extension))), $._list), $.list),
    ), /[;,]?/)), '}'),

    expression: $ => choice(
      $.number,
      $.reference,
      $.binary_expression,
      $.urinary_expression,
      $.parenthesized_expression,
      $.function_call,
    ),
    urinary_expression: $ => prec(PREC.urinary, seq('-', $.expression)),
    binary_expression: $ => prec.left(choice(
      prec(PREC.multiplicative, seq($.expression, choice('*', '/'), $.expression)),
      prec(PREC.additive, seq($.expression, choice('+', '-'), $.expression)),
    )),
    parenthesized_expression: $ => prec(PREC.parenthesis, seq('(', $.expression, ')')),
    function_call: $ => prec(PREC.call, seq( // TODO make this not include \( in the identifier match
      field("name", alias(/\.?[A-Za-z0-9_][A-Za-z0-9_\.]*\(/, $.identifier)),
      commaSep($.expression),
      ')',
    )),
    number: $ => token(prec(10, /-?\d*\.?\d+%?d?/)),
  }
});

/**
 * @param {RuleOrLiteral} rule
 */
function commaSep(rule) {
  return optional(seq(rule, repeat(seq(',', rule))));
}