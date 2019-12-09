/* Copyright 1995, 1996 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include <base/common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sstream>

#include <commandline/option.h>
#include <commandline/parsenum.h>
#include <rsys/paths.h>

using namespace Executor;
using namespace std;

struct opt_block
{
    string interface;
    option_vec opts;
};
static vector<opt_block> opt_blocks;

static std::vector<std::string> pre_notes;

static void
wrap(const char *buf,
     int desired_len,
     int *out_len, int *next_len)
{
    int buf_len;

    buf_len = strlen(buf);

    if(buf_len <= desired_len)
    {
        *out_len = buf_len;
        *next_len = -1;
    }
    else
    {
        const char *t;

        t = &buf[desired_len];
        while(!isspace(*t))
            t--;
        *next_len = (t - buf) + 1;

        while(isspace(*t))
            t--;

        *out_len = (t - buf) + 1;
    }
}

static string generate_help_message(void)
{
    std::ostringstream help;
    int i;
    const char *buf;

    for(i = 0; i < pre_notes.size(); i++)
    {
        int out_len, next_len;

        std::string wrap_buf = pre_notes[i];

        for(buf = wrap_buf.c_str();; buf += next_len)
        {
            wrap(buf, 75, &out_len, &next_len);
            help << std::string(buf, buf+out_len) << '\n';
            if(next_len == -1)
                break;
        }
    }
    /* newline to separate pre-notes and options */
    help << '\n';
    for(const auto& [interface, opts] : opt_blocks)
    {
        help << interface << ":\n";

        for(const auto& opt : opts)
        {
            const char *spaces = "                    ";
            int next_len, out_len;

            int opt_text_len;
            bool same_line_p;

            if(opt.desc == "")
                continue;

            opt_text_len = opt.text.length();
            same_line_p = opt_text_len < 14;

            help << "  -" << opt.text;
            
            if(same_line_p)
                help << spaces + 3 + opt_text_len;
            else
                help << '\n' << spaces;

            std::string wrap_buf = opt.desc;
            for(buf = wrap_buf.c_str();; buf += next_len)
            {
                wrap(buf, 55, &out_len, &next_len);
                help << std::string(buf, buf+out_len) << '\n';
                if(next_len != -1)
                    help << spaces;
                else
                    break;
            }
        }
    }
    return help.str();
}

const char *
Executor::opt_help_message(void)
{
    static std::string help_message = generate_help_message();
    return help_message.c_str();
}

void Executor::opt_register_pre_note(string note)
{
    pre_notes.push_back(std::move(note));
}

#define ERRMSG_STREAM stderr

void Executor::opt_register(string new_interface,
                            option_vec new_opts)
{
    /* check for conflicting options */
    for(const auto& [interface, opts] : opt_blocks)
    {
        for(const auto& opt : opts)
            for(const auto& new_opt : new_opts)
            {
                if(opt.text == new_opt.text)
                {
                    /* conflicting options */
                    fprintf(ERRMSG_STREAM,
                        "%s: opt internal error: `%s' and `%s' both request option `%s'\n",
                        ROMlib_appname.c_str(),
                        interface.c_str(), new_interface.c_str(), opt.text.c_str());
                    exit(-16);
                }
            }
    }

    opt_blocks.push_back({new_interface, new_opts});
}

bool Executor::opt_val(const opt_database_t &db, string opt, string *retval)
{
    auto p = db.find(opt);
    if(p != db.end())
    {
        if(retval)
            *retval = p->second;
        return true;
    }
    else
        return false;
}

/* Parses an integer value and returns it in *retval.  If
 * parse_error_p is non-nullptr, sets parse_error_p to true and prints an
 * error message in case of a parse error, else leaves it untouched.
 * Returns true if a value was found.
 */
bool Executor::opt_int_val(const opt_database_t &db, string opt, int *retval,
                          bool *parse_error_p)
{
    string val;

    if(opt_val(db, opt, &val))
    {
        if(retval)
        {
            int32_t v;
            if(!parse_number(val, &v, 1))
            {
                if(parse_error_p)
                {
                    fprintf(stderr, "Malformed numeric argument to -%s.\n", opt.c_str());
                    *parse_error_p = true;
                }
                return false;
            }
            *retval = v;
        }
        return !val.empty();
    }
    else
        return false;
}

bool Executor::opt_bool_val(const opt_database_t &db, string opt, bool *retval,
                          bool *parse_error_p)
{
    int tmp;
    if(opt_int_val(db, opt, &tmp, parse_error_p))
    {
        if(retval)
            *retval = tmp != 0;
        return true;
    }
    else
        return false;
}

bool Executor::opt_parse(opt_database_t &db,
                        int *argc, char *argv[])
{
    bool parse_error_p = false;
    int i;
    int argc_left;

    /* we are sure to copy the null termination because we start with
     `i = 1' */
    argc_left = *argc;

    /* skip over the program name */
    for(i = 1; i < *argc; i++, argc_left--)
    {
        char *arg;
        int arg_i;

        arg_i = i;
        arg = argv[i];
        /* option */
        if(*arg == '-')
        {
            /* find the option among the options */
            for(const auto& [_,block] : opt_blocks)
            {
                for(const auto& opt : block)
                {
                    if(!strcmp(&arg[1],
                            opt.text.c_str()))
                    {
                        string optval = "";

                        /* found the option */
                        switch(opt.kind)
                        {
                            case opt_no_arg:
                                optval = "1";
                                break;
                            case opt_optional:
                                if((i + 1) < *argc && *argv[i + 1] != '-')
                                {
                                    optval = argv[++i];
                                    argc_left--;
                                }
                                else
                                    optval = opt.opt_val;
                                break;
                            case opt_sticky:
                                optval = &arg[1 + opt.text.length()];
                                break;
                            case opt_sep:
                                if((i + 1) < *argc)
                                {
                                    optval = argv[++i];
                                    argc_left--;
                                }
                                else
                                {
                                    fprintf(ERRMSG_STREAM, "\
    %s: option `-%s' requires argument\n",
                                            ROMlib_appname.c_str(), opt.text.c_str());
                                    parse_error_p = true;
                                }
                                break;
                            case opt_ignore:
                                goto next_arg;
                            case opt_sep_ignore:
                                if((i + 1) < *argc)
                                {
                                    i++;
                                    argc_left--;
                                }
                                else
                                {
                                    fprintf(ERRMSG_STREAM, "\
    %s: option `-%s' requires argument\n",
                                            ROMlib_appname.c_str(), opt.text.c_str());
                                    parse_error_p = true;
                                }
                                goto next_arg;
                        }
                        *argc -= (i + 1) - arg_i;
                        memmove(&argv[arg_i], &argv[i + 1],
                                (argc_left - 1) * sizeof *argv);
                        i = arg_i - 1;
                        db[opt.text] = optval;
                    }
                }
            }
        }
    next_arg:;
    }
    return parse_error_p;
}
