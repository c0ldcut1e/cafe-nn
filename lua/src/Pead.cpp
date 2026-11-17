#include "Binding.h"

#include <pead/peadBase64.h>
#include <pead/peadExpHeap.h>
#include <pead/peadHeapMgr.h>
#include <pead/peadRandom.h>
#include <pead/peadStream.h>
#include <pead/peadStringUtil.h>
#include <pead/peadThread.h>
#include <string>
#include <vector>

namespace nn::nex::lua
{
    namespace
    {
        // Lua owns the arena so scripts can bring pead up and down without any C++ setup.
        pead::Arena s_arena;

        // Each Initialize starts a new generation, so a heap left over from before a Finalize is never touched again.
        pead::u32 s_generation;

        struct HeapHandle
        {
            pead::ExpHeap *heap;
            pead::u32 generation;
        };

        struct RandomHandle
        {
            pead::Random random;
        };

        // The stream points into its own buffer, and userdata never moves, so the two can live side by side.
        struct WriteStreamHandle
        {
            WriteStreamHandle(pead::u32 size, pead::Stream::Modes mode) : buffer(size, '\0'), stream(buffer.data(), size, mode)
            {}
            std::string buffer;
            pead::RamWriteStream stream;
        };

        struct ReadStreamHandle
        {
            ReadStreamHandle(const char *data, size_t size, pead::Stream::Modes mode)
                : buffer(data, size), stream(buffer.data(), (pead::u32) size, mode)
            {}
            std::string buffer;
            pead::RamReadStream stream;
        };
    } // namespace

    template<>
    struct ClassName<HeapHandle>
    {
        static constexpr const char *value = "ExpHeap";
    };
    template<>
    struct ClassName<RandomHandle>
    {
        static constexpr const char *value = "Random";
    };
    template<>
    struct ClassName<WriteStreamHandle>
    {
        static constexpr const char *value = "WriteStream";
    };
    template<>
    struct ClassName<ReadStreamHandle>
    {
        static constexpr const char *value = "ReadStream";
    };

    namespace
    {
        void CheckInitialized(lua_State *L)
        {
            if (!pead::HeapMgr::isInitialized())
                luaL_error(L, "pead is not initialized, call nex.pead.Initialize first");
        }

        pead::ExpHeap *CheckHeap(lua_State *L, int index)
        {
            HeapHandle *handle = Check<HeapHandle>(L, index);
            if (!handle->heap || handle->generation != s_generation || !pead::HeapMgr::isInitialized())
                luaL_error(L, "the heap was destroyed");
            return handle->heap;
        }

        pead::Stream::Modes OptMode(lua_State *L, int index)
        {
            static const char *const names[] = {"binary", "text", nullptr};
            return luaL_checkoption(L, index, "binary", names) == 1 ? pead::Stream::cText : pead::Stream::cBinary;
        }

        pead::Endian::Types OptEndian(lua_State *L, int index)
        {
            static const char *const names[] = {"big", "little", nullptr};
            return luaL_checkoption(L, index, "big", names) == 1 ? pead::Endian::cLittle : pead::Endian::cBig;
        }

        int Initialize(lua_State *L)
        {
            if (pead::HeapMgr::isInitialized())
                return Push(L, true);
            lua_Integer size = luaL_optinteger(L, 1, 0x400000);
            if (size <= 0 || !s_arena.initialize((pead::u32) size))
                return Push(L, false);
            s_generation++;
            pead::HeapMgr::initialize(&s_arena);
            pead::ThreadMgr::initialize(pead::HeapMgr::getRootHeap());
            return Push(L, true);
        }

        int Finalize(lua_State *)
        {
            if (!pead::HeapMgr::isInitialized())
                return 0;
            pead::ThreadMgr::destroy();
            pead::HeapMgr::destroy();
            s_arena.destroy();
            s_generation++;
            return 0;
        }

        int IsInitialized(lua_State *L)
        {
            return Push(L, pead::HeapMgr::isInitialized());
        }

        int RootFreeSize(lua_State *L)
        {
            CheckInitialized(L);
            return Push(L, pead::HeapMgr::getRootHeap()->getFreeSize());
        }

        int NewExpHeap(lua_State *L)
        {
            CheckInitialized(L);
            lua_Integer size    = luaL_checkinteger(L, 1);
            const char *name    = luaL_optstring(L, 2, "LuaHeap");
            bool backward       = lua_toboolean(L, 3);
            pead::ExpHeap *heap = pead::ExpHeap::tryCreate((pead::u32) size, name, nullptr,
                                                           backward ? pead::Heap::cHeapDirection_Reverse : pead::Heap::cHeapDirection_Forward, true);
            if (!heap)
            {
                lua_pushnil(L);
                return 1;
            }
            New<HeapHandle>(L, HeapHandle{heap, s_generation});
            return 1;
        }

        int HeapGc(lua_State *L)
        {
            if (HeapHandle *handle = Test<HeapHandle>(L, 1))
            {
                if (handle->heap && handle->generation == s_generation && pead::HeapMgr::isInitialized())
                    handle->heap->destroy();
                handle->heap = nullptr;
            }
            return 0;
        }

        int HeapDestroy(lua_State *L)
        {
            CheckHeap(L, 1)->destroy();
            Check<HeapHandle>(L, 1)->heap = nullptr;
            return 0;
        }

        // Scripts get blocks as light userdata, which they can only hand back to the heap.
        int HeapAlloc(lua_State *L)
        {
            pead::ExpHeap *heap = CheckHeap(L, 1);
            void *ptr           = heap->tryAlloc((pead::u32) luaL_checkinteger(L, 2), (pead::s32) luaL_optinteger(L, 3, 8));
            if (ptr)
                lua_pushlightuserdata(L, ptr);
            else
                lua_pushnil(L);
            return 1;
        }

        int HeapFree(lua_State *L)
        {
            pead::ExpHeap *heap = CheckHeap(L, 1);
            luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
            void *ptr = lua_touserdata(L, 2);
            if (!heap->isInclude(ptr))
                return luaL_error(L, "the block does not belong to this heap");
            heap->free(ptr);
            return 0;
        }

        int HeapRealloc(lua_State *L)
        {
            pead::ExpHeap *heap = CheckHeap(L, 1);
            luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
            void *ptr = lua_touserdata(L, 2);
            if (!heap->isInclude(ptr))
                return luaL_error(L, "the block does not belong to this heap");
            void *grown = heap->tryRealloc(ptr, (pead::u32) luaL_checkinteger(L, 3), (pead::s32) luaL_optinteger(L, 4, 8));
            if (grown)
                lua_pushlightuserdata(L, grown);
            else
                lua_pushnil(L);
            return 1;
        }

        int HeapIsInclude(lua_State *L)
        {
            pead::ExpHeap *heap = CheckHeap(L, 1);
            return Push(L, lua_islightuserdata(L, 2) && heap->isInclude(lua_touserdata(L, 2)));
        }

        int HeapFreeAll(lua_State *L)
        {
            CheckHeap(L, 1)->freeAll();
            return 0;
        }

        int HeapGetSize(lua_State *L)
        {
            return Push(L, CheckHeap(L, 1)->getSize());
        }

        int HeapGetFreeSize(lua_State *L)
        {
            return Push(L, CheckHeap(L, 1)->getFreeSize());
        }

        int HeapGetMaxAllocatableSize(lua_State *L)
        {
            return Push(L, CheckHeap(L, 1)->getMaxAllocatableSize((pead::s32) luaL_optinteger(L, 2, 4)));
        }

        int HeapGetName(lua_State *L)
        {
            return Push(L, CheckHeap(L, 1)->getName().cstr());
        }

        const luaL_Reg HeapMethods[] = {
                {"Destroy", HeapDestroy},
                {"Alloc", HeapAlloc},
                {"Free", HeapFree},
                {"Realloc", HeapRealloc},
                {"IsInclude", HeapIsInclude},
                {"FreeAll", HeapFreeAll},
                {"GetSize", HeapGetSize},
                {"GetFreeSize", HeapGetFreeSize},
                {"GetMaxAllocatableSize", HeapGetMaxAllocatableSize},
                {"GetName", HeapGetName},
                {nullptr, nullptr},
        };

        int NewRandom(lua_State *L)
        {
            RandomHandle *handle = New<RandomHandle>(L);
            if (!lua_isnoneornil(L, 1))
                handle->random.init((pead::u32) luaL_checkinteger(L, 1));
            return 1;
        }

        int RandomInit(lua_State *L)
        {
            RandomHandle *handle = Check<RandomHandle>(L, 1);
            if (lua_isnoneornil(L, 2))
                handle->random.init();
            else
                handle->random.init((pead::u32) luaL_checkinteger(L, 2));
            return 0;
        }

        int RandomGetU32(lua_State *L)
        {
            RandomHandle *handle = Check<RandomHandle>(L, 1);
            if (lua_isnoneornil(L, 2))
                return Push(L, handle->random.getU32());
            return Push(L, handle->random.getU32((pead::u32) luaL_checkinteger(L, 2)));
        }

        int RandomGetU64(lua_State *L)
        {
            return Push(L, (lua_Integer) Check<RandomHandle>(L, 1)->random.getU64());
        }

        int RandomGetS32Range(lua_State *L)
        {
            RandomHandle *handle = Check<RandomHandle>(L, 1);
            return Push(L, handle->random.getS32Range((pead::s32) luaL_checkinteger(L, 2), (pead::s32) luaL_checkinteger(L, 3)));
        }

        int RandomGetF32(lua_State *L)
        {
            return Push(L, (double) Check<RandomHandle>(L, 1)->random.getF32());
        }

        int RandomGetBool(lua_State *L)
        {
            return Push(L, Check<RandomHandle>(L, 1)->random.getBool());
        }

        const luaL_Reg RandomMethods[] = {
                {"Init", RandomInit},     {"GetU32", RandomGetU32},   {"GetU64", RandomGetU64}, {"GetS32Range", RandomGetS32Range},
                {"GetF32", RandomGetF32}, {"GetBool", RandomGetBool}, {nullptr, nullptr},
        };

        int NewWriteStream(lua_State *L)
        {
            lua_Integer size = luaL_optinteger(L, 1, 0x1000);
            luaL_argcheck(L, size > 0, 1, "the size has to be positive");
            pead::Stream::Modes mode   = OptMode(L, 2);
            pead::Endian::Types endian = OptEndian(L, 3);
            New<WriteStreamHandle>(L, (pead::u32) size, mode)->stream.setBinaryEndian(endian);
            return 1;
        }

        template<typename T, void (pead::WriteStream::*Method)(T)>
        int StreamWriteInteger(lua_State *L)
        {
            (Check<WriteStreamHandle>(L, 1)->stream.*Method)((T) luaL_checkinteger(L, 2));
            return 0;
        }

        template<typename T, void (pead::WriteStream::*Method)(T)>
        int StreamWriteNumber(lua_State *L)
        {
            (Check<WriteStreamHandle>(L, 1)->stream.*Method)((T) luaL_checknumber(L, 2));
            return 0;
        }

        int StreamWriteString(lua_State *L)
        {
            WriteStreamHandle *handle = Check<WriteStreamHandle>(L, 1);
            size_t length;
            const char *s = luaL_checklstring(L, 2, &length);
            handle->stream.writeString(s, (pead::u32) luaL_optinteger(L, 3, (lua_Integer) length));
            return 0;
        }

        int StreamWriteMemBlock(lua_State *L)
        {
            size_t size;
            const char *data = luaL_checklstring(L, 2, &size);
            Check<WriteStreamHandle>(L, 1)->stream.writeMemBlock(data, (pead::u32) size);
            return 0;
        }

        int StreamWriteComment(lua_State *L)
        {
            Check<WriteStreamHandle>(L, 1)->stream.writeComment(luaL_checkstring(L, 2));
            return 0;
        }

        int StreamGetData(lua_State *L)
        {
            WriteStreamHandle *handle = Check<WriteStreamHandle>(L, 1);
            lua_pushlstring(L, handle->buffer.data(), handle->stream.getPosition());
            return 1;
        }

        const luaL_Reg WriteStreamMethods[] = {
                {"WriteU8", StreamWriteInteger<pead::u8, &pead::WriteStream::writeU8>},
                {"WriteU16", StreamWriteInteger<pead::u16, &pead::WriteStream::writeU16>},
                {"WriteU32", StreamWriteInteger<pead::u32, &pead::WriteStream::writeU32>},
                {"WriteU64", StreamWriteInteger<pead::u64, &pead::WriteStream::writeU64>},
                {"WriteS8", StreamWriteInteger<pead::s8, &pead::WriteStream::writeS8>},
                {"WriteS16", StreamWriteInteger<pead::s16, &pead::WriteStream::writeS16>},
                {"WriteS32", StreamWriteInteger<pead::s32, &pead::WriteStream::writeS32>},
                {"WriteS64", StreamWriteInteger<pead::s64, &pead::WriteStream::writeS64>},
                {"WriteF32", StreamWriteNumber<pead::f32, &pead::WriteStream::writeF32>},
                {"WriteF64", StreamWriteNumber<pead::f64, &pead::WriteStream::writeF64>},
                {"WriteString", StreamWriteString},
                {"WriteMemBlock", StreamWriteMemBlock},
                {"WriteComment", StreamWriteComment},
                {"GetData", StreamGetData},
                {nullptr, nullptr},
        };

        int NewReadStream(lua_State *L)
        {
            size_t size;
            const char *data           = luaL_checklstring(L, 1, &size);
            pead::Stream::Modes mode   = OptMode(L, 2);
            pead::Endian::Types endian = OptEndian(L, 3);
            New<ReadStreamHandle>(L, data, size, mode)->stream.setBinaryEndian(endian);
            return 1;
        }

        template<typename T, T (pead::ReadStream::*Method)()>
        int StreamReadInteger(lua_State *L)
        {
            return Push(L, (lua_Integer) (Check<ReadStreamHandle>(L, 1)->stream.*Method)());
        }

        template<typename T, T (pead::ReadStream::*Method)()>
        int StreamReadNumber(lua_State *L)
        {
            return Push(L, (double) (Check<ReadStreamHandle>(L, 1)->stream.*Method)());
        }

        int StreamReadString(lua_State *L)
        {
            ReadStreamHandle *handle = Check<ReadStreamHandle>(L, 1);
            lua_Integer size         = luaL_optinteger(L, 2, 0);
            std::vector<char> buffer((size_t) (size > 255 ? size + 1 : 256));
            pead::BufferedSafeString str(buffer.data(), (pead::s32) buffer.size());
            handle->stream.readString(&str, (pead::u32) size);
            return Push(L, str.cstr());
        }

        int StreamReadMemBlock(lua_State *L)
        {
            lua_Integer size = luaL_checkinteger(L, 2);
            std::string out((size_t) size, '\0');
            pead::u32 got = Check<ReadStreamHandle>(L, 1)->stream.readMemBlock(out.data(), (pead::u32) size);
            lua_pushlstring(L, out.data(), got);
            return 1;
        }

        const luaL_Reg ReadStreamMethods[] = {
                {"ReadU8", StreamReadInteger<pead::u8, &pead::ReadStream::readU8>},
                {"ReadU16", StreamReadInteger<pead::u16, &pead::ReadStream::readU16>},
                {"ReadU32", StreamReadInteger<pead::u32, &pead::ReadStream::readU32>},
                {"ReadU64", StreamReadInteger<pead::u64, &pead::ReadStream::readU64>},
                {"ReadS8", StreamReadInteger<pead::s8, &pead::ReadStream::readS8>},
                {"ReadS16", StreamReadInteger<pead::s16, &pead::ReadStream::readS16>},
                {"ReadS32", StreamReadInteger<pead::s32, &pead::ReadStream::readS32>},
                {"ReadS64", StreamReadInteger<pead::s64, &pead::ReadStream::readS64>},
                {"ReadF32", StreamReadNumber<pead::f32, &pead::ReadStream::readF32>},
                {"ReadF64", StreamReadNumber<pead::f64, &pead::ReadStream::readF64>},
                {"ReadString", StreamReadString},
                {"ReadMemBlock", StreamReadMemBlock},
                {nullptr, nullptr},
        };

        std::u16string ToUtf16(const char *src, size_t size)
        {
            std::u16string out(size + 1, u'\0');
            pead::s32 length = pead::StringUtil::convertUtf8ToUtf16(out.data(), (pead::u32) out.size(), src, (pead::s32) size);
            out.resize((size_t) length);
            return out;
        }

        int PushUtf8(lua_State *L, const std::u16string &src)
        {
            std::string out(src.size() * 3 + 1, '\0');
            pead::s32 length = pead::StringUtil::convertUtf16ToUtf8(out.data(), (pead::u32) out.size(), src.data(), (pead::s32) src.size());
            lua_pushlstring(L, out.data(), (size_t) length);
            return 1;
        }

        template<char16_t (*Map)(char16_t)>
        int MapCase(lua_State *L)
        {
            size_t size;
            const char *s       = luaL_checklstring(L, 1, &size);
            std::u16string text = ToUtf16(s, size);
            for (char16_t &c : text) c = Map(c);
            return PushUtf8(L, text);
        }

        int Utf8ToSjis(lua_State *L)
        {
            size_t size;
            const char *s = luaL_checklstring(L, 1, &size);
            std::string out(size * 2 + 1, '\0');
            pead::s32 length = pead::StringUtil::convertUtf8ToSjis(out.data(), (pead::u32) out.size(), s, (pead::s32) size);
            lua_pushlstring(L, out.data(), (size_t) length);
            return 1;
        }

        int SjisToUtf8(lua_State *L)
        {
            size_t size;
            const char *s = luaL_checklstring(L, 1, &size);
            std::u16string text(size + 1, u'\0');
            pead::s32 length = pead::StringUtil::convertSjisToUtf16(text.data(), (pead::u32) text.size(), s, (pead::s32) size);
            text.resize((size_t) length);
            return PushUtf8(L, text);
        }

        int Base(lua_State *L, int index)
        {
            lua_Integer base = luaL_optinteger(L, index, -1);
            luaL_argcheck(L, base == -1 || base == 10 || base == 16, index, "the base has to be 10 or 16");
            return (int) base;
        }

        int TryParseS64(lua_State *L)
        {
            pead::s64 value = 0;
            if (!pead::StringUtil::tryParseS64(&value, luaL_checkstring(L, 1), (pead::StringUtil::CardinalNumber) Base(L, 2)))
            {
                lua_pushnil(L);
                return 1;
            }
            return Push(L, (lua_Integer) value);
        }

        int TryParseU64(lua_State *L)
        {
            pead::u64 value = 0;
            if (!pead::StringUtil::tryParseU64(&value, luaL_checkstring(L, 1), (pead::StringUtil::CardinalNumber) Base(L, 2)))
            {
                lua_pushnil(L);
                return 1;
            }
            return Push(L, (lua_Integer) value);
        }

        int TryParseF64(lua_State *L)
        {
            pead::f64 value = 0;
            if (!pead::StringUtil::tryParseF64(&value, luaL_checkstring(L, 1)))
            {
                lua_pushnil(L);
                return 1;
            }
            return Push(L, (double) value);
        }

        const luaL_Reg StringUtilFunctions[] = {
                {"ToLower", MapCase<pead::StringUtil::toLowerCapital>},
                {"ToUpper", MapCase<pead::StringUtil::toUpperCapital>},
                {"Utf8ToSjis", Utf8ToSjis},
                {"SjisToUtf8", SjisToUtf8},
                {"TryParseS64", TryParseS64},
                {"TryParseU64", TryParseU64},
                {"TryParseF64", TryParseF64},
                {nullptr, nullptr},
        };

        int Base64Encode(lua_State *L)
        {
            size_t size;
            const char *src = luaL_checklstring(L, 1, &size);
            std::string out(pead::Base64::calcEncodedSize((pead::u32) size) + 1, '\0');
            pead::Base64::encode(out.data(), src, (pead::u32) size, lua_toboolean(L, 2));
            return Push(L, out.c_str());
        }

        int Base64Decode(lua_State *L)
        {
            size_t size;
            const char *src = luaL_checklstring(L, 1, &size);
            std::string out(size, '\0');
            pead::u32 decoded = 0;
            if (!pead::Base64::decode(out.data(), (pead::u32) out.size(), src, (pead::u32) size, &decoded))
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushlstring(L, out.data(), decoded);
            return 1;
        }

        const luaL_Reg Base64Functions[] = {
                {"Encode", Base64Encode},
                {"Decode", Base64Decode},
                {nullptr, nullptr},
        };

        const luaL_Reg Functions[] = {
                {"Initialize", Initialize},        {"Finalize", Finalize}, {"IsInitialized", IsInitialized},
                {"GetRootFreeSize", RootFreeSize}, {nullptr, nullptr},
        };
    } // namespace

    void OpenPead(lua_State *L)
    {
        DefineClass(L, ClassName<HeapHandle>::value, nullptr, HeapMethods, nullptr, HeapGc);
        Define<RandomHandle>(L, nullptr, RandomMethods);
        Define<WriteStreamHandle>(L, nullptr, WriteStreamMethods);
        Define<ReadStreamHandle>(L, nullptr, ReadStreamMethods);

        lua_newtable(L);
        SetClass(L, "ExpHeap", NewExpHeap);
        SetClass(L, "Random", NewRandom);
        SetClass(L, "WriteStream", NewWriteStream);
        SetClass(L, "ReadStream", NewReadStream);
        SetFunctions(L, "StringUtil", StringUtilFunctions);
        SetFunctions(L, "Base64", Base64Functions);
        luaL_setfuncs(L, Functions, 0);
        lua_setfield(L, -2, "pead");
    }
} // namespace nn::nex::lua
