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
    /[\t\n\r \\]/,
    $.comment,
  ],
  rules: {
    // TODO: add the actual grammar rules
    source_file: $ => choice(
      seq($.identifier, ':', repeat($.extension), choice($.list, $.group)),
      seq($.identifier, optional('='), choice(
        $.list,
        $.group,
      )),
      seq($.identifier, '=', $.value),
    ),
    _block_value: $ => choice(
      seq(optional($.identifier), ':', repeat1($.extension), choice($.list, $.group)),
      seq(optional(seq($.identifier, optional('='))), choice(
        $.list,
        $.group,
      )),
      seq($.identifier, '=', $.value),
    ),
    extension: $ => choice(
      seq($.path_reference, /[;,]?/),
      seq($.internal_reference, /[;,]?/),
    ),
    internal_reference: $ => seq($.identifier, repeat(seq('/', $.identifier))),
    path_reference: $ => seq('<', /.+/, '>', repeat(seq('/', $.identifier))),
    reference: $ => seq('&', choice(
      $.internal_reference,
      $.path_reference,
    )),
    value: $ => choice(
      $.reference,
      $.string,
      $.verbatim,
    ),
    // Periods can start identifiers but only when followed by another char two dots in a row at the start is not allowed.
    identifier: $ => token(choice(
      seq(/[A-Za-z0-9_]/, token.immediate(/[A-Za-z0-9_\.]*/)), // Normal Identifiers
      seq('.', token.immediate(/[A-Za-z0-9_]/), token.immediate(/[A-Za-z0-9_\.]*/)), // Identifiers starting with a period.
    )),
    comment: $ => token(
      choice(seq("//", /.*/), seq("/*", /[^*]*\*+([^/*][^*]*\*+)*/, "/")),
    ),
    string: $ => choice(
      '""',
      seq('"', repeat(token.immediate(choice(/[^\n"]/, /\\"/))), token.immediate('"')),
    ),
    verbatim: $ => seq('@"', /[^"]*/, '"'),

    _list: $ => seq('[', repeat($._block_value), ']'),
    _group: $ => seq('{', repeat($._block_value), '}'),
  }
});
