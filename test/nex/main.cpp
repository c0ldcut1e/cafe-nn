#include <cstdio>
#include <cstdlib>
#include <nn/nex/ByteStream.h>
#include <nn/nex/Crypto.h>
#include <nn/nex/PRUDPMessageV1.h>
#include <nn/nex/StationURL.h>

using namespace nn::nex;

static int g_failures;

#define CHECK(cond)                                                                                                                                  \
    do                                                                                                                                               \
    {                                                                                                                                                \
        if (!(cond))                                                                                                                                 \
        {                                                                                                                                            \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);                                                                                    \
            g_failures++;                                                                                                                            \
        }                                                                                                                                            \
    } while (0)

static std::string Hex(const uint8_t *p, size_t n)
{
    std::string s;
    char b[3];
    for (size_t i = 0; i < n; i++)
    {
        snprintf(b, sizeof b, "%02x", p[i]);
        s += b;
    }
    return s;
}

int RunProtocolTests();

int main()
{
    uint8_t d[16];
    MD5::Hash("", 0, d);
    CHECK(Hex(d, 16) == "d41d8cd98f00b204e9800998ecf8427e");
    MD5::Hash("The quick brown fox jumps over the lazy dog", 43, d);
    CHECK(Hex(d, 16) == "9e107d9d372bb6826bd81d3542a419d6");
    std::string big(1000, 'a');
    MD5::Hash(big.data(), big.size(), d);
    CHECK(Hex(d, 16) == "cabe45dcc9ae5b66ba86600cca6b8ba8");
    HmacMD5("key", 3, "The quick brown fox jumps over the lazy dog", 43, d);
    CHECK(Hex(d, 16) == "80070713463e7749b90c2dc24911e275");

    uint8_t rc4data[] = {'P', 'l', 'a', 'i', 'n', 't', 'e', 'x', 't'};
    RC4("Key", 3).Process(rc4data, sizeof rc4data);
    CHECK(Hex(rc4data, sizeof rc4data) == "bbf316e8d940af0ad3");

    ByteStream out;
    out.WriteU32(0x12345678);
    out.WriteString(String(L"hé"));
    out.WriteDateTime(DateTime(2026, 11, 17, 12, 30, 5));
    ByteStream in(out.GetData(), out.GetSize());
    CHECK(in.ReadU32() == 0x12345678);
    CHECK(in.ReadString() == String(L"hé"));
    DateTime dt = in.ReadDateTime();
    CHECK(dt.GetYear() == 2026 && dt.GetMonth() == 11 && dt.GetDay() == 17 && dt.GetSecond() == 5);
    in.ReadU32();
    CHECK(!in.IsValid());

    StationURL url(String("prudps:/address=10.0.0.5;port=60001;PID=2;type=3"));
    CHECK(url.GetPortNumber() == 60001 && url.GetPrincipalID() == 2 && url.IsBehindNAT() && url.IsPublic());
    CHECK(url.GetAddress() == String("10.0.0.5"));

    PRUDPSignatureContext ctx;
    ctx.SetAccessKey("f1b61c8e");
    PRUDPPacket p;
    p.sourcePort         = 0x3F;
    p.destinationPort    = 0x31;
    p.type               = PacketType::Connect;
    p.flags              = PacketFlag::Reliable | PacketFlag::NeedAck;
    p.minorVersion       = 2;
    p.supportedFunctions = 4;
    p.payload            = Buffer("abc", 3);
    Buffer wire          = PRUDPMessageV1::Encode(p, ctx);
    PRUDPPacket q;
    size_t off = 0;
    CHECK(PRUDPMessageV1::Decode(wire.GetContentPtr(), wire.GetContentSize(), off, q));
    CHECK(off == wire.GetContentSize());
    CHECK(q.type == PacketType::Connect && q.flags == p.flags && q.minorVersion == 2 && q.supportedFunctions == 4);
    CHECK(q.payload.GetContentSize() == 3 && PRUDPMessageV1::VerifySignature(q, ctx));
    q.payload.GetContentPtr()[0] ^= 1;
    CHECK(!PRUDPMessageV1::VerifySignature(q, ctx));

    g_failures += RunProtocolTests();

    printf(g_failures ? "%d failures\n" : "all passed\n", g_failures);
    return g_failures ? 1 : 0;
}
