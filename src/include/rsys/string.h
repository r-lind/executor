#if !defined(_RSYS_STRING_H_)
#define _RSYS_STRING_H_

namespace Executor
{

extern void str255_from_c_string(Str255 str255, const char *c_stringp);
extern char *pstr_index_after(StringPtr p, char c, int i);
extern void str63assign(Str63 new1, const StringPtr old);
extern void str31assign(Str63 new1, const StringPtr old);
extern StringHandle stringhandle_from_c_string(const char *c_stringp);
}

#endif /* !_RSYS_STRING_H_ */
