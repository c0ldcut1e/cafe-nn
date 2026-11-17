#include <nn/nex/ByteStream.h>
#include <nn/nex/Crypto.h>
#include <nn/nex/Kerberos.h>

namespace nn::nex
{

    Buffer KerberosEncryption::Encrypt(const Buffer &plain) const
    {
        Buffer out = plain;
        RC4 cipher(m_key.GetContentPtr(), m_key.GetContentSize());
        cipher.Process(out.GetContentPtr(), out.GetContentSize());
        qUnsignedInt8 mac[16];
        HmacMD5(m_key.GetContentPtr(), m_key.GetContentSize(), out.GetContentPtr(), out.GetContentSize(), mac);
        out.Append(mac, 16);
        return out;
    }

    bool KerberosEncryption::Decrypt(const Buffer &sealed, Buffer *plain) const
    {
        if (sealed.GetContentSize() < 16)
            return false;
        size_t size = sealed.GetContentSize() - 16;
        qUnsignedInt8 mac[16];
        HmacMD5(m_key.GetContentPtr(), m_key.GetContentSize(), sealed.GetContentPtr(), size, mac);
        if (memcmp(mac, sealed.GetContentPtr() + size, 16) != 0)
            return false;
        *plain = Buffer(sealed.GetContentPtr(), size);
        RC4 cipher(m_key.GetContentPtr(), m_key.GetContentSize());
        cipher.Process(plain->GetContentPtr(), size);
        return true;
    }

    Buffer KerberosEncryption::DeriveKey(qUnsignedInt32 principalId, const String &password)
    {
        std::string utf8 = password.ToUtf8();
        qUnsignedInt8 key[16];
        MD5::Hash(utf8.data(), utf8.size(), key);
        qUnsignedInt32 rounds = 65000 + principalId % 1024;
        for (qUnsignedInt32 i = 1; i < rounds; i++) MD5::Hash(key, 16, key);
        return Buffer(key, 16);
    }

    bool Ticket::Parse(const Buffer &decrypted)
    {
        // The session key is always 32 bytes, and the target principal and internal ticket follow it.
        ByteStream s(decrypted.GetContentPtr(), decrypted.GetContentSize());
        qUnsignedInt8 key[32];
        s.Read(key, sizeof(key));
        qUnsignedInt32 target = s.ReadU32();
        Buffer ticket         = s.ReadBuffer();
        if (!s.IsValid())
            return false;
        sessionKey        = Buffer(key, sizeof(key));
        targetPrincipalId = target;
        internalTicket    = ticket;
        return true;
    }

} // namespace nn::nex
