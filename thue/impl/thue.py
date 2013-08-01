# Thue interpreter in Python <www.python.org>
# FvdP release 1.0 (2000 oct 8)
#
# tested with Python 1.5.1, Python 1.5.2.
#
# the Thue language is a creation of John Colagioia <jcolag@bigfoot.com>;
# Frédéric van der Plancke <frederic.vdplancke@writeme.com> wrote this
# interpreter.
#
# This code is public domain, but please give credit


import string
import random
import sys
import getopt


class Rule:
    #self.lhs
    #self.rhs

    def __init__(self, lhs, rhs, output=None):
        self.lhs = lhs
        self.rhs = rhs
        self.output = output  # UNUSED in current version

    def __str__(self):
        return "%s::=%s" % (self.lhs, self.rhs)

    def __repr__(self):
        if self.output:
            return "Rule(%s,%s, %s)" % (repr(self.lhs), repr(self.rhs), repr(self.output))
        else:
            return "Rule(%s,%s)" % (repr(self.lhs), repr(self.rhs))

    def __cmp__(self):
        if not isinstance(self, other):
            raise Exception("Rules are not comparable with anything else for now")
            return cmp(self.lhs, other.lhs) and cmp(self.rhs, other.rhs)


def is_space(s):
    "test if a string is composed of spaces only"

    import string
    assert type(s) in (type(""),)

    for c in s:
        if c not in string.whitespace:
            return 0
    else:
        return 1


def find_all(s, pattern):
    """return ordered list of indexes where [pattern] appears in [s];
    """
    #print "###Finding [%s] in [%s]..." % (pattern, s)

    shift_on_match = 1

    i = 0
    indexes = []
    while 1:
        i = string.find(s, pattern, i)
        if i >= 0:
            indexes.append(i)
            i = i + shift_on_match
        else:
            break

    #print "###For %s found: %s" % (pattern, indexes)
    return indexes


def print_truncated(what, width=79, dotdotdot="..."):
    if width is None or len(what) < width:
        print what
    else:
        if width <= len(dotdotdot): print dotdotdot[:width]
        else: print what[ : width - len(dotdotdot)] + dotdotdot


#-------------------------------------------------------------------------

def parse_program(program, sep = "::=", output_sep = "~", **options):
    "(program : lines) -> (rules : Rule list, dataspace : string)"

    rules = []
    dataspace = []

    state = 0 # 0=rules, 1=dataspace

    for line in program:
        '''
        @ code for comments; removed because we don't actually need
        @ any special Thue construct to allow for comments !

        #Comments - ANY # is start of a comment
        comment = string.find(line, "#")
        if comment >= 0:
            line = line[ : comment]
            line = string.rstrip(line)
        '''

        if line[-1:] == '\n':
            line = line[:-1]

        if is_space(line):
            continue

        #--debug--:
        if options.get("debug"):
            print line

        if state == 1:
            dataspace.append(line)
            continue

        isep = string.find(line, sep)
        if isep < 0:
            #if line[:1] == "#":   #comment !
            #    continue
            if is_space(line): continue
            raise 'Malformed production: "%s"' % line
        else:
            lhs = line[ : isep]
            rhs = line[isep + len(sep) : ]
            if is_space(lhs):
                if not is_space(rhs):
                    raise 'Malformed production: "%s"' % line
                state = 1
            else:
                output = None
                #in current version, output is handled at rule application
                rules.append(Rule(lhs, rhs, output))

    return (rules, string.join(dataspace, ""))


#-------------------------------------------------------------------------

def step(rulebase, dataspace, **options):
    "return (new dataspace, applied-rule-flag)"

    match_choice = options.get("match_choice", "")
    debug = options.get("debug", 0)
    print_width = options.get("print_width", None)

    #-- find all matches

    matches = []
    for rule in rulebase:
        indices = find_all(dataspace, rule.lhs)
        for i in indices:
            matches.append((i, rule))

    #-- return if none

    if len(matches) == 0:
        if debug: print "Done"
        return (dataspace, 0)

    #-- choose a match

    if match_choice == "L":
        match = min(matches)
    elif match_choice == "R":
        match = max(matches)
    else:
        match = matches[random.randint(0, len(matches)-1)]

    #-- apply the matched rule

    (pos, rule) = match
    endpos = pos + len(rule.lhs)
    assert dataspace[pos : endpos] == rule.lhs

    #iOutput = string.find(rule.rhs, "~")
    if rule.rhs[:1] == "~":
        sys.stdout.write(rule.rhs[1:])
        replacement = ""
    elif rule.rhs == ":::":
        replacement = raw_input()
    else:
        replacement = rule.rhs

    dataspace = dataspace[ : pos] + replacement + dataspace[endpos : ]
    if debug:
        #print "|",
        print_truncated(dataspace, print_width)


    return (dataspace, 1)


def execute(rulebase, dataspace, **options):

    continued = 1
    while continued:
        (dataspace, continued) = apply(step, (rulebase, dataspace), options)
        # 'apply(f, args, {k:v, ...})' is essentially 'f(args, k=v, ...)'


def execute_from_source(program, **options):
    (rules, data) = apply(parse_program, (program,), options)

    if options.get("debug", 0):
        print "Rules:"
        for r in rules:
            print repr(r)
        print "Dataspace & its transformation:"
        print_truncated(data)


    #execute(rules, data, **options)  # won't work before Python 2
    apply(execute, (rules, data), options)

#-------------------------------------------------------------------------

main_usage = """
usage: [python] thue[.py] [<options>] <program-file-name>

where <options> can be any space-separated combination of:
   -d     debug mode (prints: rules; dataspace after each transformation)
   -w N   in debug mode, dataspace print width is limited to N characters;
           if dataspace exceeds that limit, the offending part is
           replaced with '...'.
   -l     execute leftmost matches first
   -r     execute rightmost matches first
          (default: matches are executed in random order)
"""

main_options = "dlr"


class BadArgument(Exception): pass

def main(argv = None):
    if argv is None: argv = sys.argv[1:]

    try:
        (option_list, args) = getopt.getopt(argv, "dlrw:")
    except getopt.error, e:
        print e
        print main_usage
        sys.exit(1)

    options = {}
    try:
        if len(args) != 1:
            raise BadArgument("exactly one program file name is required")
        (progname,) = args

        for (opt, arg) in option_list:
            if opt == "-l": options["match_choice"] = "L"
            if opt == "-r": options["match_choice"] = "R"
            if opt == "-d": options["debug"] = 1
            if opt == "-w":
                try: width = int(arg)
                except: raise BadArgument("Illegal width '%s'" % arg)
                options["print_width"] = int(arg)

    except BadArgument, e:
        print "error:", e.args[0]
        print main_usage
        sys.exit(1)

    try: prog = open(progname)
    except IOError:
        print "Could not open file %s" % progname
        sys.exit(1)

    try: prog = prog.readlines()
    except IOError:
        print "Error reading file %s" % progname

    apply(execute_from_source, (prog,), options)
    # == 'execute_from_source(prog, **options)' in before Python 2



main()
