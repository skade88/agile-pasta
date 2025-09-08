# String Literal Syntax Test

This demonstrates the new quoted string literal syntax that resolves ambiguity in rules parsing.

## Before (Problematic)
```
FIELD|full_name|first_name + ' ' + last_name|Spaces might not be preserved
```

## After (Clear and Unambiguous)
```
FIELD|full_name|first_name + " " + last_name|Spaces are preserved
FIELD|greeting|"Hello, " + name + "!"|Complex strings with punctuation
```

## Backward Compatibility
Both single and double quotes are supported, but double quotes are recommended for new rules.