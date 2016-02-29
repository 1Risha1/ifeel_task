#pragma once
#include <vector>
#include <string>

extern std::string optarg;
extern int optind;

struct option
{
	std::string name;
	int has_arg;
	int *flag;
	int val;
};

#define	no_argument		0
#define required_argument	1
#define optional_argument	2

extern int getopt(int argc, char *const *argv,
	std::vector<struct option> &longopts, int &longind);

class getopt_error : public std::invalid_argument {
public:
	explicit getopt_error(const char* _msg);
	explicit getopt_error(const std::string& _msg);
private:
};