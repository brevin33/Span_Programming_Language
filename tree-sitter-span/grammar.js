/**
 * @file Span programming language tree-sitter
 * @author Brevin Bell
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "span",

  externals: $ => [
    $.end_of_statement
  ],

  rules: {
    source_file: $ => repeat($.top_level_statment),

    top_level_statment: $ => choice(
      $.function,
    ),

    inner_statment: $ => choice(
    ),

    scope: $ => seq(
      '{',
      repeat($.inner_statment),
      '}',
    ),

    struct_definition: $ => seq(
      'struct',
      $.identifier,
      optional($.template_definition),
      // TODO: more
    ),

    function: $ => seq(
      field("return_type", $.type),
      field("method_type", optional(seq($.type, '.'))),
      field("name", $.identifier),
      optional($.template_definition),
      $.function_definition_parameter_list,
      $.scope,
    ),

    function_definition_parameter_list: $ => seq(
      '(',
      optional(seq(
        $.function_definition_parameter,
        repeat(seq(
          ',',
          $.function_definition_parameter
        ))
      )),
      ')',
    ),

    function_definition_parameter: $ => seq(
      $.type,
      $.identifier,
    ),

    type: $ => prec.right(choice(
      seq(
        $.identifier,
        optional($.type_modifier),
      ),
      seq(
        '(',
        $.identifier,
        $.type_modifier,
        ')',
      ),
    )),

    type_modifier: $ => choice(
        '*',
        '&',
        '^',
        '\'',
        seq(
          '[',
          optional($.number),
          ']'
        ),
        seq(
          '{',
          $.type,
          '}'
        ),
        $.type_modifier_implicit_struct,
        $.type_modifier_implicit_enum,
    ),

    type_modifier_implicit_struct: $ => prec.left(repeat1(seq(
      ',',
      $.type,
    ))),

    type_modifier_implicit_enum: $ => prec.left(repeat1(seq(
      '|',
      $.type,
    ))),

    template_definition: $ => seq(
      '<',
      $.tempalte_definition_argument,
      '>',
    ),

    tempalte_definition_argument: $ => seq(
      $.identifier,
      optional(seq(
        ':',
        $.type,
      ))
    ),

    template_instance: $ => seq(
      '<',
      repeat1($.type),
      '>',
    ),




    number: $ => /\d+(\.\d+)?/,
    identifier: $ => /[a-zA-Z_]\w*/,
  }
});

