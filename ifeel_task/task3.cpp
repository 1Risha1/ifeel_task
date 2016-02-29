#include "task3.h"
#include <exception>
#include <iostream>

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::exception;

getopt_error::getopt_error(const char* _msg) : invalid_argument (_msg) {
}

getopt_error:: getopt_error(const std::string& _msg) : invalid_argument(_msg) {
}

static char* const* prev_argv = NULL;        /* Keep a copy of argv and argc to */
static int prev_argc = 0;                    /*    tell if getopt params change */
static int optind = 0;                   /* Option we're checking */
extern std::string optarg = "";

/*Find index of option stored in LONGOPTS*/
static int _find_longind(const string &opt, const vector<struct option> &longopts) {
	vector<int> matches;
	for (size_t i = 0; i < longopts.size(); i++) {
		if (opt.compare(longopts[i].name.substr(0, opt.size())) == 0) {
			matches.push_back(i);
		}
	}
	if (matches.size() > 1) {
		throw getopt_error("option --" + opt + " not a unique prefix");
	}
	else if (matches.empty()) {
		throw getopt_error("option--" + opt +" not recognized");
	}
	return matches[0];
}

static void print_help(vector<struct option> &longopts) {
	cout << "List of possible options:\n";
	for (int i = 0; i < longopts.size(); i++) {
		std::cout << "--" <<longopts[i].name;
		if (longopts[i].has_arg == required_argument) {
			std::cout << "[= ]my-option-val";
		}
		else if (longopts[i].has_arg == optional_argument) {
			std::cout << "[=my-option-val]";
		}
		std::cout << std::endl;
	}
}

/*
Scan elements of ARGV (whose length is ARGC) for option characters
given in OPTSTRING.

Long-named options begin with `--'.
Their names may be abbreviated as long as the abbreviation is unique
or is an exact match for some defined option.  If they have an
argument, it follows the option name in the same ARGV-element, separated
from the option name by a `=', or else the in next ARGV-element.
When `getopt' finds a long-named option, it returns 0 if that option's
`flag' field is nonzero, the value of the option's `val' field
if the `flag' field is zero.
*/
int getopt(int argc, char *const *argv,
	vector<struct option> &longopts, int &longind) {
	if (argc == optind) {
		return -1;
	}

	/* If we have new argv, reinitialize */
	if (prev_argv != argv || prev_argc != argc)
	{
		/* Initialize variables */
		prev_argv = argv;
		prev_argc = argc;
		/*First always is program name*/
		optind = 1;
	}

	string opt_name = string(argv[optind]);
	/*Skip all non-option args*/
	while (opt_name.substr(0, 2).compare("--") != 0) {
		if (argc == ++optind) {
			return -1;
		}
		opt_name = string(argv[optind]);	
	}

	/*delete leading --*/
	opt_name = opt_name.substr(2, opt_name.size());
	

	size_t ind;
	string arg = "";
	if ((ind = opt_name.find('=')) != string::npos){
		arg = opt_name.substr(ind + 1);
		opt_name = opt_name.substr(0, ind);
	}

	longind = _find_longind(opt_name, longopts);
	const struct option &opt = longopts[longind];

	if (!opt.name.compare("help")) {
		print_help(longopts);
	}


	if (opt.has_arg == no_argument) {
		if (arg.compare("")) {
			throw getopt_error("option " + opt.name + " must not have an argument");
		}	
	} else if (opt.has_arg == required_argument) {
		if (!arg.compare("")) {
			if (optind >= argc) {
				throw getopt_error("option " + opt.name + " requires argument");
			}
			else {
				arg = argv[++optind];
			}
		}
	}
	optarg = arg;		
	optind++;

	if (opt.flag != NULL) {
		*(opt.flag) = opt.val;
		return 0;
	}
	else {
		return opt.val;
	}
}


int main(int argc, char* argv[]) {
	vector<struct option> a;
	int flag_a = 0, longind;
	a.push_back({ "opta", no_argument, &flag_a, 1 });
	a.push_back({ "help", no_argument, 0, 0 });
	a.push_back({ "rob", optional_argument, 0, 0 });
	a.push_back({ "bn", required_argument, 0, 0 });

	while (getopt(argc, argv, a, longind) != -1) {
		cout << a[longind].name << " " << optarg << endl;
	}
	return 0;
}