#include <cstdio>
#include <cstring>
#include <pead/peadPrint.h>
#include <pead/peadStringUtil.h>

#if defined(__WIIU__)
#include <coreinit/debug.h>
#endif

namespace pead
{

    namespace
    {

        SystemPrintOutput s_systemOutput;
        PrintOutput *s_defaultOutput               = &s_systemOutput;
        IDelegate1<const SafeString &> *s_listener = nullptr;

    } // namespace

    namespace system
    {

        void PrintString(const char *str, s32 length)
        {
            if (!str)
                return;
            if (length < 0)
                length = (s32) strlen(str);
#if defined(__WIIU__)
            // OSReport takes a format, so we pass the text through %.*s to keep any % in it literal.
            OSReport("%.*s", (int) length, str);
#else
            fwrite(str, 1, (size_t) length, stderr);
#endif
        }

        void Print(const char *format, ...)
        {
            char buffer[PrintFormatter::cBufferSize];
            va_list args;
            va_start(args, format);
            s32 length = StringUtil::vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            PrintString(buffer, length);
        }

    } // namespace system

    PrintOutput *PrintConfig::getDefaultOutput()
    {
        return s_defaultOutput;
    }

    PrintOutput *PrintConfig::setDefaultOutput(PrintOutput *output)
    {
        PrintOutput *previous = s_defaultOutput;
        s_defaultOutput       = output ? output : &s_systemOutput;
        return previous;
    }

    void PrintConfig::setListener(IDelegate1<const SafeString &> *listener)
    {
        s_listener = listener;
    }

    IDelegate1<const SafeString &> *PrintConfig::getListener()
    {
        return s_listener;
    }

    PrintFormatter &PrintFormatter::format(const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        m_buffer.appendWithFormatV(format, args);
        va_end(args);
        return *this;
    }

    void PrintFormatter::flush()
    {
        if (m_buffer.isEmpty())
            return;
        m_output->write(m_buffer.cstr(), m_buffer.calcLength());
        if (s_listener && m_output == s_defaultOutput)
            s_listener->invoke(m_buffer);
        m_buffer.clear();
    }

    template<>
    PrintFormatter &PrintFormatter::out<s32>(const s32 &value)
    {
        return format("%d", (int) value);
    }

    template<>
    PrintFormatter &PrintFormatter::out<u32>(const u32 &value)
    {
        return format("%u", (unsigned) value);
    }

    template<>
    PrintFormatter &PrintFormatter::out<s64>(const s64 &value)
    {
        return format("%lld", (long long) value);
    }

    template<>
    PrintFormatter &PrintFormatter::out<u64>(const u64 &value)
    {
        return format("%llu", (unsigned long long) value);
    }

    template<>
    PrintFormatter &PrintFormatter::out<f32>(const f32 &value)
    {
        return format("%g", (double) value);
    }

    template<>
    PrintFormatter &PrintFormatter::out<f64>(const f64 &value)
    {
        return format("%g", value);
    }

    template<>
    PrintFormatter &PrintFormatter::out<bool>(const bool &value)
    {
        return format("%s", value ? "true" : "false");
    }

    template<>
    PrintFormatter &PrintFormatter::out<SafeString>(const SafeString &value)
    {
        return format("%s", value.cstr());
    }

} // namespace pead
