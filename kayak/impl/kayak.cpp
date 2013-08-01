#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>

using namespace std;

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


bool random_bit() {
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		srand(time(0));
	}
	return rand() >= RAND_MAX/2;
}


class Stack {
	vector<bool> s;
	bool entropy;
public:
	Stack(bool e=false) : entropy(e) {}
	void push(bool b);
	bool pop();
	void flip();
	bool contains_only_false() {
		return !entropy && s.empty();
	}

	static void swap(Stack& a, Stack& b) {
		std::swap(a.s, b.s); std::swap(a.entropy, b.entropy);
	}
};

void Stack::push(bool b) {
	if (b || entropy || s.size()) {
		s.push_back(b);
	}
}

bool Stack::pop() {
	if (s.size() != 0) {
		bool result = s.back();
		s.pop_back();
		return result;
	} else if (entropy) {
		return random_bit();
	} else {
		return false;
	}
}

void Stack::flip() {
	reverse(s.begin(), s.end());
}


struct Func;

struct CallInfo {
	string left_name, right_name;
	Func* func;	// 0 if not yet looked up
	vector<int> args;
};

struct Instr {
	enum Type { Not=0, Push=1, Pop=-1, LoopOpen=2, LoopClose=-2, Call=3, Llac=-3 } type;
	union {
		int pushpop_stack;
		int loop_skip;
		CallInfo* call_info;
	};
	Instr(enum Type t) {
		type = t;
	}
	Instr(enum Type t, int i) {
		type = t; pushpop_stack = i;
	}
	Instr(enum Type t, CallInfo* c) {
		type = t; call_info = c;
	}
};


bool reverse_equal(const string& a, const string& b) {
	return (a.length() == b.length() && equal(a.begin(), a.end(), b.rbegin()));
}


struct Func {
	string leftname,rightname;
	int num_locals;	// includes args
	vector<Instr> instructions;
	vector<int> right_args;	// left args are 0..right_args.size()-1
public:
	int match_name(const string& left, const string& right) const {
		if (left==leftname && right==rightname)
			return 1;
		if (reverse_equal(left,rightname) && reverse_equal(right,leftname))
			return -1;
		return 0;
	}

	static int func_equal(Func* a, Func* b) {
		return a->match_name(b->leftname, b->rightname);
	}

	bool is_main_function() {
		return leftname.empty();
	}
};


struct StackTraceback;
void error(const char* msg, StackTraceback* call_stack = 0);


void skip_comment(istream& is) {
	int depth = 0;
	for (;;) {
		char ch;
		if (!is.get(ch)) {
			error("end of file encountered while reading comment");
		}
		if (ch == '<') {
			++depth;
		} else if (ch == '>') {
			if (--depth < 0) return;
		}
	}
}


bool is_operator(char ch) {
	return (ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '|');
}


void get_token(istream& is, string& result, bool empty_okay = false) {
	result.erase();
	char ch;
	if (!(is >> ch)) {
		if (!empty_okay) {
			error("unexpected end of file");
		}
	}
	else if (is_operator(ch)) {
		result += ch;
	}
	else if (ch == '<') {
		skip_comment(is);
		get_token(is, result, empty_okay);
	}
	else if (ch == '>') {
		error("beginning of file encountered while reading comment");
	}
	else {
		do {
			result += ch;
		} while (is.get(ch) && !isspace(ch) && !is_operator(ch) && ch != '<' && ch != '>');
		is.putback(ch);
	}
}


template<class Iter, class Pred>
bool has_dupes(Iter first, Iter last, Pred p) {
	for (; first < last; ++first) {
		Iter mid = first;
		while (++mid < last) {
			if (p(*first, *mid)) {
				return true;
			}
		}
	}
	return false;
}


void parse_arglist(istream& is, vector<string>& arg_names) {
	string token;
	get_token(is, token);
	if (token[0] != ')') {
		for (;;) {
			if (is_operator(token[0])) {
				error("expected identifier");
			}
			arg_names.push_back(token);
			get_token(is, token);
			if (token[0] == ')') {
				break;
			} else if (token[0] != '|') {
				error("expected '|' or ')'");
			}
			get_token(is, token);
		}
	}
	if (has_dupes(arg_names.begin(), arg_names.end(), equal_to<string>())) {
		error("duplicate name in argument/parameter list");
	}
}


int lookup_local(vector<string>& locals, string& name) {
	vector<string>::iterator found = find(locals.begin(), locals.end(), name);
	if (found < locals.end()) {
		return found - locals.begin();
	} else {
		locals.push_back(name);
		return locals.size()-1;
	}
}


class LookupLocal {
	vector<string>& locals;
public:
	LookupLocal(vector<string>& l) : locals(l) {}
	int operator()(string& name) { return lookup_local(locals,name); }
};


Func* parse_func(istream& is) {

	// function name

	string token;
	get_token(is, token, true);
	if (token.empty()) {
		return 0;
	}

	Func* func = new Func;

	if (is_operator(token[0])) {
		if (token[0] != '(') {
			error("expected function name or '('");
		}
	} else {
		swap(func->leftname, token);
		get_token(is, token);
		if (token[0] != '(') {
			error("expected '('");
		}
	}

	// function parameter list

	vector<string> local_vars;
	parse_arglist(is, local_vars);
	int num_args = local_vars.size();
	get_token(is, token);
	if (token[0] != '{') {
		error("expected '{'");
	}

	// function body

	bool register_full = false;
	vector<int> loop_openings;
	string next_token;
	get_token(is, next_token);
	while (next_token[0] != '}') {
		swap(next_token, token);
		get_token(is, next_token);
		if (is_operator(token[0])) {
			switch (token[0]) {
			case '|':
				if (!register_full) {
					error("can't use | operator with no value in the register");
				}
				func->instructions.push_back(Instr(Instr::Not));
				break;
			case '[':
				if (!register_full) {
					error("can't use [] operator with no value in the register");
				}
				loop_openings.push_back(func->instructions.size());
				func->instructions.push_back(Instr(Instr::LoopOpen));
				register_full = false;
				break;
			case ']':
				if (loop_openings.empty()) {
					error("unexpected ']'");
				}
				if (register_full) {
					error("pushes/pops not matched inside []");
				}
				func->instructions.push_back(Instr(Instr::LoopClose, loop_openings.back()));
				func->instructions[loop_openings.back()].loop_skip = func->instructions.size()-1;
				loop_openings.pop_back();
				register_full = true;
				break;
			default:
				error("unexpected operator");
			}
		} else {
			if (next_token[0] == '(') {
				// function call
				CallInfo* call_info = new CallInfo;
				swap(call_info->left_name, token);
				call_info->func = 0;
				vector<string> args;
				parse_arglist(is, args);
				transform(args.begin(), args.end(), back_inserter(call_info->args), LookupLocal(local_vars));
				get_token(is, call_info->right_name);
				if (is_operator(call_info->right_name[0])) {
					error("expected identifier");
				}
				func->instructions.push_back(Instr(Instr::Call, call_info));
				get_token(is, next_token);
			} else {
				// push/pop
				func->instructions.push_back(Instr(register_full ? Instr::Push : Instr::Pop, lookup_local(local_vars, token)));
				register_full = !register_full;
			}
		}
	}
	if (!loop_openings.empty()) {
		error("unexpected '['");
	}
	if (register_full) {
		error("pushes/pops not matched inside {}");
	}

	// trailing parameter list and name
	get_token(is, token);
	if (token[0] != '(') {
		error("expected '('");
	}

	vector<string> right_params;
	parse_arglist(is, right_params);
	if (right_params.size() != num_args) {
		error("number of left args differs from number of right args");
	}
	transform(right_params.begin(), right_params.end(), back_inserter(func->right_args), LookupLocal(local_vars));
	if (!func->leftname.empty()) {
		get_token(is, func->rightname);
		if (is_operator(func->rightname[0])) {
			error("expected right-hand function name after trailing parameter list");
		}
	}

	func->num_locals = local_vars.size();

	return func;
}


struct StackTraceback {
	Func* func;
	bool backwards;
	StackTraceback* prev;
};

void print_call_stack(StackTraceback* call_stack) {
	cout << "call stack (innermost to outermost):" << endl;
	while (call_stack) {
		cout << '\t' << call_stack->func->leftname;
		cout << (call_stack->backwards ? "<-" : "->");
		cout << call_stack->func->rightname << endl;
		call_stack = call_stack->prev;
	}
}


void invoke(Func* func, bool backwards, const vector<Stack*>& args, StackTraceback* call_stack, const vector<Func*>& funcs) {
	StackTraceback stack_traceback = { func, backwards, call_stack };
	if (args.size() != func->right_args.size()) {
		error("function called with wrong number of arguments", &stack_traceback);
	}
	vector<Stack> locals(func->num_locals);
	for (int m = 0; m < args.size(); ++m) {
		Stack::swap(*args[m], locals[backwards ? func->right_args[m] : m]);
	}
	vector<bool> accum;
	vector<Instr>::iterator instr = backwards ? func->instructions.end()-1 : func->instructions.begin();
	while (instr >= func->instructions.begin() && instr < func->instructions.end()) {
		if (instr->type == Instr::Call && instr->call_info->func == 0) {
			for (vector<Func*>::const_iterator f = funcs.begin(); ; ++f) {
				if (f == funcs.end()) {
					error("call of undefined function", &stack_traceback);
				}
				int match = (*f)->match_name(instr->call_info->left_name, instr->call_info->right_name);
				if (match) {
					instr->call_info->func = *f;
					if (match < 0) {
						instr->type = Instr::Llac;
					}
					break;
				}
			}
		}
		int type = backwards ? -instr->type : instr->type;
		switch (type) {
		case Instr::Not:
			accum.back() = !accum.back();
			break;
		case Instr::Push:
			locals[instr->pushpop_stack].push(accum.back());
			accum.pop_back();
			break;
		case Instr::Pop:
			accum.push_back(locals[instr->pushpop_stack].pop());
			break;
		case Instr::LoopOpen:
			if (!accum.back()) {
				instr = func->instructions.begin() + instr->loop_skip;
			}
			break;
		case Instr::Call:
		case Instr::Llac:
		    {
			vector<Stack*> subcall_args(instr->call_info->args.size());
			for (int i=0; i<instr->call_info->args.size(); ++i) {
				subcall_args[i] = &locals[instr->call_info->args[instr->type==Instr::Llac ? instr->call_info->args.size()-1-i : i]];
			}
			invoke(instr->call_info->func, type==Instr::Llac, subcall_args, &stack_traceback, funcs);
			break;
		    }
		}
		backwards ? --instr : ++instr;
	}
	for (int o = 0; o < args.size(); ++o) {
		Stack::swap(*args[o], locals[backwards ? o : func->right_args[o]]);
	}
	for (int p = 0; p < locals.size(); ++p) {
		if (!locals[p].contains_only_false()) {
			cout << "***RUNTIME ERROR*** a stack was nonempty on function exit" << endl;
		}
	}
}


Stack* read_input(istream& is) {
	Stack* s = new Stack(false);
	char ch;
	while (is.get(ch)) {
		int n = (int(ch) & 255) * 2 + 1;
		for (int i=0; i<9; ++i) {
			s->push(n&1);
			n >>= 1;
		}
	}
	s->flip();
	return s;
}


void write_output(Stack* s, ostream& os) {
	while (s->pop()) {
		int n=0;
		for (int place=1; place<256; place*=2) {
			n += place * s->pop();
		}
		os << char(n);
	}
	if (!s->contains_only_false()) {
		cout << "***RUNTIME ERROR*** the output stack contained extra bits" << endl;
	}
}


void error(const char* msg, StackTraceback* call_stack) {
	cout << "Error: " << msg << endl;
	if (call_stack) {
		print_call_stack(call_stack);
	}
	exit(1);
}


int main(int argc, char** argv)
{
	if (argc != 2) {
		cout <<	"usage: kayak <filename>\n"
			"\n"
			"  If <filename> exists, runs it forwards. Otherwise,\n"
			"  if <emanelif> exists, runs it backwards.\n";
		return 1;
	}
	bool run_in_reverse=false;
	ifstream is(argv[1]);
	if (!is) {
		is.clear();
		run_in_reverse=true;
		reverse(argv[1], argv[1]+strlen(argv[1]));
		is.open(argv[1]);
		if (!is) {
			error("unable to open either the specified file or its reversal");
		}
	}
	vector<Func*> funcs;
	{
		Func* func;
		while ((func = parse_func(is)) != 0) {
			funcs.push_back(func);
		}
	}
	is.close();
	if (has_dupes(funcs.begin(), funcs.end(), Func::func_equal)) {
		error("duplicate function in source file");
	}
	vector<Func*>::iterator func = find_if(funcs.begin(), funcs.end(), mem_fun(&Func::is_main_function));
	if (func == funcs.end()) {
		error("no main function");
	}
	int num_main_args = (*func)->right_args.size();
	if (num_main_args < 1 || num_main_args > 2) {
		error("main function must take 1 or 2 arguments");
	}
	Stack input(false);
	vector<Stack*> main_args(num_main_args);
	if (num_main_args == 1) {
		main_args[0] = read_input(cin);
	} else {
		main_args[run_in_reverse] = new Stack(true);
		main_args[!run_in_reverse] = read_input(cin);
	}
	invoke(*func, run_in_reverse, main_args, 0, funcs);
	write_output(main_args[(num_main_args == 1) ? 0 : run_in_reverse], cout);
	return 0;
}
