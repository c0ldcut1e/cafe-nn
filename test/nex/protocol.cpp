#include <cstdio>
#include <nn/nex.h>

using namespace nn::nex;

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

static Message Rewind(const Message &m)
{
    return Message(m.GetData(), m.GetSize());
}

int RunProtocolTests()
{
    Buffer key = KerberosEncryption::DeriveKey(1750087940, String("password"));
    CHECK(key.GetContentSize() == 16);
    const char text[] = "ticket body";
    Buffer sealed     = KerberosEncryption(key).Encrypt(Buffer(text, sizeof text));
    Buffer opened;
    CHECK(KerberosEncryption(key).Decrypt(sealed, &opened));
    CHECK(opened.GetContentSize() == sizeof text && memcmp(opened.GetContentPtr(), text, sizeof text) == 0);
    sealed.GetContentPtr()[0] ^= 1;
    CHECK(!KerberosEncryption(key).Decrypt(sealed, &opened));

    MatchmakeSession session;
    session.SetID(77);
    session.SetMaxParticipants(8);
    session.SetGameMode(3);
    session.SetAttribute(2, 42);
    session.SetDescription(String("lobby"));
    MatchmakeParam param;
    param.params[String("SR")]  = Variant(true);
    param.params[String("GIR")] = Variant((uint32_t) 5);
    session.SetMatchmakeParam(param);
    Message out;
    AnyObjectHolder<Gathering, String>(session).Add(out);
    Message in = Rewind(out);
    AnyObjectHolder<Gathering, String> holder;
    CHECK(holder.Extract(in));
    CHECK(holder.IsValid() && String(holder->GetDataTypeName()) == String("MatchmakeSession"));
    if (holder.IsValid())
    {
        const MatchmakeSession *back = (const MatchmakeSession *) holder.Get();
        CHECK(back->GetID() == 77 && back->GetMaxParticipants() == 8 && back->GetGameMode() == 3);
        CHECK(back->GetAttribute(2) == 42 && back->GetDescription() == String("lobby"));
    }
    CHECK(in.IsValid() && in.GetCursor() == in.GetSize());

    PersistentGathering community;
    community.SetPersistentGatheringType(PersistentGatheringType::PasswordLocked);
    community.SetAttribute(0, 9);
    Message pgOut;
    community.StreamOut(pgOut);
    Message pgIn = Rewind(pgOut);
    PersistentGathering pgBack;
    CHECK(pgBack.StreamIn(pgIn));
    CHECK(pgBack.GetPersistentGatheringType() == PersistentGatheringType::PasswordLocked && pgBack.GetAttribute(0) == 9);

    NotificationEvent event;
    event.SetSource(5);
    event.SetType(3001);
    event.SetParam1(1234);
    event.SetStringParam(String("hi"));
    Message evOut;
    event.StreamOut(evOut);
    Message evIn = Rewind(evOut);
    NotificationEvent evBack;
    CHECK(evBack.StreamIn(evIn));
    CHECK(evBack.GetMainType() == 3 && evBack.GetSubType() == 1 && evBack.GetParam1() == 1234);
    CHECK(evBack.GetStringParam() == String("hi"));

    CHECK(MatchmakeExtensionClient::PersistentGatheringCodeToGatheringId(MatchmakeExtensionClient::GatheringIdToPersistentGatheringCode(123456)) ==
          123456);

    MatchmakeSessionSearchCriteria criteria;
    criteria.SetAttributeWithRange(1, 2, 5);
    criteria.SetGameMode(1);
    Message crit;
    criteria.StreamOut(crit);
    CHECK(crit.GetSize() > 5);

    return g_failures;
}
