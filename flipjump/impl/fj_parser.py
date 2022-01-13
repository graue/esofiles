from sly import Lexer, Parser
from os import path
from defs import *


global curr_file, curr_text, error_occurred, curr_namespace, reserved_names


def syntax_error(line, msg=''):
    global error_occurred
    error_occurred = True
    print()
    if msg:
        print(f"Syntax Error in file {curr_file} line {line}:")
        print(f"  {msg}")
    else:
        print(f"Syntax Error in file {curr_file} line {line}")


def syntax_warning(line, is_error, msg=''):
    if is_error:
        global error_occurred
        error_occurred = True
    print()
    print(f"Syntax Warning in file {curr_file}", end="")
    if line is not None:
        print(f" line {line}", end="")
    if msg:
        print(f":")
        print(f"  {msg}")
    else:
        print()


class FJLexer(Lexer):
    tokens = {NS, DEF, REP,
              WFLIP, SEGMENT, RESERVE,
              ID, DOT_ID, NUMBER, STRING,
              LE, GE, EQ, NEQ,
              SHL, SHR,
              NL, SC}

    literals = {'=', '+', '-', '*', '/', '%',
                '(', ')',
                '$',
                '^', '|', '&',
                '?', ':',
                '<', '>',
                '"',
                '#',
                '{', '}',
                "@", ","}

    ignore_ending_comment = r'//.*'

    # Tokens
    DOT_ID = dot_id_re
    ID = id_re
    NUMBER = number_re
    STRING = string_re

    ID[r'def'] = DEF
    ID[r'rep'] = REP
    ID[r'ns'] = NS

    ID[r'wflip'] = WFLIP

    ID[r'segment'] = SEGMENT
    ID[r'reserve'] = RESERVE

    global reserved_names
    reserved_names = {DEF, REP, NS, WFLIP, SEGMENT, RESERVE}

    LE = "<="
    GE = ">="

    EQ = "=="
    NEQ = "!="

    SHL = r'<<'
    SHR = r'>>'

    # Punctuations
    NL = r'[\r\n]'
    SC = r';'

    ignore = ' \t'

    def NUMBER(self, t):
        n = t.value
        if len(n) >= 2:
            if n[0] == "'":
                t.value = handle_char(n[1:-1])[0]
            elif n[1] in 'xX':
                t.value = int(n, 16)
            elif n[1] in 'bB':
                t.value = int(n, 2)
            else:
                t.value = int(n)
        else:
            t.value = int(t.value)
        return t

    def STRING(self, t):
        chars = []
        s = t.value[1:-1]
        i = 0
        while i < len(s):
            val, length = handle_char(s[i:])
            chars.append(val)
            i += length
        t.value = sum(val << (i*8) for i, val in enumerate(chars))
        return t

    def NL(self, t):
        self.lineno += 1
        return t

    def error(self, t):
        global error_occurred
        error_occurred = True
        print()
        print(f"Lexing Error in file {curr_file} line {self.lineno}: {t.value[0]}")
        self.index += 1


class FJParser(Parser):
    tokens = FJLexer.tokens
    # TODO add Unary Minus (-), Unary Not (~). Maybe add logical or (||) and logical and (&&). Maybe handle power (**).
    precedence = (
        ('right', '?', ':'),
        ('left', '|'),
        ('left', '^'),
        ('nonassoc', '<', '>', LE, GE),
        ('left', EQ, NEQ),
        ('left', '&'),
        ('left', SHL, SHR),
        ('left', '+', '-'),
        ('left', '*', '/', '%'),
        ('right', '#'),
    )
    # debugfile = 'src/parser.out'

    def __init__(self, w, warning_as_errors, verbose=False):
        self.verbose = verbose
        self.defs = {'w': Expr(w)}
        self.warning_as_errors = warning_as_errors

        # [(params, quiet_params), statements, (curr_file, p.lineno, ns_name)]
        self.macros = {main_macro: [([], []), [], (None, None, '')]}

    def check_macro_name(self, name, line):
        global reserved_names
        base_name = self.ns_to_base_name(name[0])
        if base_name in reserved_names:
            syntax_error(line, f'macro name can\'t be {name[0]} ({base_name} is a reserved name)!')
        if name in self.macros:
            _, _, (other_file, other_line, _) = self.macros[name]
            syntax_error(line, f'macro {name} is declared twice! '
                               f'also declared in file {other_file} (line {other_line}).')

    def check_params(self, ids, macro_name, line):
        for param_id in ids:
            if param_id in self.defs:
                syntax_error(line, f'parameter {param_id} in macro {macro_name[0]}({macro_name[1]}) '
                                   f'is also defined as a constant variable (with value {self.defs[param_id]})')
        for i1 in range(len(ids)):
            for i2 in range(i1):
                if ids[i1] == ids[i2]:
                    syntax_error(line, f'parameter {ids[i1]} in macro {macro_name[0]}({macro_name[1]}) '
                                       f'is declared twice!')

    def check_label_usage(self, labels_used, labels_declared, params, externs, global_labels, line, macro_name):
        if global_labels & externs:
            syntax_error(line, f"In macro {macro_name[0]}({macro_name[1]}):  "
                               f"extern labels can't be global labels: " + ', '.join(global_labels & externs))
        if global_labels & params:
            syntax_error(line, f"In macro {macro_name[0]}({macro_name[1]}):  "
                               f"extern labels can't be regular labels: " + ', '.join(global_labels & params))
        if externs & params:
            syntax_error(line, f"In macro {macro_name[0]}({macro_name[1]}):  "
                               f"global labels can't be regular labels: " + ', '.join(externs & params))

        # params.update([self.ns_full_name(p) for p in params])
        # externs = set([self.ns_full_name(p) for p in externs])
        # globals.update([self.ns_full_name(p) for p in globals])

        unused_labels = params - labels_used.union(self.ns_to_base_name(label) for label in labels_declared)
        if unused_labels:
            syntax_warning(line, self.warning_as_errors,
                           f"In macro {macro_name[0]}({macro_name[1]}):  "
                           f"unused labels: {', '.join(unused_labels)}.")

        bad_declarations = labels_declared - set(self.ns_full_name(label) for label in externs.union(params))
        if bad_declarations:
            syntax_warning(line, self.warning_as_errors,
                           f"In macro {macro_name[0]}({macro_name[1]}):  "
                           f"Declared a not extern/parameter label: {', '.join(bad_declarations)}.")

        bad_uses = labels_used - global_labels - params - set(labels_declared) - {'$'}
        if bad_uses:
            # print('\nused:', labels_used, 'globals:', globals, 'params:', params)
            syntax_warning(line, self.warning_as_errors,
                           f"In macro {macro_name[0]}({macro_name[1]}):  "
                           f"Used a not global/parameter/declared-extern label: {', '.join(bad_uses)}.")

    @staticmethod
    def ns_name():
        return '.'.join(curr_namespace)

    @staticmethod
    def ns_full_name(base_name):
        return '.'.join(curr_namespace + [base_name])

    @staticmethod
    def dot_id_to_ns_full_name(p):
        base_name = p.DOT_ID
        without_dots = base_name.lstrip('.')
        if len(without_dots) == len(base_name):
            return base_name
        num_of_dots = len(base_name) - len(without_dots)
        if num_of_dots - 1 > len(curr_namespace):
            syntax_error(p.lineno, f'Used more leading dots than current namespace depth '
                                   f'({num_of_dots}-1 > {len(curr_namespace)})')
        return '.'.join(curr_namespace[:len(curr_namespace)-(num_of_dots-1)] + [without_dots])

    @staticmethod
    def ns_to_base_name(name):
        return name.split('.')[-1]

    def error(self, token):
        global error_occurred
        error_occurred = True
        print()
        print(f'Syntax Error in file {curr_file} line {token.lineno}, token=("{token.type}", {token.value})')

    @_('definable_line_statements')
    def program(self, p):
        ops = p.definable_line_statements
        self.macros[main_macro][1] = ops

        # labels_used, labels_declared = all_used_labels(ops)
        # bad_uses = labels_used - set(labels_declared) - {'$'}
        # if bad_uses:
        #     syntax_warning(None, self.warning_as_errors,
        #                    f"Outside of macros:  "
        #                    f"Used a not declared label: {', '.join(bad_uses)}.")

    @_('definable_line_statements NL definable_line_statement')
    def definable_line_statements(self, p):
        if p.definable_line_statement:
            return p.definable_line_statements + p.definable_line_statement
        return p.definable_line_statements

    @_('definable_line_statement')
    def definable_line_statements(self, p):
        if p.definable_line_statement:
            return p.definable_line_statement
        return []

    @_('')
    def empty(self, p):
        return None

    @_('line_statement')
    def definable_line_statement(self, p):
        return p.line_statement

    @_('macro_def')
    def definable_line_statement(self, p):
        return []

    @_('NS ID')
    def namespace(self, p):
        curr_namespace.append(p.ID)

    @_('namespace "{" NL definable_line_statements NL "}"')
    def definable_line_statement(self, p):
        curr_namespace.pop()
        return p.definable_line_statements

    @_('DEF ID macro_params "{" NL line_statements NL "}"')
    def macro_def(self, p):
        params, local_params, global_params, extern_params = p.macro_params
        name = (self.ns_full_name(p.ID), len(params))
        self.check_macro_name(name, p.lineno)
        self.check_params(params + local_params, name, p.lineno)
        ops = p.line_statements
        self.check_label_usage(*all_used_labels(ops), set(params + local_params), set(extern_params),
                               set(global_params), p.lineno, name)
        self.macros[name] = [(params, local_params), ops, (curr_file, p.lineno, self.ns_name())]
        return None

    @_('empty')
    def maybe_ids(self, p):
        return []

    @_('IDs')
    def maybe_ids(self, p):
        return p.IDs

    @_('empty')
    def maybe_local_ids(self, p):
        return []

    @_('"@" IDs')
    def maybe_local_ids(self, p):
        return p.IDs

    @_('empty')
    def maybe_extern_ids(self, p):
        return []

    @_('empty')
    def maybe_global_ids(self, p):
        return []

    @_('"<" ids')
    def maybe_global_ids(self, p):
        return p.ids

    @_('">" IDs')
    def maybe_extern_ids(self, p):
        return p.IDs

    @_('maybe_ids maybe_local_ids maybe_global_ids maybe_extern_ids')
    def macro_params(self, p):
        return p.maybe_ids, p.maybe_local_ids, p.maybe_global_ids, p.maybe_extern_ids

    @_('IDs "," ID')
    def IDs(self, p):
        return p.IDs + [p.ID]

    @_('ID')
    def IDs(self, p):
        return [p.ID]

    @_('line_statements NL line_statement')
    def line_statements(self, p):
        return p.line_statements + p.line_statement

    @_('line_statement')
    def line_statements(self, p):
        return p.line_statement

    # @_('empty')
    # def line_statements(self, p):
    #     return []

    @_('empty')
    def line_statement(self, p):
        return []

    @_('statement')
    def line_statement(self, p):
        if p.statement:
            return [p.statement]
        return []

    @_('label statement')
    def line_statement(self, p):
        if p.statement:
            return [p.label, p.statement]
        return [p.label]

    @_('label')
    def line_statement(self, p):
        return [p.label]

    @_('ID ":"')
    def label(self, p):
        return Op(OpType.Label, (self.ns_full_name(p.ID),), curr_file, p.lineno)

    @_('expr SC')
    def statement(self, p):
        return Op(OpType.FlipJump, (p.expr, next_address()), curr_file, p.lineno)

    @_('expr SC expr')
    def statement(self, p):
        return Op(OpType.FlipJump, (p.expr0, p.expr1), curr_file, p.lineno)

    @_('SC expr')
    def statement(self, p):
        return Op(OpType.FlipJump, (Expr(0), p.expr), curr_file, p.lineno)

    @_('SC')
    def statement(self, p):
        return Op(OpType.FlipJump, (Expr(0), next_address()), curr_file, p.lineno)

    @_('ID')
    def id(self, p):
        return p.ID, p.lineno

    @_('DOT_ID')
    def id(self, p):
        return self.dot_id_to_ns_full_name(p), p.lineno

    @_('ids "," id')
    def ids(self, p):
        return p.ids + [p.id[0]]

    @_('id')
    def ids(self, p):
        return [p.id[0]]

    @_('id')
    def statement(self, p):
        macro_name, lineno = p.id
        return Op(OpType.Macro, ((macro_name, 0), ), curr_file, lineno)

    @_('id expressions')
    def statement(self, p):
        macro_name, lineno = p.id
        return Op(OpType.Macro, ((macro_name, len(p.expressions)), *p.expressions), curr_file, lineno)

    @_('WFLIP expr "," expr')
    def statement(self, p):
        return Op(OpType.WordFlip, (p.expr0, p.expr1, next_address()), curr_file, p.lineno)

    @_('WFLIP expr "," expr "," expr')
    def statement(self, p):
        return Op(OpType.WordFlip, (p.expr0, p.expr1, p.expr2), curr_file, p.lineno)

    @_('ID "=" expr')
    def statement(self, p):
        name = self.ns_full_name(p.ID)
        if name in self.defs:
            syntax_error(p.lineno, f'Can\'t redeclare the variable "{name}".')
        if not p.expr.eval(self.defs, curr_file, p.lineno):
            self.defs[name] = p.expr
            return None
        syntax_error(p.lineno, f'Can\'t evaluate expression:  {str(p.expr)}.')

    @_('REP "(" expr "," ID ")" id')
    def statement(self, p):
        macro_name, lineno = p.id
        return Op(OpType.Rep,
                  (p.expr, p.ID, Op(OpType.Macro, ((macro_name, 0), ), curr_file, lineno)),
                  curr_file, p.lineno)

    @_('REP "(" expr "," ID ")" id expressions')
    def statement(self, p):
        exps = p.expressions
        macro_name, lineno = p.id
        return Op(OpType.Rep,
                  (p.expr, p.ID, Op(OpType.Macro, ((macro_name, len(exps)), *exps), curr_file, lineno)),
                  curr_file, p.lineno)

    @_('SEGMENT expr')
    def statement(self, p):
        return Op(OpType.Segment, (p.expr,), curr_file, p.lineno)

    @_('RESERVE expr')
    def statement(self, p):
        return Op(OpType.Reserve, (p.expr,), curr_file, p.lineno)

    @_('expressions "," expr')
    def expressions(self, p):
        return p.expressions + [p.expr]

    @_('expr')
    def expressions(self, p):
        return [p.expr]

    @_('_expr')
    def expr(self, p):
        return p._expr[0]

    @_('_expr "+" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a + b), p.lineno
        return Expr(('+', (a, b))), p.lineno

    @_('_expr "-" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a - b), p.lineno
        return Expr(('-', (a, b))), p.lineno

    @_('_expr "*" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a * b), p.lineno
        return Expr(('*', (a, b))), p.lineno

    @_('"#" _expr')
    def _expr(self, p):
        a = p._expr[0]
        if a is int:
            return Expr(a.bit_length()), p.lineno
        return Expr(('#', (a,))), p.lineno

    @_('_expr "/" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a // b), p.lineno
        return Expr(('/', (a, b))), p.lineno

    @_('_expr "%" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a % b), p.lineno
        return Expr(('%', (a, b))), p.lineno

    @_('_expr SHL _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a << b), p.lineno
        return Expr(('<<', (a, b))), p.lineno

    @_('_expr SHR _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a >> b), p.lineno
        return Expr(('>>', (a, b))), p.lineno

    @_('_expr "^" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a ^ b), p.lineno
        return Expr(('^', (a, b))), p.lineno

    @_('_expr "|" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a | b), p.lineno
        return Expr(('|', (a, b))), p.lineno

    @_('_expr "&" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(a & b), p.lineno
        return Expr(('&', (a, b))), p.lineno

    @_('_expr "?" _expr ":" _expr')
    def _expr(self, p):
        a, b, c = p._expr0[0], p._expr1[0], p._expr2[0]
        if a is int and b is int and c is int:
            return Expr(b if a else c), p.lineno
        return Expr(('?:', (a, b, c))), p.lineno

    @_('_expr "<" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(1 if a < b else 0), p.lineno
        return Expr(('<', (a, b))), p.lineno

    @_('_expr ">" _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(1 if a > b else 0), p.lineno
        return Expr(('>', (a, b))), p.lineno

    @_('_expr LE _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(1 if a <= b else 0), p.lineno
        return Expr(('<=', (a, b))), p.lineno

    @_('_expr GE _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(1 if a >= b else 0), p.lineno
        return Expr(('>=', (a, b))), p.lineno

    @_('_expr EQ _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(1 if a == b else 0), p.lineno
        return Expr(('==', (a, b))), p.lineno

    @_('_expr NEQ _expr')
    def _expr(self, p):
        a, b = p._expr0[0], p._expr1[0]
        if a is int and b is int:
            return Expr(1 if a != b else 0), p.lineno
        return Expr(('!=', (a, b))), p.lineno

    @_('"(" _expr ")"')
    def _expr(self, p):
        return p._expr

    @_('NUMBER')
    def _expr(self, p):
        return Expr(p.NUMBER), p.lineno

    @_('STRING')
    def _expr(self, p):
        return Expr(p.STRING), p.lineno

    @_('"$"')
    def _expr(self, p):
        return next_address(), p.lineno

    @_('id')
    def _expr(self, p):
        id_str, lineno = p.id
        if id_str in self.defs:
            return self.defs[id_str], lineno
        return Expr(id_str), lineno


def exit_if_errors():
    if error_occurred:
        raise FJParsingException(f'Errors found in file {curr_file}. Assembly stopped.')


def parse_macro_tree(input_files, w, warning_as_errors, verbose=False):
    global curr_file, curr_text, error_occurred, curr_namespace
    error_occurred = False

    lexer = FJLexer()
    parser = FJParser(w, warning_as_errors, verbose=verbose)
    for curr_file in input_files:
        if not path.isfile(curr_file):
            raise FJParsingException(f"No such file {curr_file}.")
        curr_text = open(curr_file, 'r').read()
        curr_namespace = []

        lex_res = lexer.tokenize(curr_text)
        exit_if_errors()

        parser.parse(lex_res)
        exit_if_errors()

    return parser.macros
