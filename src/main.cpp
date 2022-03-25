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
#include <debug/mon_debugger.h>
#include <PowerCore.h>
#include <vdriver/eventrecorder.h>
#include <commandline/parsenum.h>

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

#include <boost/any.hpp>
namespace boost::program_options {
template<class T, class charT>
void validate(boost::any& v,
                const std::vector<std::basic_string<charT> >& s,
                std::optional<T>*,
                int);
}
#include <boost/program_options.hpp>
#include <iostream>

using namespace Executor;
using namespace std;

namespace po = boost::program_options;

/* Set to true if there was any error parsing arguments. */
static bool bad_arg_p = false;


static bool use_native_code_p = true;
static bool breakOnProcessStart = false;
static bool logtraps = false;
static std::string keyboard;
static bool list_keyboards_p = false;

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

static void reportBadArgs()
{
    fprintf(stderr,
            "Type \"%s -help\" for a list of command-line options.\n",
            ROMlib_appname.c_str());
    exit(-10);
}

static void checkBadArgs(const std::vector<std::string>& args)
{
    if(!args.empty())
    {
        int a;

        /* Only complain if we see something with a leading dash; anything
         * else might be a file to launch.
         */
        for(const auto& arg : args)
        {
            if(arg[0] == '-')
            {
                fprintf(stderr, "%s: unknown option `%s'\n",
                        ROMlib_appname.c_str(), arg.c_str());
                bad_arg_p = true;
            }
        }
    }

    if(bad_arg_p)
        reportBadArgs();
}

bool flag_headless = false;
std::optional<fs::path> flag_record, flag_playback;


namespace boost::filesystem
{
    void validate(boost::any& v, const std::vector<std::string>& s, fs::path*, int)
    {
        v = fs::path(po::validators::get_single_string(s));
    }
}

/** Validates optional arguments. */
template<class T, class charT>
void boost::program_options::validate(boost::any& v,
                const std::vector<std::basic_string<charT> >& s,
                std::optional<T>*,
                int)
{
    boost::program_options::validators::check_first_occurrence(v);
    boost::program_options::validators::get_single_string(s);
    boost::any a;
    validate(a, s, (T*)0, 0);
    v = boost::any(std::optional<T>(boost::any_cast<T>(a)));
}


template<typename T>
class extended_typed_value : public po::typed_value<T>
{
    std::function<T(const std::string&)> parser_;
public:
    using po::typed_value<T>::typed_value;

    extended_typed_value<T>* parser(std::function<T(const std::string&)> p)
    {
        parser_ = std::move(p);
        return this;
    }

    virtual void xparse(boost::any& value_store, 
                const std::vector< std::string >& new_tokens) 
        const override
    {
        if (parser_)
            value_store = parser_(po::validators::get_single_string(new_tokens));
        else
            po::typed_value<T>::xparse(value_store, new_tokens);
    }
};

template<typename T>
extended_typed_value<T>* xvalue(T *p = nullptr)
{
    return new extended_typed_value<int>(p);
}

po::typed_value<bool>* inverted_bool_switch(bool *p = nullptr)
{
    return po::bool_switch(p)->default_value(true)->implicit_value(false);
}

static void updateArgcArgv(int& argc, char **argv, std::vector<std::string>& args)
{
    for(int i = 0; i < args.size(); i++)
        argv[i + 1] = args[i].data();
    argc = args.size() + 1;
    argv[argc] = nullptr;
}

struct SilentBadArgException {};

static int parseMemoryArgument(const std::string& s)
{
    int32_t tmp;
    if (!parse_number(s, &tmp, 1))
        throw po::invalid_option_value(s);
    return tmp;
}

static std::vector<std::string> parseCommandLine(int& argc, char **argv)
{
    po::options_description desc;

    bool modeHelp = false, modeKeyboards = false, modeVersion = false;

    po::options_description modes("Getting Information");
    modes.add_options()
        ("help,h", po::bool_switch(&modeHelp))
        ("version,v", po::bool_switch(&modeVersion))
        ("keyboards", po::bool_switch(&modeKeyboards))
        ;
    desc.add(modes);
 
    po::options_description screen("Screen");
    screen.add_options()
        ("bpp", po::value(&flag_bpp)->notifier([](int d) {
            if(d > 32 || d < 1 || (d & (d-1)))
                throw po::invalid_option_value("Argument to --bpp must be 1,2,4,8,16 or 32.");
        }), "screen depth (1,2,4,8,16,32)")
        ("size", po::value<std::string>()->notifier([&](const std::string& s) {
            if(!parse_size_opt("size", s))
                throw SilentBadArgException();
        }), "screen size in pixels")
        ("grayscale", po::bool_switch(&flag_grayscale), "grayscale graphics (for use with --bpp 2,4,8)");
    desc.add(screen);


    po::options_description printing("Printing");
    printing.add_options()
        ("print", po::bool_switch(&ROMlib_print), "tell emulated application to print the specified document(s)")
        ("cities", inverted_bool_switch(&ROMlib_fontsubstitution), "do not substitute standard PostScript fonts for classic Mac fonts")
        ("prvers", po::value<std::string>()->notifier([&](const std::string& s) {
            uint32_t vers;

            if(!ROMlib_parse_version(s, &vers))
                throw SilentBadArgException();
            else
                ROMlib_PrDrvrVers = (vers >> 8) * 10 + ((vers >> 4) & 0xF);
        }), "printer driver version to report to the application")
        ("prres", po::value<std::string>()->notifier([&](const std::string& s) {
            if(!parse_prres_opt(&ROMlib_optional_res_x, &ROMlib_optional_res_y, s))
                throw SilentBadArgException();
        }), "printer resolution");
    desc.add(printing);

    po::options_description testing("Automated Testing");
    testing.add_options()
        ("headless", po::bool_switch(&flag_headless), "disable all graphics output")
        ("record", po::value(&flag_record), "record events to file")
        ("playback", po::value(&flag_playback), "play back events from file")
        ("timewarp", po::value<std::string>()->notifier([&](const std::string& str) {
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
                throw po::invalid_option_value("bad arguments to --timewarp");
        }), "speed up or slow down time")
        ;
    desc.add(testing);

    po::options_description debugging("Debugging");
    debugging.add_options()
        ("logtraps", po::bool_switch(&logtraps), "print all operating system and toolbox calls and their arguments")
        ("break", po::bool_switch(&breakOnProcessStart), "break into debugger at program start")
        ("debug", po::value<std::string>()->notifier([](const std::string& s) {
            if (!error_parse_option_string(s.c_str()))
                throw SilentBadArgException();
        }), 
            "enable certain debugging output and consistency checks.  This "
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
            "\"trace\" enables miscellaneous  trace information, "
            "\"sound\" enables miscellaneous sound logging information, "
            "\"trapfailure\" enables warnings when traps return error codes, "
            "\"errno\" enables some C library-related warnings, "
            "\"unexpected\" enables warnings for unexpected events, "
            "\"unimplemented\" enables warnings for unimplemented traps.  "
            "Example: \"executor -debug unimp,trace\""
        )
        ;
    desc.add(debugging);


    po::options_description emulation("Emulation");
    emulation.add_options()
        ("ppc", po::bool_switch(&ROMlib_prefer_ppc), "prefer PowerPC code in FAT binaries")
#ifdef GENERATE_NATIVE_CODE
        ("no-jit", inverted_bool_switch(&use_native_code_p), "enable JIT compiler")
#endif
        /* TODO: ("memory", po::value<std::size_t>()->notify([](size_t x) { std::cout << "a\n"; }), 
            "specify the total memory you want reserved for use by the programs "
            "run under Executor and for Executor's internal system software.  "
            "For example, \"executor -memory 5.5M\" would "
            "make five and a half megabytes available to the virtual machine.  "
            "Executor will require extra memory above and beyond this amount "
            "for other uses."
        )*/
        ("applzone", xvalue(&ROMlib_applzone_size)->parser(&parseMemoryArgument), 
            "specify the memory to allocate for the application being run, "
            "e.g. \"executor -applzone 4M\" would make four megabytes "
            "of RAM available to the application.  \"applzone\" stands for "
            "\"application zone\".")
        ("syszone", xvalue(&ROMlib_syszone_size)->parser(&parseMemoryArgument),
            "like -applzone, but specifies the amount of memory to make "
            "available to Executor's internal system software."
        )
        ("stack", xvalue(&ROMlib_stack_size)->parser(&parseMemoryArgument), 
            "like -applzone, but specifies the amount of stack memory to allocate."
        )
        ("system", po::value<std::string>()->notifier([&](const std::string& s) {
            if (!parse_system_version(s))
                throw SilentBadArgException();
        }), "specify the system version that executor reports to applications")
        ;
    desc.add(emulation);

    po::options_description sound("Sound");
    sound.add_options()
        ("nosound", inverted_bool_switch(&sound_disabled_p), "disabe sound output")
        ;
    desc.add(sound);

    po::options_description misc("Miscellaneous");
    misc.add_options()
        ("noclock", po::bool_switch(&ROMlib_noclock), "disable timer interrupt")
        ("speech", po::bool_switch(&ROMlib_speech_enabled), "enable speech manager (mac hosts only)")
        ("noautorefresh", inverted_bool_switch(&do_autorefresh_p), "turns off automatic detection of programs that bypass QuickDraw")
        ("refresh", po::value(&ROMlib_refresh)
            ->implicit_value(10),
            "Handle programs that bypass QuickDraw, at a performance penalty."
            "Follow -refresh with an number indicating how many 60ths of a second "
            "to wait between each screen refresh, e.g. \"executor -refresh 10\".")
        ("appearance", po::value<std::string>()->notifier([&](const std::string& s) {
            if(!ROMlib_parse_appearance(s.c_str()))
                throw SilentBadArgException();
        }), "(mac or windows) specify the appearance of windows and "
                    "menus.  For example \"executor -appearance windows\" will make each "
                    "window have a blue title bar")
        ("scancodes", po::bool_switch(&ROMlib_use_scan_codes), 
            "different form of key mapping (may be useful in "
            "conjunction with -keyboard; not supported for all vdrivers)")
        ("keyboard", po::value(&keyboard), "choose a specific keyboard map")
#if defined(__linux__)
        ("nodrivesearch", po::bool_switch(&nodrivesearch_p), 
            "Do not look for a floppy drive, CD-ROM drive or any other drive "
            "except as specified by the MacVolumes environment variable"
        )
#endif
        ("nobrowser", po::bool_switch(&ROMlib_nobrowser), "don't run Browser")
        ("sticky", po::bool_switch(&ROMlib_sticky_menus_p), "sticky menus")
        ;
    desc.add(misc);

    po::variables_map vm;
    auto parsed = po::command_line_parser(argc, argv)
          .options(desc)
          .allow_unregistered()
          .run();
    po::store(parsed, vm);
    po::notify(vm);

    std::cout << argc << ":";
    for(int i = 0; i < argc; i++)
        std::cout << " " << argv[i];
    std::cout << std::endl;

    std::vector<std::string> unrecognized = po::collect_unrecognized(parsed.options, po::include_positional);

    for(auto x : unrecognized)
        std::cout << " " << x;
    std::cout << std::endl;
    if(modeHelp)
    {
        std::cout << desc;
        exit(0);
    }
    else if(modeVersion)
    {
        fprintf(stdout, "%s\n", EXECUTOR_VERSION);
        exit(0);
    }
    else if(modeKeyboards)
    {
        list_keyboards_p = true;
        flag_headless = true;
    }

    return unrecognized;
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

    /* Guarantee various time variables are set up properly. */
    msecs_elapsed();

    ROMlib_appname = fs::path(argv[0]).filename().string();
    auto remainingArgs = parseCommandLine(argc, argv);

    if(flag_playback)
        EventSink::instance = std::make_unique<EventPlayback>(*flag_playback);
    else if(flag_record)
        EventSink::instance = std::make_unique<EventRecorder>(*flag_record);
    else
        EventSink::instance = std::make_unique<EventSink>();

    updateArgcArgv(argc, argv, remainingArgs);

    if(flag_headless)
        vdriver = std::make_unique<HeadlessVideoDriver>(EventSink::instance.get());
    else
        vdriver = std::make_unique<DefaultVDriver>(EventSink::instance.get(), argc, argv);
    
    remainingArgs = std::vector<std::string>(argv + 1, argv + argc);
    
    checkBadArgs(remainingArgs);

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

            InitAppFiles(remainingArgs);

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
