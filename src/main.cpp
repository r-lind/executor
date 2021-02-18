/* Copyright 1988 - 2005 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/*
 * Errors should be handled more cleanly.
 */

#include <base/common.h>


#include <FontMgr.h>
#include <DialogMgr.h>
#include <AppleEvents.h>


#include <quickdraw/cquick.h>
#include <textedit/tesave.h>
#include <mman/mman.h>
#include <menu/menu.h>
#include <prefs/prefs.h>
#include <commandline/flags.h>
#include <commandline/option.h>
#include <time/syncint.h>
#include <vdriver/vdriver.h>
#include <time/vbl.h>
#include <time/time.h>
#include <rsys/segment.h>
#include <rsys/version.h>
#include <base/m68kint.h>
#include <quickdraw/rgbutil.h>
#include <vdriver/refresh.h>
#include <rsys/executor.h>
#include <wind/wind.h>
#include <osevent/osevent.h>
#include <quickdraw/image.h>
#include <rsys/dump.h>
#include <file/file.h>
#include <ctl/ctl.h>
#include <commandline/parseopt.h>
#include <print/print.h>
#include <mman/memsize.h>
#include <vdriver/autorefresh.h>
#include <sound/sounddriver.h>
#include <error/system_error.h>
#include <commandline/option.h>
#include <base/emustubs.h>
#include <sane/float.h>
#include <rsys/paths.h>
#include <appleevent/apple_events.h>
#include <rsys/gestalt.h>
#include <rsys/launch.h>
#include <quickdraw/text.h>
#include <rsys/appearance.h>
#include <hfs/hfs_plus.h>
#include <base/cpu.h>
#include <base/debugger.h>
#include <rsys/mon_debugger.h>
#include <PowerCore.h>
#include <vdriver/eventrecorder.h>

#include "default_vdriver.h"
#include "headless.h"

#if defined(__linux__) && defined(PERSONALITY_HACK)
#include <sys/personality.h>
#define READ_IMPLIES_EXEC 0x0400000
#endif


#include <ctype.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

#include <vector>
#include <thread>

using namespace Executor;
using namespace std;


/* Set to true if there was any error parsing arguments. */
static bool bad_arg_p = false;


static bool use_native_code_p = true;
static bool breakOnProcessStart = false;
static bool logtraps = false;
static std::string keyboard;
static bool list_keyboards_p = false;

static const option_vec common_opts = {
    { "headless", "no graphics output", opt_no_arg, "" },
    { "record", "record events to file", opt_sep, "" },
    { "playback", "play back events from file", opt_sep, "" },
    { "timewarp", "speed up or slow down time", opt_sep, "1/1" },
    { "sticky", "sticky menus", opt_no_arg, "" },
    { "nobrowser", "don't run Browser", opt_no_arg, "" },
    { "bpp", "default screen depth", opt_sep, "" },
    { "size", "default screen size", opt_sep, "" },
    { "debug",
      ("enable certain debugging output and consistency checks.  This "
       "is primarily used by ARDI developers, but we are making it "
       "available during the pre-beta period to expert users.  The next "
       "argument must be a list of comma-separated words describing which "
       "debug options you want enabled.  You can abbreviate the debug "
       "options as long as the abbreviation is unambiguous.  Here is a "
       "list of the options (some of which will may do nothing):  "
       "\"all\" enables all debugging options, "
       "\"fslog\" enables filesystem call logging, "
       "\"memcheck\" enables heap consistency checking (slow!), "
       "\"textcheck\" enables text record consistency checking (slow!), "
       "\"trace\" enables miscellaneous trace information, "
       "\"sound\" enables miscellaneous sound logging information, "
       "\"trapfailure\" enables warnings when traps return error codes, "
       "\"errno\" enables some C library-related warnings, "
       "\"unexpected\" enables warnings for unexpected events, "
       "\"unimplemented\" enables warnings for unimplemented traps.  "
       "Example: \"executor -debug unimp,trace\""),

      opt_sep, "" },
    { "nosound", "disable any sound hardware",
      opt_no_arg, "" },

#if defined(__linux__)
    { "nodrivesearch",
      "Do not look for a floppy drive, CD-ROM drive or any other drive "
      "except as specified by the MacVolumes environment variable",
      opt_no_arg, "" },
#endif /* __linux__ */
    { "keyboards", "list available keyboard mappings",
      opt_no_arg, "" },
    { "keyboard", "choose a specific keyboard map", opt_sep, "" },
    { "print",
      "tell program to print file; not useful unless you also "
      "specify a program to run and one or more documents to print.",
      opt_no_arg, "" },

  { "noclock",     "disable timer",               opt_no_arg,   "" },

    { "noautorefresh",
      "turns off automatic detection of programs that bypass QuickDraw.",
      opt_no_arg, "" },
    {
        "refresh",
        "handle programs that bypass QuickDraw, at a performance penalty.  "
        "Follow -refresh with an number indicating how many 60ths of a second "
        "to wait between each screen refresh, e.g. \"executor -refresh 10\".",
        opt_optional, "10",
    },
    {
        "help", "print this help message", opt_no_arg, "",
    },
    {
        "version", "print the Executor version", opt_no_arg, "",
    },
    {
        "memory",
        "specify the total memory you want reserved for use by the programs "
        "run under Executor and for Executor's internal system software.  "
        "For example, \"executor -memory 5.5M\" would "
        "make five and a half megabytes available to the virtual machine.  "
        "Executor will require extra memory above and beyond this amount "
        "for other uses.",
        opt_sep, "",
    },
    {
        "applzone",
        "specify the memory to allocate for the application being run, "
        "e.g. \"executor -applzone 4M\" would make four megabytes "
        "of RAM available to the application.  \"applzone\" stands for "
        "\"application zone\".",
        opt_sep, "",
    },
    {
        "stack",
        "like -applzone, but specifies the amount of stack memory to allocate.",
        opt_sep, "",
    },
    {
        "syszone", "like -applzone, but specifies the amount of memory to make "
                   "available to Executor's internal system software.",
        opt_sep, "",
    },
    { "system",
      "specify the system version that executor reports to applications",
      opt_sep, "" },
    { "notnative",
      "don't use native code in syn68k",
      opt_no_arg, "" },

    { "grayscale", "\
specify that executor should run in grayscale mode even if it is \
capable of color.",
      opt_no_arg, "" },

    { "cities", "Don't use Helvetica for Geneva, Courier for Monaco and Times "
                "for New York when generating PostScript",
      opt_no_arg, "" },

    { "prvers",
      "specify the printer version that executor reports to applications",
      opt_sep, "" },

    { "prres",
      "specify an additional resolution available for printing, e.g. "
      "\"executor -prres 600x600\" will make 600dpi resolution available "
      "in addition to the standard 72dpi.  Not all apps will be able to use "
      "this additional resolution.",
      opt_sep, "" },

    { "ppc", "prefer PPC code to 68K code when both are available", opt_no_arg, "" },

    { "appearance", "(mac or windows) specify the appearance of windows and "
                    "menus.  For example \"executor -appearance windows\" will make each "
                    "window have a blue title bar",
      opt_sep, "" },
    {"scancodes", "different form of key mapping (may be useful in "
        "conjunction with -keyboard; not supported for all vdrivers)", opt_no_arg},

    { "logtraps", "print every operating system and toolbox calls and their arguments", opt_no_arg, "" },
    { "speech", "enable speech manager (mac hosts only)", opt_no_arg, ""},
    { "break", "break into debugger at program start", opt_no_arg, ""}
};

/* Prints the specified value in a representation appropriate for command
 * line switches.
 */
static void
print_command_line_value(FILE *fp, int v)
{
    if(v > 0 && v % 1024 == 0)
    {
        if(v % (1024 * 1024) == 0)
            fprintf(fp, "%dM", v / (1024 * 1024));
        else
            fprintf(fp, "%dK", v / 1024);
    }
    else
        fprintf(fp, "%d", v);
}

static void
check_arg(string argname, int *arg, int min, int max)
{
    if(*arg < min || *arg > max)
    {
        fprintf(stderr, "%s: invalid value for `%s': must be between ",
                ROMlib_appname.c_str(), argname.c_str());
        print_command_line_value(stderr, min);
        fputs(" and ", stderr);
        print_command_line_value(stderr, max);
        fputs(" inclusive.\n", stderr);
        bad_arg_p = true;
    }
}


/* This is to tell people about the switch from "-applzone 4096" to
 * "-applzone 4M".
 */
static void
note_memory_syntax(const char *arg, unsigned val)
{
    /* Make sane error messages when the supplied value is insane.
   * Otherwise, try to print an example that illustrates how to
   * use the value they are trying to generate.
   */
    if(val < 100 || val > 1000000)
        val = 2048;

    fprintf(stderr, "Specified %s is too small. "
                    "For a %u.%02u\nmegabyte %s you would "
                    "say \"-%s ",
            arg, val / 1024, (val % 1024) * 100 / 1024,
            arg, arg);

    if(val % 1024 == 0)
        fprintf(stderr, "%uM", val / 1024);
    else if(val < 1024)
        fprintf(stderr, "%uK", val);
    else
        fprintf(stderr, "%u.%02uM", val / 1024, (val % 1024) * 100 / 1024);

    fputs("\"\n", stderr);

    bad_arg_p = true;
}

/* Concatenates all strings in the array, separated by spaces. */
static const char *
construct_command_line_string(int argc, char **argv)
{
    char *s;
    unsigned long string_length;
    int i, j;

    for(i = 0, string_length = 0; i < argc; i++)
        string_length += strlen(argv[i]) + 1;
    s = (char *)malloc(string_length + 1);
    s[0] = '\0';
    for(j = 0; j < argc; j++)
        sprintf(s + strlen(s), "%s%s", j ? " " : "", argv[j]);
    return s;
}

static void reportBadArgs()
{
    fprintf(stderr,
            "Type \"%s -help\" for a list of command-line options.\n",
            ROMlib_appname.c_str());
    exit(-10);
}

static void checkBadArgs(int argc, char **argv)
{
    if(argc >= 2)
    {
        int a;

        /* Only complain if we see something with a leading dash; anything
         * else might be a file to launch.
         */
        for(a = 1; a < argc; a++)
        {
            if(argv[a][0] == '-')
            {
                fprintf(stderr, "%s: unknown option `%s'\n",
                        ROMlib_appname.c_str(), argv[a]);
                bad_arg_p = true;
            }
        }
    }

    if(bad_arg_p)
        reportBadArgs();
}

bool flag_headless = false;
std::optional<fs::path> flag_record, flag_playback;

static void parseCommandLine(int& argc, char **argv)
{
    opt_database_t opt_db;
    string arg;

    opt_register("common", common_opts);

    opt_register_pre_note("welcome to the executor help message.");
    opt_register_pre_note("usage: `executor [option...] "
                          "[program [document1 document2 ...]]'");

    if(!bad_arg_p)
        bad_arg_p = opt_parse(opt_db, &argc, argv);

    if(opt_val(opt_db, "version", nullptr))
    {
        fprintf(stdout, "%s\n", EXECUTOR_VERSION);
        exit(0);
    }

    /*
   * If they ask for help, it's not an error -- it should go to stdout
   */

    if(opt_val(opt_db, "help", nullptr))
    {
        fprintf(stdout, "%s", opt_help_message());
        exit(0);
    }

    /* Verify that the user input a legal bits per pixel.  "0" is a legal
   * value here; that means "use the vdriver's default."
   */
    opt_int_val(opt_db, "bpp", &flag_bpp, &bad_arg_p);
    if(flag_bpp != 0 && flag_bpp != 1
       && flag_bpp != 2 && flag_bpp != 4 && flag_bpp != 8
       && flag_bpp != 16 && flag_bpp != 32)
    {
        fprintf(stderr, "Bits per pixel must be 1, 2, 4, 8, 16 or 32.\n");
        bad_arg_p = true;
    }

    if(opt_val(opt_db, "ppc", nullptr))
        ROMlib_set_ppc(true);

    if(opt_val(opt_db, "size", &arg))
        bad_arg_p |= !parse_size_opt("size", arg);

    if(opt_val(opt_db, "prres", &arg))
        bad_arg_p |= !parse_prres_opt(&ROMlib_optional_res_x,
                                      &ROMlib_optional_res_y, arg);

    if(opt_val(opt_db, "debug", &arg))
        bad_arg_p |= !error_parse_option_string(arg);

#if defined(__APPLE__)
    // sync() really takes a long time on Mac OS X.
    ROMlib_nosync = true;
#endif

    {
        int skip;
        skip = 0;
        opt_int_val(opt_db, "nosound", &skip, &bad_arg_p);
        sound_disabled_p = (skip != 0);
    }

    use_native_code_p = !opt_val(opt_db, "notnative", nullptr);

    ROMlib_fontsubstitution = !opt_val(opt_db, "cities", nullptr);

    /* Parse the "-memory" option. */
    {
        int total_memory;
        if(opt_int_val(opt_db, "memory", &total_memory, &bad_arg_p))
        {
            check_arg("memory", &total_memory,
                      (MIN_APPLZONE_SIZE + DEFAULT_SYSZONE_SIZE
                       + DEFAULT_STACK_SIZE),
                      (MAX_APPLZONE_SIZE + DEFAULT_SYSZONE_SIZE
                       + DEFAULT_STACK_SIZE));

            /* Set up the three memory sizes appropriately.  For now we
       * just allocate the defaults for syszone and stack, and
       * put everything else in -applzone.
       */
            ROMlib_syszone_size = DEFAULT_SYSZONE_SIZE;
            ROMlib_stack_size = DEFAULT_STACK_SIZE;
            ROMlib_applzone_size = (total_memory - ROMlib_syszone_size
                                    - ROMlib_stack_size);
        }
    }

    /* I bumped the minimal ROMlib_applzone to 512, since Loser needs
   more than 256.  I guess it's a little unfair to people who bypass
   Loser, but it will prevent confusion.  */
    opt_int_val(opt_db, "applzone", &ROMlib_applzone_size, &bad_arg_p);
    if(ROMlib_applzone_size < 65536)
        note_memory_syntax("applzone", ROMlib_applzone_size);
    else
        check_arg("applzone", &ROMlib_applzone_size, MIN_APPLZONE_SIZE,
                  MAX_APPLZONE_SIZE);

    opt_int_val(opt_db, "syszone", &ROMlib_syszone_size, &bad_arg_p);
    if(ROMlib_syszone_size < 65536)
        note_memory_syntax("syszone", ROMlib_syszone_size);
    else
        check_arg("syszone", &ROMlib_syszone_size, MIN_SYSZONE_SIZE,
                  MAX_SYSZONE_SIZE);

    opt_int_val(opt_db, "stack", &ROMlib_stack_size, &bad_arg_p);
    if(ROMlib_stack_size < 32768)
        note_memory_syntax("stack", ROMlib_stack_size);
    else
        check_arg("stack", &ROMlib_stack_size, MIN_STACK_SIZE, MAX_STACK_SIZE);

    opt_bool_val(opt_db, "sticky", &ROMlib_sticky_menus_p, &bad_arg_p);
    opt_bool_val(opt_db, "nobrowser", &ROMlib_nobrowser, &bad_arg_p);
    opt_bool_val(opt_db, "print", &ROMlib_print, &bad_arg_p);
    opt_bool_val(opt_db, "speech", &ROMlib_speech_enabled, &bad_arg_p);
    opt_bool_val (opt_db, "noclock", &ROMlib_noclock,   &bad_arg_p);
    {
        int no_auto = false;
        opt_int_val(opt_db, "noautorefresh", &no_auto, &bad_arg_p);
        do_autorefresh_p = !no_auto;
    }

    opt_int_val(opt_db, "refresh", &ROMlib_refresh, &bad_arg_p);
    check_arg("refresh", &ROMlib_refresh, 0, 60);

    opt_bool_val(opt_db, "grayscale", &flag_grayscale, &bad_arg_p);

#if defined(__linux__)
    opt_bool_val(opt_db, "nodrivesearch", &nodrivesearch_p, &bad_arg_p);
#endif

    {
        string str;

        if(opt_val(opt_db, "prvers", &str))
        {
            uint32_t vers;

            if(!ROMlib_parse_version(str, &vers))
                bad_arg_p = true;
            else
                ROMlib_PrDrvrVers = (vers >> 8) * 10 + ((vers >> 4) & 0xF);
        }
    }

    {
        string appearance_str;

        if(opt_val(opt_db, "appearance", &appearance_str))
            bad_arg_p |= !ROMlib_parse_appearance(appearance_str.c_str());
    }


    /* parse the `-system' option */
    {
        string system_str;

        if(opt_val(opt_db, "system", &system_str))
            bad_arg_p |= !parse_system_version(system_str);
    }

    opt_bool_val(opt_db, "break", &breakOnProcessStart, &bad_arg_p);
    opt_bool_val(opt_db, "logtraps", &logtraps, &bad_arg_p);

    opt_bool_val(opt_db, "scancodes", &ROMlib_use_scan_codes, &bad_arg_p);
    opt_val(opt_db, "keyboard", &keyboard);
    opt_bool_val(opt_db, "keyboards", &list_keyboards_p, &bad_arg_p);
    if(list_keyboards_p)
        flag_headless = true;

    if(opt_val(opt_db, "headless", nullptr))
        flag_headless = true;

    if(std::string str; opt_val(opt_db, "record", &str))
        flag_record = fs::path(str);

    if(std::string str; opt_val(opt_db, "playback", &str))
        flag_playback = fs::path(str);

    if(std::string str; opt_val(opt_db, "timewarp", &str))
    {
        std::string numerStr, denomStr;
        if(auto pos = str.find('/'); pos != std::string::npos)
        {
            numerStr = str.substr(0, pos);
            denomStr = str.substr(pos + 1);
        }
        else
        {
            numerStr = str;
            denomStr = "1";
        }

        int numer = 0, denom = 0;
        try
        {
            numer = std::stoi(numerStr);
            denom = std::stoi(denomStr);
        }
        catch(...)
        {
        }

        if(numer > 0 && denom > 0)
            ROMlib_SetTimewarp(numer, denom);
        else
        {
            fprintf(stderr, "%s: bad arguments to -timewarp\n",
                    ROMlib_appname.c_str());
            bad_arg_p = true;
        }
    }


    if(bad_arg_p)
        reportBadArgs();
}

int main(int argc, char **argv)
{
#if defined(__linux__) && defined(PERSONALITY_HACK)
    int pers;

    // TODO: figure out how much of this is still necessary.
    // MMAP_PAGE_ZERO should be unnecessary now,
    // but the 32-bit optimized assembly stuff might need READ_IMPLIES_EXEC.
    pers = personality(0xffffffff);
    if((pers & MMAP_PAGE_ZERO) == 0)
    {
        if(personality(pers | MMAP_PAGE_ZERO | READ_IMPLIES_EXEC) == 0)
            execv(argv[0], argv);
    }
#endif

    ROMlib_command_line = construct_command_line_string(argc, argv);

    /* Guarantee various time variables are set up properly. */
    msecs_elapsed();

    ROMlib_appname = fs::path(argv[0]).filename().string();

    parseCommandLine(argc, argv);

    if(flag_playback)
        EventSink::instance = std::make_unique<EventPlayback>(*flag_playback);
    else if(flag_record)
        EventSink::instance = std::make_unique<EventRecorder>(*flag_record);
    else
        EventSink::instance = std::make_unique<EventSink>();

    if(flag_headless)
        vdriver = std::make_unique<HeadlessVideoDriver>(EventSink::instance.get());
    else
        vdriver = std::make_unique<DefaultVDriver>(EventSink::instance.get(), argc, argv);
        
    checkBadArgs(argc, argv);

    auto executorThread = std::thread([&] {
        try
        {
            char thingOnStack; /* used to determine an approximation of the stack base address */
            InitMemory(&thingOnStack);

            initialize_68k_emulator(nullptr,
                                    use_native_code_p,
                                    (uint32_t *)SYN68K_TO_US(0),
                                    0);

            EM_A7 = ptr_to_longint(LM(CurStackBase));

            Executor::traps::init(logtraps);
            InitLowMem();
            syncint_init(); // timer interrupts: must not be inited before cpu & trapvevtors

            ROMlib_InitGDevices();
            
            ROMlib_eventinit();
            hle_init();

            ROMlib_fileinit();

            InitUtil();

            InitResources();
            
            ROMlib_set_system_version(system_version);

            {
                bool keyboard_set_failed = false;

                if(!keyboard.empty())
                {
                    keyboard_set_failed = !ROMlib_set_keyboard(keyboard.c_str());
                    if(keyboard_set_failed)
                        printf("``%s'' is not an available keyboard\n", keyboard.c_str());
                }

                if(keyboard_set_failed || list_keyboards_p)
                    display_keyboard_choices();
            }

            InitAppFiles(argc, argv);

            InitFonts();

        #if !defined(NDEBUG)
            dump_init(nullptr);
        #endif

            ROMlib_color_init();

            wind_color_init();
            image_inits();  // must be called after `ROMlib_color_init ()'
            sb_ctl_init();  //  must be after `image_inits ()'

            AE_init();

            {
                INTEGER env = 0;
                ROMlib_Fsetenv(inout(env), 0);
            }

            sound_init();

            set_refresh_rate(ROMlib_refresh);

            InitMonDebugger();
            base::Debugger::instance->setBreakOnProcessEntry(breakOnProcessStart);

            executor_main();
            ExitToShell();   
        }
        catch(const ExitToShellException& e)
        {
        }

        vdriver->endEventLoop();
    });

    vdriver->runEventLoop();
    executorThread.join();

    vdriver.reset();

    /* NOT REACHED */
    return 0;
}
