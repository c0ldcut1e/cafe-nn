/**
 * @file
 * @brief This is pead's debug output. system::PrintString sends text to the console on every platform, and PrintFormatter builds a line with a fixed-size buffer.
 */

#pragma once

#include <pead/peadDelegate.h>
#include <pead/peadSafeString.h>

namespace pead
{
    namespace system
    {
        /**
         * @brief This writes text to the debug console: OSReport on Wii U and stderr elsewhere.
         *
         * @param str The text, which is written as is with no formatting.
         * @param length The length, or -1 when str is terminated.
         */
        void PrintString(const char *str, s32 length);
        void Print(const char *format, ...);
    } // namespace system

    class PrintOutput
    {
    public:
        virtual ~PrintOutput()                          = default;
        virtual void write(const char *str, s32 length) = 0;
    };

    /**
     * @brief This writes through system::PrintString.
     */
    class SystemPrintOutput : public PrintOutput
    {
    public:
        void write(const char *str, s32 length) override
        {
            system::PrintString(str, length);
        }
    };

    /**
     * @brief This appends to a BufferedSafeString, cutting off at its size.
     */
    class BufferingPrintOutput : public PrintOutput
    {
    public:
        explicit BufferingPrintOutput(BufferedSafeString *buffer) : m_buffer(buffer)
        {}
        void write(const char *str, s32 length) override
        {
            m_buffer->append(SafeString(str), length);
        }

    private:
        BufferedSafeString *m_buffer;
    };

    /**
     * @brief This holds where print output goes by default and lets callers watch it.
     */
    class PrintConfig
    {
    public:
        static PrintOutput *getDefaultOutput();

        /**
         * @brief This changes the default output.
         *
         * @param output The new output, or null for the system console.
         * @return The previous output.
         */
        static PrintOutput *setDefaultOutput(PrintOutput *output);

        /**
         * @brief This calls listener with every line written to the default output. Pass null to stop.
         */
        static void setListener(IDelegate1<const SafeString &> *listener);
        static IDelegate1<const SafeString &> *getListener();
    };

    /**
     * @brief This formats one line of output into its own buffer and writes it to an output when flushed or destroyed.
     */
    class PrintFormatter
    {
    public:
        static constexpr s32 cBufferSize = 512;

        explicit PrintFormatter(PrintOutput *output = nullptr) : m_output(output ? output : PrintConfig::getDefaultOutput())
        {}
        ~PrintFormatter()
        {
            flush();
        }
        PrintFormatter(const PrintFormatter &)            = delete;
        PrintFormatter &operator=(const PrintFormatter &) = delete;

        PrintFormatter &format(const char *format, ...);

        /**
         * @brief This appends a value in its natural text form.
         */
        template<typename T>
        PrintFormatter &out(const T &value);

        template<typename T>
        PrintFormatter &operator<<(const T &value)
        {
            return out(value);
        }

        /**
         * @brief This writes what has been formatted and empties the buffer.
         */
        void flush();

    private:
        PrintOutput *m_output;
        FixedSafeString<cBufferSize> m_buffer;
    };

    template<>
    PrintFormatter &PrintFormatter::out<s32>(const s32 &value);
    template<>
    PrintFormatter &PrintFormatter::out<u32>(const u32 &value);
    template<>
    PrintFormatter &PrintFormatter::out<s64>(const s64 &value);
    template<>
    PrintFormatter &PrintFormatter::out<u64>(const u64 &value);
    template<>
    PrintFormatter &PrintFormatter::out<f32>(const f32 &value);
    template<>
    PrintFormatter &PrintFormatter::out<f64>(const f64 &value);
    template<>
    PrintFormatter &PrintFormatter::out<bool>(const bool &value);
    template<>
    PrintFormatter &PrintFormatter::out<SafeString>(const SafeString &value);

} // namespace pead
