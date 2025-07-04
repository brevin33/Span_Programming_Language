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
      $.expression_statement,
    ),

    inner_statment: $ => choice(
      $.expression_statement,
      $.return_statement,
      $.if_statement,
      $.scope_continue_statement,
      $.continue_statement,
      $.scope_break_statement,
      $.break_statement,
      $.while_statement,
      $.for_statement,
      $.switch_statement,
      $.scope,
      $.yeild_statement,
      $.assignment_statement,
    ),


    return: $ => 'return',
    if: $ => 'if',
    else: $ => 'else',
    for: $ => 'for',
    while: $ => 'while',
    switch: $ => 'switch',
    case: $ => 'case',
    yield: $ => 'yield',
    break: $ => 'break',
    sbreak: $ => 'sbreak',
    continue: $ => 'continue',
    scontinue: $ => 'scontinue',
    in: $ => 'in',

    assignment: $ => seq(
      $.assignment_assignee,
      repeat(seq(
        ',',
        $.assignment_assignee,
      )),
      '=',
      $.expression,
    ),

    assignment_assignee: $ => choice(
        $.expression,
        $.variable_declaration,
      ),

    variable_declaration: $ => seq(
      $.type,
      $.identifier,
    ),

    assignment_statement: $ => seq(
      $.assignment,
      $.end_of_statement,
    ),

    while_statement: $ => seq(
      $.while,
      $.expression,
      $.scope,
    ),

    switch_statement: $ => seq(
      $.switch,
      $.expression,
      $.switch_scope,
    ),

    yeild_statement: $ => seq(
      $.yield,
      $.expression,
      $.end_of_statement,
    ),

    case_statement: $ => seq(
      $.case,
      $.expression,
      $.scope,
    ),

    switch_scope: $ => seq(
      '{',
      repeat($.case_statement),
      '}',
      $.end_of_statement,
    ),

    for_statement: $ => seq(
      $.for,
      $.identifier,
      $.in,
      $.expression,
      $.scope,
    ),

    return_statement: $ => seq(
      $.return,
      $.expression,
      $.end_of_statement,
    ),

    if_statement: $ => seq(
      $.if,
      $.expression,
      $.scope,
      optional($.else_statement),
    ),

    scope_continue_statement: $ => seq(
      $.scontinue,
      optional($.number),
      $.end_of_statement,
    ),
      
    continue_statement: $ => seq(
      $.continue,
      optional($.number),
      $.end_of_statement,
    ),

    scope_break_statement: $ => seq(
      $.sbreak,
      optional($.number),
      $.end_of_statement,
    ),

    break_statement: $ => seq(
      $.break,
      optional($.number),
      $.end_of_statement,
    ),

    else_statement: $ => seq(
      $.else,
      choice(
        $.if_statement,
        $.scope,
      ),
    ),

    expression_statement: $ => seq(
      $.expression,
      $.end_of_statement,
    ),

    boolean: $ => choice('true', 'false'),

    expression: $ => prec(1, choice(
      $.number,
      $.boolean,
      $.char,
      $.identifier,
      $.string,
      $.function_call,
      $.member_access,
      $.type_data_access,
      $.parenthesized_expression,
      $.biop,
      $.method_call,
      $.grouped_expression,
    )),

    grouped_expression: $ => prec.left(2, seq(
      $.expression,
      repeat1(seq(
        ',',
        $.expression,
      )),
    )),

    type_data_access: $ => seq(
      $.type,
      '.',
      $.identifier,
    ),

    biop: $ => choice(
      prec.left(9, seq($.expression, choice('*', '/', '%'), $.expression)),
      prec.left(8, seq($.expression, choice('+', '-'), $.expression)),
      prec.left(7, seq($.expression, '^', $.expression)),
      prec.left(6, seq($.expression, choice('&', '|'), $.expression)),
      prec.left(5, seq($.expression, choice('==', '!=', '<', '<=', '>', '>='), $.expression)),
      prec.left(4, seq($.expression, choice('&&', '||'), $.expression)),
    ),

    parenthesized_expression: $ => seq(
      '(',
      $.expression,
      ')',
    ),

    function_call: $ => seq(
      field("function", $.identifier),
      $.function_call_parameter_list,
    ),

    method_call: $ => seq(
      field("object", $.expression),
      '.',
      $.function_call,
    ),

    member_access: $ => prec(2, seq(
      field("object", $.expression),
      '.',
      field("member", $.identifier),
    )),

    function_call_parameter_list: $ => seq(
      '(',
      optional(seq(
        $.function_call_parameter,
        repeat(seq(
          ',',
          $.function_call_parameter
        ))
      )),
      ')',
    ),

    function_call_parameter: $ => seq(
      $.expression,
    ),

    scope: $ => seq(
      '{',
      repeat($.inner_statment),
      '}',
      $.end_of_statement,
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


    string_open: $ => '"',
    string_close: $ => '"',
    string_open_multiline: $ => '"""',
    string_close_multiline: $ => '"""',

    string_content: $ => choice(
      /[^"\n\r\\{]/,
      $.escape_sequence,
      seq(
        /\\/,
        '{'
      ),
    ),

    string_content_multiline: $ => choice(
      /[^"{\\]/,
      $.escape_sequence,
      seq(
        /\\/,
        '{'
      ),
    ),

    escape_sequence: $ => /\\./,

    string: $ => choice(
      seq(
        $.string_open,
        repeat(choice(
          $.string_content,
          $.interpolation
        )),
        $.string_close
      ),
      seq(
        $.string_open_multiline,
        repeat(choice(
          $.string_content_multiline,
          $.interpolation
        )),
        $.string_close_multiline
      )
    ),

    char: $ => seq(
      $.char_open,
      repeat($.char_content),
      $.char_close
    ),

    char_open: $ => "'",
    char_close: $ => "'",

    char_content: $ => choice(
      /[^'\\\n\r]/,
      $.escape_sequence,
    ),

    interpolation_open: $ => '{',
    interpolation_close: $ => '}',

    interpolation: $ => seq(
      $.interpolation_open,
      $.expression,
      $.interpolation_close
    ),

  }
});

