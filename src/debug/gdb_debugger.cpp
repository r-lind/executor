#include <debug/gdb_debugger.h>

#include <base/debugger.h>
#include <res/resource.h>
#include <util/macstrings.h>
#include <SegmentLdr.h>
#include <MemoryMgr.h>

#include <boost/asio.hpp>

#include <thread>
#include <iostream>
#include <deque>
#include <string>

using namespace Executor;

using boost::asio::ip::tcp;

class GdbDebugger;

class GdbConnection
{
private:
    std::string partial_;
    //std::function<void (const std::string&)> handler;
    std::size_t hashAt_ = 0;
    char started_ = 0;
    uint8_t sum_ = 0;
    bool newConnection_ = true;
    
    void clear();

    std::mutex mutex_;
    std::condition_variable cond_;
    std::deque<std::string> cmdqueue_;

    GdbDebugger& debugger_;

protected:
    void receive(const char* p, std::size_t n);
    virtual void sendRaw(const std::string& str) = 0;
    void requestInterrupt();

public:
    void send(const std::string& s);

    GdbConnection(GdbDebugger& debugger) : debugger_(debugger) {}

    std::string getNextRequest();
    bool isNew() { return newConnection_; }
};



template<typename Socket, typename Acceptor>
class AsioGdbConnection : public GdbConnection
{
protected:
    boost::asio::io_context& context_;
    Acceptor acceptor_;
    Socket socket_;

    std::array<char, 128> readBuf_;


    virtual void sendRaw(const std::string& str) override
    {
        boost::asio::write(socket_, boost::asio::buffer(str));
    }
    
    void onAccept(boost::system::error_code error)
    {
        if(!error)
        {
            requestInterrupt();
            startReading();
        }
    }
    
    void onRead(boost::system::error_code error, size_t n)
    {
        if(error)
        {
            socket_ = Socket(context_);
            acceptor_.async_accept(socket_, [this](const boost::system::error_code& e) { onAccept(e); });
        }
        else
        {
            receive(readBuf_.data(), n);
            startReading();
        }
    }

    void startReading()
    {
        socket_.async_read_some(boost::asio::buffer(readBuf_),
            [this](const boost::system::error_code& error, size_t n)
            { onRead(error, n); });
    }
    
public:
    AsioGdbConnection(GdbDebugger& debugger, boost::asio::io_context& context, Acceptor acceptor)
        : GdbConnection(debugger), context_(context), acceptor_(std::move(acceptor)), socket_(context_)
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code e) { onAccept(e); });
    }
};


class GdbDebugger : public base::Debugger
{
    std::thread thread_;
    boost::asio::io_context io_context_;    

    std::atomic_bool interrupt_;
    
    AsioGdbConnection<tcp::socket, tcp::acceptor> tcp68k_;
    
public:
    GdbDebugger();
    ~GdbDebugger();

    virtual bool interruptRequested() override { return interrupt_; }
    
    void requestInterrupt() { interrupt_ = true; }
    
    virtual DebuggerExit interact(DebuggerEntry e) override;
    
    std::string handleGdbRequest(const std::string request);
};


void GdbConnection::clear()
{
    partial_.clear();
    hashAt_ = 0;
    started_ = 0;
    sum_ = 0;
}

void GdbConnection::receive(const char* p, std::size_t n)
{
    while(n > 0)
    {
        char c = *p++;
        --n;
                    
        if(!started_)
        {
            if(c == '$')
                started_ = c;
            else if(c == '\003')
               debugger_.requestInterrupt();
            else
                std::cout << "ignored " << c << std::endl;
        }
        else
        {
            partial_ += c;
            if(hashAt_)
            {
                if(partial_.size() == hashAt_ + 3)
                {
                    int expected;
                    sscanf(partial_.data() + partial_.size() - 2, "%x", &expected);
                    
                    if(expected == sum_)
                    {
                        sendRaw("+");
                        partial_.resize(partial_.size() - 3);
                        std::cout << "request: " << partial_ << std::endl;
                        
                        std::lock_guard lk(mutex_);
                        cond_.notify_one();
                        cmdqueue_.push_back(partial_);
                    }
                    else
                    {
                        sendRaw("-");
                    }
                    
                    clear();
                }
            }
            else
            {
                if(c == '#')
                    hashAt_ = partial_.size() - 1;
                else
                    sum_ += (uint8_t)c;
            }
        }
    }
}

void GdbConnection::send(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 10);
    
    out += '$';
    
    int sum = 0;
    for(char c : s)
    {
        out += c;
        sum += (uint8_t)c;
    }
    
    char sumhex[3];
    sprintf(sumhex, "%02x", sum & 0xFF);
    
    out += "#";
    out += sumhex;
    
    std::cout << "sending reply: " << out << std::endl;
    sendRaw(out);
}

void GdbConnection::requestInterrupt()
{
    debugger_.requestInterrupt();
}

std::string GdbConnection::getNextRequest()
{
    std::unique_lock lk(mutex_);
    newConnection_ = false;
    cond_.wait(lk, [this] { return !cmdqueue_.empty(); });
    std::string result = std::move(cmdqueue_.front());
    cmdqueue_.pop_front();
    return result;
}

#ifdef WIN32
static bool safeRead(uint8_t *ptr, uint8_t& result)
{
    result = *ptr;
    return true;
}
#else
static bool pipeInited = false;
static int memAccessPipe[2];

static bool safeRead(uint8_t *ptr, uint8_t& result)
{
    if(!pipeInited)
    {
        pipe(memAccessPipe);
        pipeInited = true;
    }
    
    auto written = write(memAccessPipe[1], ptr, 1);
    
    if(written == 1)
    {
        read(memAccessPipe[0], &result, 1);
        return true;
    }
    else
        return false;
}
#endif

auto GdbDebugger::interact(DebuggerEntry entry) -> DebuggerExit
{
    interrupt_ = false;
    
    auto *connection = &tcp68k_;
    std::string stopReason = "S05";
    
    if(!connection->isNew())
        connection->send(stopReason);
    
    for(;;)
    {
        std::string request = connection->getNextRequest();
        
        if(request.empty())
            ;
        else if(request.substr(0,10) == "qSupported")
            connection->send("requestSize=47ff;qXfer:libraries:read+");
        else if(request == "qAttached")
            connection->send("1");
        else if(request == "qfThreadInfo")
        {
            connection->send("m 1");
        }
        else if(request == "qsThreadInfo")
        {
            connection->send("l");
        }
        else if(request.substr(0,2) == "qL")
        {
            connection->send("qM011000000000000002a");
        }
        else if(request.substr(0,1) == "H")
            connection->send("OK");
        else if(request == "qC") // current thread
            connection->send("QC1");
        else if(request == "?")
            connection->send(stopReason);
        else if(request == "g")
        {
            std::string regs;
            for(int i = 0; i < 18; i++)
            {
                //regs += "00000000";
                char tmp[10];
                
                uint32_t val;
                
                if(i < 16)
                    val = cpu_state.regs[i].ul.n;
                else if(i == 16)
                    val = cpu_state.sr;
                else if(i == 17)
                    val = entry.addr;
                sprintf(tmp, "%08X", (unsigned)val);
                regs += tmp;
            }
            connection->send(regs);
        }
        else if(request == "qOffsets")
        {
            auto map = ROMlib_rntohandl(LM(CurApRefNum), nullptr);
            resref *rr = nullptr;

            if(map && ROMlib_maptypidtop(map, "CODE"_4, 1, &rr) == noErr && rr && rr->rhand && *rr->rhand)
            {
                uint32_t base = guest_cast<uint32_t>(*rr->rhand);
                char response[64];
                sprintf(response, "TextSeg=%x;DataSeg=0", (unsigned)base + 4);
                connection->send(response);
            }
            else
                connection->send("");
            
        }
        else if(request.substr(0, 22) == "qXfer:libraries:read::")
        {
            char *p;
            uint32_t addr = std::strtoul(request.data() + 22, &p, 16);
            uint32_t count = std::strtoul(p + 1, nullptr, 16);

            auto appname = toUnicodeFilename(mac_string_view(LM(CurApName)));
            auto debugSymbolsStr = appname.filename().stem().string() + ".code.bin.gdb";

            auto map = ROMlib_rntohandl(LM(CurApRefNum), nullptr);
            resref *rr = nullptr;

            std::vector<uint32_t> sections;            
            for(int id = 1;
                map && ROMlib_maptypidtop(map, "CODE"_4, id, &rr) == noErr && rr && rr->rhand && *rr->rhand;
                id++)
            {
                uint32_t base = guest_cast<uint32_t>(*rr->rhand);
                sections.push_back(id == 1 ? base + 4 : base + 40);
            }

            sections.push_back(ptr_to_longint(LM(CurStackBase)));

            std::ostringstream stream;

            stream << R"raw(
                <library-list>
                <library name=")raw" << debugSymbolsStr << "\">";
            for(uint32_t base : sections)
                stream << "<segment address=\"0x" << std::hex << base << "\"/>";
            stream << R"raw(</library>
                </library-list>
            )raw";

            auto libsxml = stream.str();

            if(addr > libsxml.size())
                connection->send("E00");
            else
            {
                std::string response;
                if(addr + count >= libsxml.size())
                    response = "l";
                else
                    response = "m";

                for(char c : libsxml.substr(addr, count))
                {
                    switch(c)
                    {
                        case '}':
                        case '*':
                        case '#':
                        case '$':
                            response += '}';
                            response += (char) (c ^ 0x20);
                            break;
                        default:
                            response += c;
                    }
                }
                connection->send(response);
            }
        }
        else if(request[0] == 'm')
        {
            char *p;
            uint32_t addr = std::strtoul(request.data() + 1, &p, 16);
            uint32_t count = std::strtoul(p + 1, nullptr, 16);
            
            std::string mem;
            mem.reserve(count * 2);
            for(uint32_t i = 0; i < count; i++)
            {
                char tmp[10];
                
                uint32_t stripped = (addr + i) & ADDRESS_MASK;
                uint32_t segment = stripped >> (ADDRESS_BITS - OFFSET_TABLE_BITS);
                
                if((stripped & ((1 << (ADDRESS_BITS - OFFSET_TABLE_BITS))-1)) > ROMlib_sizes[segment])
                    break;

                uint8_t *native = (uint8_t*)( (uintptr_t)stripped + ROMlib_offsets[segment] );

                uint8_t data;
                if(!safeRead(native, data))
                    break;
                
                sprintf(tmp, "%02X", (unsigned)*native);
                
                mem += tmp;
            }
            connection->send(mem);
        }
        else if(request[0] == 'M')
        {
            char *p;
            uint32_t addr = std::strtoul(request.data() + 1, &p, 16);
            uint32_t count = std::strtoul(p + 1, &p, 16);
            ++p;
            for(uint32_t i = 0; i < count; i++)
            {
                
                uint32_t stripped = (addr + i) & ADDRESS_MASK;
                uint32_t segment = stripped >> (ADDRESS_BITS - OFFSET_TABLE_BITS);
                
                if((stripped & ((1 << (ADDRESS_BITS - OFFSET_TABLE_BITS))-1)) > ROMlib_sizes[segment])
                    break;

                uint8_t *native = (uint8_t*)( (uintptr_t)stripped + ROMlib_offsets[segment] );

                uint8_t data;
                if(!safeRead(native, data))
                    break;
                
                char tmp[3] = { p[0], p[1], 0 };
                unsigned val;
                sscanf(tmp, "%X", &val);
                *native = val;
                p+=2;
            }
            connection->send("OK");
        }
        else if(request[0] == 'c')
        {
            return { entry.addr, false };
        }
        else if(request[0] == 's')
        {
            return { entry.addr, true };
        }
        else
            connection->send("");
    }
    
    return { entry.addr, false };
}


GdbDebugger::GdbDebugger()
    : tcp68k_(*this, io_context_, tcp::acceptor(io_context_, tcp::endpoint(tcp::v4(), 2159)))
{
    interrupt_ = false;
    thread_ = std::thread([&] {
        io_context_.run();
    });
}

GdbDebugger::~GdbDebugger()
{
    io_context_.stop();
    thread_.join();
}

void Executor::InitGdbDebugger()
{
    try
    {
        base::Debugger::instance = new GdbDebugger();
    }
    catch(const boost::system::system_error& err)
    {
        std::cout << "Error opening listener socket.\n";
    }
}

