#if !defined(_RSYS_OPTION_H_)
#define _RSYS_OPTION_H_

#include <vector>

namespace Executor
{
typedef enum option_kind {
    /* this option has no argument; it evaluates to `def' if provided */
    opt_no_arg,
    /* option is provided if next element in argv does not start with `-' */
    opt_optional,
    /* argument is part of argument, ie., -p4 */
    opt_sticky,
    /* argument is next value in argv */
    opt_sep,
    /* ignore this option */
    opt_ignore,
    /* ignore this option and its argument */
    opt_sep_ignore,
} option_kind_t;

typedef struct option
{
    /* text of the option.  does not include the `-' prefix */
    std::string text;

    /* description of the option */
    std::string desc;

    /* kind of argument */
    option_kind_t kind;

    /* value of option if it is specified with no argument */
    std::string opt_val;
} option_t;

typedef std::vector<option_t> option_vec;

typedef std::unordered_map<std::string, std::string> opt_database_t;

void opt_register(std::string new_interface, option_vec opts);

bool opt_parse(opt_database_t &db, int *argc, char *argv[]);

/* returns true if options was specified */
bool opt_int_val(const opt_database_t &db, std::string opt, int *retval = nullptr,
                bool *parse_error_p = nullptr);
bool opt_bool_val(const opt_database_t &db, std::string opt, bool *retval = nullptr,
                bool *parse_error_p = nullptr);

bool opt_val(const opt_database_t &db, std::string opt, std::string *retval = nullptr);

const char *opt_help_message(void);
void opt_register_pre_note(std::string note);
}
#endif /* !_RSYS_OPTION_H_ */
