#if !defined(_RSYS_OSEVENT_H_)
#define _RSYS_OSEVENT_H_

#include <EventMgr.h>
#include <OSEvent.h>

namespace Executor
{
extern INTEGER ROMlib_mods;

extern OSErr ROMlib_PPostEvent(INTEGER evcode, LONGINT evmsg,
                                  GUEST<EvQElPtr> *qelp, LONGINT when, Point where, INTEGER butmods);

extern Ptr ROMlib_kchr_ptr(void);

extern void invalidate_kchr_ptr(void);

enum
{
    NUMPAD_ENTER = 0x3
};

enum
{
    MODIFIER_SHIFT = 8,
    VIRT_MASK = 0x7F
};

enum
{
    MODIFIER_MASK = (ControlKey << 1) - 1
};

enum
{
    sizeof_KeyMap = 16
}; /* can't use sizeof(LM(KeyMap)) */

extern LONGINT ROMlib_xlate(INTEGER virt, INTEGER modifiers,
                            bool down_p);

extern void ROMlib_eventinit();

extern void post_keytrans_key_events(INTEGER evcode, LONGINT keywhat,
                                     int32_t when, Point where,
                                     uint16_t button_state, unsigned char virt);

extern void display_keyboard_choices(void);

extern bool ROMlib_set_keyboard(const char *keyboardname);
extern Boolean ROMlib_bewaremovement;
extern void ROMlib_showhidecursor(void);
extern void maybe_wait_for_keyup(void);

extern uint16_t ROMlib_right_to_left_key_map(uint16_t what);

extern bool ROMlib_get_index_and_bit(LONGINT loc, int *indexp,
                                     uint8_t *bitp);

extern bool hle_get_event(EventRecord *evt, bool remflag);
extern void hle_init(void);
extern void hle_reinit(void);
extern void hle_reset(void);
}
#endif
