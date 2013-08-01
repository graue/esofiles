// These global variables keep the state of the code:
// Execution doesn't influence them, only the user

rules = {}
dataspace = ""

// These global variables keep the state of the execution:

workspace = ""
state = 'nothing'
magic = ''
matchindex = 0
matchlen = 0
selected_rule = ''
output_text = ''

// Code follows.

function init_execution()
{
    workspace = dataspace
    state = 'nothing'
    magic = ''
    matchindex = 0
    matchlen = 0
    selected_rule = ''
}

function step()
{
    // Steps once through the execution. Modifies
    // the globals which keep the state of the execution.
    // Doesn't display anything.
    output_text = ''
    magic = ''
    if (state == 'done') return;
    if (state == 'nothing' || state == 'changed') {
        matching_rules = []
        for (rule in rules) {
            matches = all_matches(rule, workspace)
            if (matches.length > 0) {
                matching_rules.push([rule, matches])
            }
        }
        if (matching_rules.length == 0) {
            state = 'done'
            return
        }
        var selected = random_choice(matching_rules)
        selected_rule = selected[0]
        matchindex = random_choice(selected[1])
        matchlen = selected_rule.length
        state = 'selected'
        return
    } else if (state == 'selected') {
        state = 'changed'
        var selected_rhs = random_choice(rules[selected_rule])
        if (selected_rhs == '') {
            magic = 'empty'
            workspace = workspace.substring(0,matchindex)+
                        workspace.substring(matchindex+matchlen,workspace.length)
            matchlen = 0
            return
        }
        if (selected_rhs.charAt(0) == '~') {
            // handle output!
            magic = 'output'
            output_text = selected_rhs.substring(1,selected_rhs.length)
            workspace = workspace.substring(0,matchindex)+
                        workspace.substring(matchindex+matchlen,workspace.length)
            matchlen = 0
            return
        }
        if (selected_rhs == ':::') {
            // handle input!
            selected_rhs = prompt("Gimme a string!")
        }
        workspace = workspace.substring(0,matchindex) + selected_rhs +
                    workspace.substring(matchindex+matchlen,workspace.length)
        matchlen = selected_rhs.length
        return
    }
}

function random_choice(a)
{
    // Returns a random element from an array
    return a[Math.floor(Math.random() * a.length)]
}


function all_matches(s, text)
{
    // Returns starting indexes of all matches of 's' in the text
    var result = []
    var lastindex = 0
    do {
        i = text.indexOf(s, lastindex);
        if (i != -1) {
            result.push(i)
        }
        lastindex = i + 1
    } while (i != -1)
    return result
}

function split_rule(text)
{
    // Splits a rule into two parts at the '::=' separator
    i = text.indexOf('::=')
    if (i == -1) {
        // malformed rule
        return undefined
    }
    return [text.substr(0, i), text.substr(i+3, text.length)]

}

function update_state(text)
{
    // parses the text, and updates rules, dataspace

    var lines = text.split(/(\r\n|\r|\n)/);
    var datalines = [];
    var doing_rules = true;
    rules = {}
    dataspace = ""
    for (l in lines) {
        line = lines[l]
        if (line.match(/^\s*$/)) {
            // empty line
            continue;
        }
        if (doing_rules && line == '::=') {
            doing_rules = false;
            continue;
        }
        if (doing_rules) {
            var new_rule = split_rule(line);
            if (rules[new_rule[0]]) {
                rules[new_rule[0]].push(new_rule[1]);
            } else {
                rules[new_rule[0]] = [new_rule[1]];
            }
        } else {
            datalines.push(line);
        }
    }
    dataspace = datalines.join('\n')
}

