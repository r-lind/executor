#pragma once

#include <chrono>
#include <vector>
#include <functional>
#include <syn68k_public.h>

class PowerCore;

namespace Executor
{
extern void syncint_init(void);
extern void syncint_post(std::chrono::microseconds usecs, bool fromLast = false);
extern void syncint_wait_interrupt();
extern void syncint_check_interrupt();


class Interrupt
{
    int index = -1;

    static constexpr int maxInterrupts = 10;
    struct Impl;
    static std::array<Impl, maxInterrupts> interrupts;
    static int nInterrupts;

    static void handleCommon();

public:
    Interrupt() = default;
    explicit Interrupt(std::function<void ()> f);

    void trigger();


    static syn68k_addr_t handle68K(syn68k_addr_t interrupt_pc, void *unused);
    static void handlePowerPC(PowerCore& cpu);
};

}
