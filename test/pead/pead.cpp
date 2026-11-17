#include <atomic>
#include <cstdio>
#include <cstring>
#include <pead/peadBase64.h>
#include <pead/peadExpHeap.h>
#include <pead/peadHeapMgr.h>
#include <pead/peadNew.h>
#include <pead/peadPrint.h>
#include <pead/peadRandom.h>
#include <pead/peadStream.h>
#include <pead/peadStringUtil.h>
#include <pead/peadThread.h>

using namespace pead;

static int g_failures;

#define CHECK(cond)                                                                                                                                  \
    do                                                                                                                                               \
    {                                                                                                                                                \
        if (!(cond))                                                                                                                                 \
        {                                                                                                                                            \
            printf("%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);                                                                          \
            g_failures++;                                                                                                                            \
        }                                                                                                                                            \
    } while (0)

static void TestExpHeap()
{
    ExpHeap *heap = ExpHeap::create(0x10000, "TestHeap", nullptr, Heap::cHeapDirection_Forward, true);
    CHECK(heap != nullptr);
    if (!heap)
        return;
    CHECK(HeapMgr::findHeapByName("TestHeap") == heap);

    u32 freeBefore = heap->getFreeSize();
    void *a        = heap->alloc(100);
    void *b        = heap->alloc(200, 64);
    void *c        = heap->alloc(300, -16);
    CHECK(a && b && c);
    CHECK(((uintptr_t) b & 63) == 0);
    CHECK(((uintptr_t) c & 15) == 0);
    CHECK(c > b);
    CHECK(heap->isInclude(a) && heap->isInclude(c));
    CHECK(HeapMgr::instance()->findContainHeap(b) == heap);

    // Freeing out of order has to merge the holes back into one block.
    heap->free(b);
    heap->free(a);
    heap->free(c);
    CHECK(heap->getFreeSize() == freeBefore);
    CHECK(heap->getMaxAllocatableSize(4) + sizeof(MemBlock) >= freeBefore - 16);

    void *big = heap->tryAlloc(0x20000, 4);
    CHECK(big == nullptr);

    void *r = heap->alloc(64);
    memset(r, 0x5A, 64);
    void *grown = heap->tryRealloc(r, 4096, 8);
    CHECK(grown != nullptr && ((u8 *) grown)[63] == 0x5A);
    heap->free(grown);

    s32 *ints = NewArray<s32>(16, heap);
    CHECK(heap->isInclude(ints));
    DeleteArray(ints);
    CHECK(heap->getFreeSize() == freeBefore);

    heap->alloc(32);
    heap->freeAll();
    CHECK(heap->getFreeSize() == freeBefore);

    heap->destroy();
    CHECK(HeapMgr::findHeapByName("TestHeap") == nullptr);
}

static void TestSafeString()
{
    FixedSafeString<16> s;
    s.format("%d-%s", 42, "abc");
    CHECK(s == SafeString("42-abc"));
    s.append("0123456789ABCDEF");
    CHECK(s.calcLength() == 15);
    s.copy("one two one");
    CHECK(s.replaceString("one", "three") == 2);
    CHECK(s == SafeString("three two three"));
    CHECK(s.findIndex("two") == 6);
    CHECK(s.rfindIndex("three") == 10);
    CHECK(s.startsWith("thr") && s.endsWith("ree"));
    s.trimMatchedString("three");
    CHECK(s == SafeString("three two "));

    WFixedSafeString<32> w;
    w.format(u"%s=%d %hs", u"x", 7, "y");
    CHECK(w == WSafeString(u"x=7 y"));
}

static void TestStringUtil()
{
    s32 v = 0;
    CHECK(StringUtil::tryParseS32(&v, " -123 ", StringUtil::cBaseAuto) && v == -123);
    CHECK(StringUtil::tryParseS32(&v, "0x1F", StringUtil::cBaseAuto) && v == 31);
    u8 small = 0;
    CHECK(!StringUtil::tryParseU8(&small, "256", StringUtil::cBase10));
    CHECK(!StringUtil::tryParseS32(&v, "12a", StringUtil::cBase10));
    f32 f = 0;
    CHECK(StringUtil::tryParseF32(&f, "1.5") && f == 1.5f);

    CHECK(StringUtil::toUpperCapital(u'a') == u'A');
    CHECK(StringUtil::toLowerCapital(u'Ж') == u'ж');
    CHECK(StringUtil::toLowerCapital(u'Ａ') == u'ａ');

    char utf8[32];
    char16_t utf16[32];
    char sjis[32];
    StringUtil::convertUtf16ToUtf8(utf8, sizeof(utf8), u"日本😀", -1);
    CHECK(strcmp(utf8, "\xE6\x97\xA5\xE6\x9C\xAC\xF0\x9F\x98\x80") == 0);
    StringUtil::convertUtf8ToUtf16(utf16, 32, utf8, -1);
    CHECK(utf16[0] == u'日' && utf16[2] == 0xD83D && utf16[3] == 0xDE00);
    s32 sjisLength = StringUtil::convertUtf16ToSjis(sjis, sizeof(sjis), u"あア｡", -1);
    CHECK(sjisLength == 5 && memcmp(sjis, "\x82\xA0\x83\x41\xA1", 5) == 0);
    StringUtil::convertSjisToUtf16(utf16, 32, sjis, -1);
    CHECK(utf16[0] == u'あ' && utf16[1] == u'ア' && utf16[2] == u'｡' && utf16[3] == 0);
}

static void TestStream()
{
    u8 buffer[64];
    {
        RamWriteStream out(buffer, sizeof(buffer), Stream::cBinary);
        out.setBinaryEndian(Endian::cBig);
        out.writeU16(0x1234);
        out.writeU32(0xDEADBEEF);
        out.writeF32(2.5f);
        out.writeString("hi", 4);
    }
    CHECK(buffer[0] == 0x12 && buffer[1] == 0x34 && buffer[2] == 0xDE);
    {
        RamReadStream in(buffer, sizeof(buffer), Stream::cBinary);
        in.setBinaryEndian(Endian::cBig);
        CHECK(in.readU16() == 0x1234);
        CHECK(in.readU32() == 0xDEADBEEF);
        CHECK(in.readF32() == 2.5f);
        FixedSafeString<8> str;
        in.readString(&str, 4);
        CHECK(str == SafeString("hi"));
    }

    char text[128] = {};
    {
        RamWriteStream out(text, sizeof(text) - 1, Stream::cText);
        out.writeS32(-5);
        out.writeComment("ignored");
        out.writeF64(0.25);
        out.writeString("a b", 0);
        out.writeMemBlock("\x01\xAB", 2);
    }
    {
        RamReadStream in(text, (u32) strlen(text), Stream::cText);
        CHECK(in.readS32() == -5);
        CHECK(in.readF64() == 0.25);
        FixedSafeString<8> str;
        in.readString(&str, 0);
        CHECK(str == SafeString("a b"));
        u8 block[2] = {};
        CHECK(in.readMemBlock(block, 2) == 2 && block[0] == 0x01 && block[1] == 0xAB);
    }
}

static void TestBase64AndRandom()
{
    char encoded[16];
    Base64::encode(encoded, "foob", 4, false);
    CHECK(strcmp(encoded, "Zm9vYg==") == 0);
    u8 decoded[8];
    u32 size = 0;
    CHECK(Base64::decode(decoded, sizeof(decoded), encoded, 8, &size) && size == 4 && memcmp(decoded, "foob", 4) == 0);

    Random a(1234);
    Random b(1234);
    CHECK(a.getU32() == b.getU32());
    for (int i = 0; i < 100; i++)
    {
        s32 x = a.getS32Range(-3, 3);
        CHECK(x >= -3 && x < 3);
    }
}

namespace
{
    class Counter
    {
    public:
        std::atomic<s32> m_sum{0};
        void onMessage(Thread *, MessageQueue::Element msg)
        {
            m_sum += (s32) msg;
        }
    };
} // namespace

static void TestThread()
{
    Counter counter;
    Delegate2<Counter, Thread *, MessageQueue::Element> delegate(&counter, &Counter::onMessage);
    DelegateThread *thread =
            new (HeapMgr::getRootHeap()) DelegateThread("TestThread", &delegate, nullptr, Thread::cDefaultPriority,
                                                        Thread::cMessageQueueBlockType_Block, Thread::cQuitMsg, Thread::cDefaultStackSize, 8);
    thread->start();
    for (s32 i = 1; i <= 10; i++) thread->sendMessage(i, MessageQueue::cBlock);
    thread->quitAndWaitDoneSingleThread();
    CHECK(counter.m_sum == 55);
    Delete(thread);
}

static void TestPrint()
{
    FixedSafeString<64> captured;
    BufferingPrintOutput output(&captured);
    {
        PrintFormatter line(&output);
        line << 12 << SafeString(" ") << true << SafeString(" ") << 1.5;
    }
    CHECK(captured == SafeString("12 true 1.5"));
}

int RunPeadTests()
{
    Arena arena;
    CHECK(arena.initialize(0x100000));
    HeapMgr::initialize(&arena);
    ThreadMgr::initialize(HeapMgr::getRootHeap());

    TestExpHeap();
    TestSafeString();
    TestStringUtil();
    TestStream();
    TestBase64AndRandom();
    TestThread();
    TestPrint();

    ThreadMgr::destroy();
    HeapMgr::destroy();
    arena.destroy();
    return g_failures;
}

int main()
{
    int failures = RunPeadTests();
    printf(failures ? "%d failures\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
