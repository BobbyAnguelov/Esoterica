import re
from typing import Optional, Callable, Any, Iterable, NamedTuple, Union, List, Dict, Tuple
import typing

try:
    regex_type = re.Pattern
except AttributeError:
    try:
        regex_type = re.RegexObject
    except AttributeError:
        regex_type = type(re.compile(''))

Matcher = Callable[[str, int], int]

def make_regex_matcher(regex) -> Matcher:
    def matcher(text: str, begin: int) -> int:
        m = regex.match(text, begin)
        if m:
            return m.end()
        else:
            return -1
    return matcher

def make_literal_matcher(literal) -> Matcher:
    def matcher(text: str, begin: int) -> int:
        if text.startswith(literal, begin):
            return begin + len(literal)
        else:
            return -1
    return matcher

def never_matcher(text: str, begin: int) -> int:
    return -1

def always_one_matcher(text: str, begin: int) -> int:
    return begin + 1

class Location(NamedTuple):
    filename: str
    source: str
    begin: int
    end: int
    line: int
    column: int

    def __str__(self):
        if self.filename:
            return f"{self.filename}:{self.line}:{self.column}"
        else:
            return f"{self.line}:{self.column}"

class ParserHint(NamedTuple):
    location: Location
    message: str

class ParseError(Exception):
    def __init__(self, loc: Location, message: str, hints: Iterable[ParserHint]):
        msg = f"{loc}: {message}"
        if hints:
            lines = (f".. while parsing {h.message} at {h.location}" for h in reversed(hints))
            msg = msg + "\n" + "\n".join(lines)
        super().__init__(msg)
        self.loc = loc

class Rule:
    """Rule to match tokens

    """
    def __init__(self, name: str, matcher: Matcher=never_matcher, literal: str="", value:Any=None, ignore:bool=False):
        self.name = name
        self.matcher = matcher
        self.literal = literal
        if value is None or callable(value):
            self.valuer = value
        else:
            self.valuer = lambda s: value
        self.ignore = bool(ignore)

    def __repr__(self):
        return "Rule({!r})".format(self.name)

    def __str__(self):
        return "{!r}".format(self.name)

# Special rules for begin and end
Begin = Rule("begin-of-file")
End = Rule("end-of-file")
Synthetic = Rule("synthetic")
Error = Rule("error")

def make_matcher_from_pattern(pattern: Any) -> Matcher:
    if isinstance(pattern, str):
        # Compile strings to regex
        regex = re.compile(pattern, re.ASCII)
        return make_regex_matcher(regex)
    elif isinstance(pattern, regex_type):
        # Already compiled regex
        return make_regex_matcher(pattern)
    elif callable(pattern):
        # Custom matcher function
        return pattern
    else:
        raise TypeError('Invalid type for rule pattern {!r}'.format(type(pattern)))

class Lexer(object):
    def __init__(self):
        self.global_rules = []
        self.prefix_rules = {}
        self.lexer_type = SourceLexer

    def add_rule(self, rule: Rule, prefix:Iterable[str]=None):
        if prefix:
            for pre in prefix:
                if not isinstance(pre, str):
                    raise TypeError("Prefixes must be an iterable of str")
                if len(pre) > 1:
                    raise ValueError('Prefixes must be single characters')
                rules = self.prefix_rules.setdefault(pre, [])
                rules.append(rule)
        else:
            self.global_rules.append(rule)

    def rule(self, name: str, pattern: Any, *, value:Any=None, prefix:Optional[Iterable[str]]=None):
        matcher = make_matcher_from_pattern(pattern)
        rule = Rule(name, matcher, "", value, ignore=False)
        self.add_rule(rule, prefix)
        return rule

    def ignore(self, name: str, pattern: Any, *, value:Any=None, prefix:Optional[Iterable[str]]=None):
        matcher = make_matcher_from_pattern(pattern)
        rule = Rule(name, matcher, "", value, ignore=True)
        self.add_rule(rule, prefix)
        return rule

    def ignore_whitespace(self, *, ignore_newline=True):
        spaces = " \t\v\r"
        if ignore_newline:
            spaces += "\n"
        regex = re.compile(f"[{re.escape(spaces)}]+")
        self.ignore("whitespace", regex, prefix=spaces)

    def literal(self, literal: str, value: Any=None):
        if not isinstance(literal, str):
            raise TypeError('Literals must be strings, got {!r}'.format(type(literal)))
        if not literal:
            raise ValueError('Empty literal')
        if len(literal) == 1:
            # Prefix match is full match
            matcher = always_one_matcher
        else:
            matcher = make_literal_matcher(literal)
        rule = Rule(repr(literal), matcher, literal, value)
        self.add_rule(rule, literal[0])
        return rule

    def literals(self, *args: str):
        return [self.literal(arg) for arg in args]
    
    def make(self, source: str, filename: str=""):
        return self.lexer_type(self, source, filename)

class Token:
    __slots__ = ["rule", "location", "value", "_text"]

    def __init__(self, rule: Rule, location: Location):
        self.rule = rule
        self.location = location
        self.value = None
        self._text = None

        if rule.valuer:
            self.value = rule.valuer(self.text())

    def text(self) -> str:
        if self._text is None:
            loc = self.location
            self._text = loc.source[loc.begin:loc.end]
        return self._text

    def __str__(self) -> str:
        loc = self.location
        length = loc.end - loc.begin
        if self.rule.literal or length > 20:
            return self.rule.name
        else:
            return f"{self.rule.name} {self.text()!r}"

    def __repr__(self) -> str:
        return f"Token({self.rule.name!r}"

def synthetic(text: str):
    length = len(text)
    loc = Location("", text, 0, length, 1, 1)
    return Token(Synthetic, loc)

class SourceLexer:
    def __init__(self, lexer: Lexer, source: str, filename:str=""):
        self.pos = 0
        self.lexer = lexer
        self.source = source
        self.source_length = len(source)
        self.filename = filename
        self.line = 1
        self.line_end = 0

    def scan(self) -> Token:
        pos = self.pos
        source_end = self.source_length
        lexer = self.lexer
        source = self.source
        global_rules = lexer.global_rules

        while pos < source_end:
            prefix = source[pos]
            prefix_rules = lexer.prefix_rules.get(prefix)

            best_rule = None
            best_end = -1

            if prefix_rules:
                for rule in prefix_rules:
                    end = rule.matcher(source, pos)
                    if end >= best_end:
                        best_rule = rule
                        best_end = end

            for rule in global_rules:
                end = rule.matcher(source, pos)
                if end >= best_end:
                    best_rule = rule
                    best_end = end
            
            column = pos - self.line_end + 1
            while self.line_end < best_end:
                line_end = source.find("\n", self.line_end, best_end)
                if line_end < 0: break
                self.line_end = line_end + 1
                self.line += 1
            
            if best_end < 0:
                loc = Location(self.filename, source, pos, pos + 1, self.line, column)
                return Token(Error, loc)
        
            if best_rule.ignore:
                pos = best_end
            else:
                self.pos = best_end
                loc = Location(self.filename, source, pos, best_end, self.line, column)
                return Token(best_rule, loc)

        loc = Location(self.filename, source, source_end, source_end + 1, self.line + 1, 1)
        return Token(End, loc)

def format_rule(rule):
    if isinstance(rule, list):
        return 'any of ({})'.format(', '.join(format_rule(r) for r in rule))
    elif isinstance(rule, Rule):
        return rule.name
    elif isinstance(rule, str):
        return repr(rule)
    else:
        raise TypeError(f'Unsupported rule type {repr(type(rule))}')

def format_message(msg):
    return " " + msg if msg else ""

class ParserHintContext:
    def __init__(self, parser: "Parser", token_or_loc: Union[Token, Location], message: str):
        self.parser = parser
        if hasattr(token_or_loc, "location"):
            self.location = token_or_loc.location
        else:
            self.location = token_or_loc
        self.message = message

    def __enter__(self):
        self.parser.hint_stack.append(ParserHint(self.location, self.message))

    def __exit__(self, type, value, traceback):
        self.parser.hint_stack.pop()

class Parser:
    def __init__(self, lexer: Lexer, source: str, filename:str=""):
        begin_loc = Location(filename, source, 0, 0, 1, 1)
        self.lexer = lexer
        self.source_lexer = lexer.make(source, filename)
        self.prev_token = Token(Begin, begin_loc)
        self.token = self.source_lexer.scan()
        self.hint_stack = []

    def scan(self):
        if self.token.rule is not End:
            self.prev_token = self.token
            self.token = self.source_lexer.scan()
            if self.token.rule is Error:
                self.fail(f"Bad token starting with {self.token.text()!r}")
        return self.prev_token

    def peek(self, rule: Any) -> Optional[Token]:
        if isinstance(rule, list):
            for r in rule:
                tok = self.peek(r)
                if tok: return tok
        elif isinstance(rule, Rule):
            if self.token.rule == rule:
                return self.token
        elif isinstance(rule, str):
            if self.token.rule.literal == rule:
                return self.token
        else:
            raise TypeError(f'Unsupported rule type {type(rule)!r}')

    def accept(self, rule) -> Optional[Token]:
        tok = self.peek(rule)
        if tok:
            self.scan()
            return tok
        else:
            return None
    
    def fail_at(self, location: Location, message: str):
        raise ParseError(location, message, self.hint_stack)

    def fail(self, message: str):
        self.fail_at(self.token.location, message)

    def fail_prev(self, message: str):
        self.fail_at(self.prev_token.location, message)

    def fail_got(self, message: str):
        self.fail_at(self.token.location, message + f", got {self.token}")

    def fail_prev_got(self, message: str):
        self.fail_at(self.prev_token.location, message + f", got {self.prev_token}")

    def require(self, rule, message: str="") -> Token:
        tok = self.accept(rule)
        if tok:
            return tok
        else:
            fr, fm = format_rule, format_message
            self.fail_got(f"Expected {fr(rule)}{fm(message)}")

    def sep(self, sep, message="") -> Iterable[int]:
        n = 0
        yield n
        while self.accept(sep):
            yield n
            n += 1

    def until(self, end, message="") -> Iterable[int]:
        n = 0
        while not self.accept(end):
            yield n
            n += 1

    def sep_until(self, sep, end, message="") -> Iterable[int]:
        n = 0
        while not self.accept(end):
            if n > 0 and not self.accept(sep):
                fr, fm = format_rule, format_message
                self.fail_got(f"Expected {fr(sep)} or {fr(end)}{fm(message)}")
            yield n
            n += 1
    
    def ignore(self, rule) -> int:
        n = 0
        while self.accept(rule):
            n += 1
        return n

    def hint(self, token_or_loc: Union[Token, Location], message: str):
        return ParserHintContext(self, token_or_loc, message)

get_origin = getattr(typing, "get_origin", lambda o: getattr(o, "__origin__", None))
get_args = getattr(typing, "get_args", lambda o: getattr(o, "__args__", None))

class AstField(NamedTuple):
    name: str
    base: type
    optional: bool
    sequence: bool

def make_ast_field(name, base):
    origin, args = get_origin(base), get_args(base)
    optional = sequence = False
    if origin == Union and len(args) == 2 and type(None) in args:
        base = args[args.index(type(None)) ^ 1]
        optional = True
        origin, args = get_origin(base), get_args(base)
    if origin == List:
        base = args[0]
        sequence = True
    elif origin:
        base = object
    return AstField(name, base, optional, sequence)

class Ast:
    def __init__(self, *args, **kwargs):
        cls = type(self)
        if len(args) > len(cls.fields):
            raise TypeError(f"Too many fields for {cls.__name__}: {len(args)}, expected {len(cls.fields)}")
        for field, arg in zip(cls.fields, args):
            setattr(self, field.name, arg)
        for name, arg in kwargs.items():
            setattr(self, name, arg)
        for field in cls.fields:
            try:
                value = getattr(self, field.name)
                if field.optional and value is None:
                    continue
                if field.sequence:
                    for ix, v in enumerate(value):
                        if not isinstance(v, field.base):
                            raise TypeError(f"Trying to assign '{type(v).__name__}' to '{cls.__name__}' field '{field.name}: {field.base.__name__}' index [{ix}]")
                else:
                    if not isinstance(value, field.base):
                        raise TypeError(f"Trying to assign '{type(value).__name__}' to '{cls.__name__}' field '{field.name}: {field.base.__name__}'")
            except AttributeError:
                raise ValueError(f"'{cls.__name__}' requires field '{field.name}: {field.base.__name__}'")

    def __init_subclass__(cls, **kwargs):
        fields = getattr(cls, "__annotations__", {})
        cls.fields = [make_ast_field(k, v) for k,v in fields.items()]
        super().__init_subclass__(**kwargs)

    def _imp_dump(self, result, indent):
        cls = type(self)
        indent_str = "  " * indent
        result += (cls.__name__, "(")
        first = True
        num_asts = 0
        for field in cls.fields:
            if issubclass(field.base, Ast):
                num_asts += 1
                continue
            if not first: result.append(", ")
            first = False
            result += (field.name, "=", str(getattr(self, field.name, None)))

        for field in cls.fields:
            if not issubclass(field.base, Ast): continue

            if num_asts > 1:
                result += ("\n", indent_str, "  ")
            else:
                if not first: result.append(", ")

            result.append(field.name)
            result.append("=")
            attr = getattr(self, field.name, None)
            if not attr:
                result.append("None")
                continue

            if field.sequence:
                result.append("[")
                seq_indent = 1 if num_asts == 1 else 2
                print(seq_indent)
                for ast in getattr(self, field.name, None):
                    result += ("\n", indent_str, "  " * seq_indent)
                    ast._imp_dump(result, indent + seq_indent)
                result += ("\n", indent_str, "  ]")
            else:
                attr._imp_dump(result, indent + 1)
        
        result += ")"

    def dump(self, indent=0):
        result = []
        self._imp_dump(result, indent)
        return "".join(result)
