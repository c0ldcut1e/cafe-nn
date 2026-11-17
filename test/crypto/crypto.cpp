#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <nn/crypto.h>
#include <string>
#include <vector>

using namespace nn::crypto;

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

static std::string Hex(const void *data, size_t size)
{
    const qUnsignedInt8 *p = static_cast<const qUnsignedInt8 *>(data);
    std::string s;
    char b[3];
    for (size_t i = 0; i < size; i++)
    {
        snprintf(b, sizeof b, "%02x", p[i]);
        s += b;
    }
    return s;
}

static std::vector<qUnsignedInt8> Bytes(const char *hex)
{
    std::vector<qUnsignedInt8> out;
    for (size_t i = 0; hex[i] && hex[i + 1]; i += 2)
    {
        char pair[3] = {hex[i], hex[i + 1], 0};
        out.push_back((qUnsignedInt8) strtoul(pair, nullptr, 16));
    }
    return out;
}

static std::vector<qUnsignedInt8> Range(qUnsignedInt8 first, size_t count)
{
    std::vector<qUnsignedInt8> out(count);
    for (size_t i = 0; i < count; i++) out[i] = (qUnsignedInt8) (first + i);
    return out;
}

static std::string Sha1(const std::string &s)
{
    qUnsignedInt8 d[20];
    CalculateSha1(d, s.data(), s.size());
    return Hex(d, 20);
}

static std::string Md5(const std::string &s)
{
    qUnsignedInt8 d[16];
    CalculateMd5(d, s.data(), s.size());
    return Hex(d, 16);
}

static void TestHashes()
{
    CHECK(Sha1("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    CHECK(Sha1("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK(Sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    CHECK(Sha1(std::string(1000000, 'a')) == "34aa973cd4c4daa4f61eeb2bdbad27316534016f");

    CHECK(Md5("") == "d41d8cd98f00b204e9800998ecf8427e");
    CHECK(Md5("abc") == "900150983cd24fb0d6963f7d28e17f72");
    CHECK(Md5("12345678901234567890123456789012345678901234567890123456789012345678901234567890") == "57edf4a22be3c955ac49da2e2107b67a");

    // Feeding the input in odd pieces has to give the same digest as one call
    std::string text(200, 'x');
    for (size_t i = 0; i < text.size(); i++) text[i] = (char) i;
    Sha1Context sha;
    Md5Context md5;
    for (size_t at = 0, step = 1; at < text.size(); at += step, step = step * 2 + 1)
    {
        size_t n = step < text.size() - at ? step : text.size() - at;
        sha.Update(text.data() + at, n);
        md5.Update(text.data() + at, n);
    }
    qUnsignedInt8 d[20];
    sha.GetHash(d);
    CHECK(Hex(d, 20) == Sha1(text));
    CHECK(sha.GetHashSize() == 20);
    md5.GetHash(d);
    CHECK(Hex(d, 16) == Md5(text));
    CHECK(md5.GetHashLength() == 200);

    // Resuming from the starting state at length 0 is the same as a fresh hash
    const qUnsignedInt32 initial[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    Sha1Context resumed;
    resumed.InitializeWithContext(initial, 0);
    resumed.Update("abc", 3);
    resumed.GetHash(d);
    CHECK(Hex(d, 20) == "a9993e364706816aba3e25717850c26c9cd0d89d");

    HashContextBase &base = sha;
    base.Initialize();
    base.Update("abc", 3);
    base.GetHash(d);
    CHECK(Hex(d, 20) == "a9993e364706816aba3e25717850c26c9cd0d89d");
}

template<size_t KeySize>
static void CheckAes(const char *expected)
{
    auto key   = Range(0, KeySize);
    auto plain = Bytes("00112233445566778899aabbccddeeff");
    Aes<KeySize> aes;
    CHECK(!aes.SetKey(key.data(), KeySize - 1));
    CHECK(aes.SetKey(key.data(), KeySize));
    qUnsignedInt8 block[16];
    aes.Encrypt(block, plain.data());
    CHECK(Hex(block, 16) == expected);
    aes.Decrypt(block, block);
    CHECK(Hex(block, 16) == "00112233445566778899aabbccddeeff");
}

static void TestAes()
{
    CheckAes<16>("69c4e0d86a7b0430d8cdb78070b4c55a");
    CheckAes<24>("dda97ca4864cdfe06eaf70a0ec0d7191");
    CheckAes<32>("8ea2b7ca516745bfeafc49904b496089");
}

static void TestCtr()
{
    auto key    = Bytes("2b7e151628aed2a6abf7158809cf4f3c");
    auto iv     = Bytes("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    auto plain  = Bytes("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51");
    auto cipher = "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff";

    Aes128 aes;
    aes.SetKey(key.data(), key.size());
    std::vector<qUnsignedInt8> out(plain.size());
    CtrEncryptor<16> encryptor;
    CHECK(encryptor.Encrypt(out.data(), out.size(), plain.data(), plain.size(), iv.data(), iv.size(), &aes) == plain.size());
    CHECK(Hex(out.data(), out.size()) == cipher);

    // Splitting mid block keeps the key stream position
    CtrDecryptor<16> decryptor;
    CHECK(decryptor.Initialize(&aes, iv.data(), iv.size()));
    std::vector<qUnsignedInt8> back(out.size());
    CHECK(decryptor.Update(back.data(), 5, out.data(), 5) == 5);
    CHECK(decryptor.Update(back.data() + 5, back.size() - 5, out.data() + 5, out.size() - 5) == out.size() - 5);
    CHECK(back == plain);
    CHECK(decryptor.Update(back.data(), 1, out.data(), 2) == 0);
    CHECK(!decryptor.Initialize(&aes, iv.data(), 8));

    // The counter carries across the whole 16 bytes
    qUnsignedInt8 counter[16];
    memset(counter, 0xff, 16);
    counter[0] = 0x01;
    detail::CtrMode<16>::IncrementCounter(counter);
    CHECK(Hex(counter, 16) == "02000000000000000000000000000000");
}

static void CheckCcm(const std::vector<qUnsignedInt8> &nonce, const std::vector<qUnsignedInt8> &adata, const std::vector<qUnsignedInt8> &plain,
                     size_t macSize, const char *expected)
{
    auto key = Range(0x40, 16);
    Aes128 aes;
    aes.SetKey(key.data(), key.size());

    std::vector<qUnsignedInt8> out(plain.size()), mac(macSize);
    CcmEncryptor encryptor;
    CHECK(encryptor.Initialize(&aes, nonce.data(), nonce.size(), adata.size(), plain.size(), macSize));
    CHECK(encryptor.EncryptAndGenerate(out.data(), mac.data(), out.size(), plain.data(), plain.size(), adata.data(), adata.size(), macSize));
    CHECK(Hex(out.data(), out.size()) + Hex(mac.data(), mac.size()) == expected);

    std::vector<qUnsignedInt8> back(out.size());
    CcmDecryptor decryptor;
    CHECK(decryptor.Initialize(&aes, nonce.data(), nonce.size(), adata.size(), out.size(), macSize));
    CHECK(decryptor.DecryptAndVerify(back.data(), back.size(), out.data(), out.size(), adata.data(), adata.size(), mac.data(), macSize));
    CHECK(back == plain);

    mac[0] ^= 1;
    CHECK(decryptor.Initialize(&aes, nonce.data(), nonce.size(), adata.size(), out.size(), macSize));
    CHECK(!decryptor.DecryptAndVerify(back.data(), back.size(), out.data(), out.size(), adata.data(), adata.size(), mac.data(), macSize));
    CHECK(back == std::vector<qUnsignedInt8>(back.size(), 0));
}

static void TestCcm()
{
    CheckCcm(Range(0x10, 7), Range(0, 8), Range(0x20, 4), 4, "7162015b4dac255d");
    CheckCcm(Range(0x10, 8), Range(0, 16), Range(0x20, 16), 6, "d2a1f0e051ea5f62081a7792073d593d1fc64fbfaccd");

    // Associated data past 0xff00 bytes takes the six byte length header
    CheckCcm(Range(0x10, 13), std::vector<qUnsignedInt8>(70000, 0), Range(0x20, 37), 16,
             "69915dad1e84c6376a68c2967e4dab615ae0fd1faec44cc484828529463ccf7232ec7cb9e0a372a94b73c9658dda79c303425f8663");

    Aes128 aes;
    auto key   = Range(0x40, 16);
    auto nonce = Range(0x10, 13);
    aes.SetKey(key.data(), key.size());
    CcmEncryptor ccm;
    CHECK(!ccm.Initialize(&aes, nonce.data(), 6, 0, 0, 16));
    CHECK(!ccm.Initialize(&aes, nonce.data(), 13, 0, 0, 5));
    CHECK(!ccm.Initialize(&aes, nonce.data(), 13, 0, 0x10000, 16));
    CHECK(ccm.GetIvSize() == 0 && ccm.GetUnitSize() == 16);

    // Fewer bytes than Initialize promised can't produce a tag
    qUnsignedInt8 data[8] = {}, mac[16];
    CHECK(ccm.Initialize(&aes, nonce.data(), 13, 0, sizeof data, 16));
    CHECK(ccm.UpdatePdata(data, sizeof data, data, 4) == 4);
    CHECK(!ccm.GenerateMac(mac, sizeof mac));
}

static void TestRandom()
{
    qUnsignedInt8 a[64] = {}, b[64] = {};
    GenerateRandomBytes(a, sizeof a);
    GenerateRandomBytes(b, sizeof b);
    CHECK(memcmp(a, b, sizeof a) != 0);
    qUnsignedInt8 zero[64] = {};
    CHECK(memcmp(a, zero, sizeof a) != 0);
    GenerateRandomBytes(nullptr, 0);
}

int RunCryptoTests()
{
    TestHashes();
    TestAes();
    TestCtr();
    TestCcm();
    TestRandom();
    return g_failures;
}

int main()
{
    int failures = RunCryptoTests();
    printf(failures ? "%d failures\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
