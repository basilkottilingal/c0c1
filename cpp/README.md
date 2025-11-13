
# Translation phases in C

1. Raw sourcec code 
  - replace trigraphs.

| Replace |  Replace by |
| ----    |  ------     |
| ??=	| `#`	hash        |
| ??(	| `[`	left bracket |
| ??/	| `\`	backslash   |
| ??)	| `]`	right bracket |
| ??'	| `^`	caret       |
| ??<	| `{`	left brace  |
| ??!	| `\|` pipe       |
| ??>	| `}`	right brace |
| ??- | `~` tilde       |
       Trigraphs are obsolete in c11 and removed in c23 altogether
   - make sure source code characters are in [0x00,0xFF] (utf-8).
   - optionally you can replace

| Replace |  Replace by |
| ----    |  ------     |
| `\r\n`  | `\n`        |
| `\r`    |   `\n`      |
| `\f`    |  ` `        |        
       This is optional. Instead you can take [\r\t\f\v] as white space
       during lexing.
2. Remove `\\\n`

   It's a guaranteed feature in ISO C (across all versions).
   Note : replace the following digraphs 

| Replace |  Replace by |
| ----    |  ------     |
|    %:	  |   `#`  |
|    %:%:	|   `##` |
  
  Note : 
  - `%:` is replaced by `#` only if it is at the beginning of a line or
it satisfies the regex pattern `^[ \t]*%:`.
  - `%:%:` is replaced by `##` only if it is a properly placed
concate operator in the  `#define` macro definition


3. Replace `//.*` and `/*...*/` with empty spaces of equal length
4. Preprocess # directives.
5. Concatenate {STRING}({WS}*{STRING})* into a single string
6. Lexing and parsing (AST creation) and compiling.
   Note : apart from the most common C-lexicons don't forget to parse
   the following digraphs during lexing

| Replace |  Replace by |
| ----    |  ------     |
|    <%	  |  {  |
|    %>	| } |
|    <:	| [ |
|    :>	| ] |
   These digraphs are still part of C
7. Linking
  

