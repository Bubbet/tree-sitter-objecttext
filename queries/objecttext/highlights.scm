(identifier) @variable
(assignment
    "=" @operator)
(block
    "=" @operator)
(list
    "=" @operator)

(block
    ":" @operator)
(list
    ":" @operator)

[
"*"
"/"
"+"
"-"
] @operator

[
";"
","
] @punctuation.delimiter

[
"("
")"
"["
"]"
"{"
"}"
] @punctuation.bracket

(string) @string
(number) @number
(bool) @boolean
(bare_string) @string
(comment) @comment
[
    (reference)
    (extension)
] @constant