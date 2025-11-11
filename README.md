# c0c1
c compiler

## implementations
1. `lexing` : uses [lexer generator](git@github.com:basilkottilingal/lexer.git)
  for preprocessing and lexing.
```bash
git subtree add --prefix=lexer git@github.com:basilkottilingal/lexer.git v1.0 --squash
```
2. 'parsing' : uses GNU bison for parsing and create AST
3. The rest is not implemented.
