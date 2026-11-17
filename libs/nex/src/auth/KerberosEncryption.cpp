#include <nn/nex/auth/KerberosEncryption.h>
#include <nn/nex/core/Buffer.h>
#include <nn/nex/core/String.h>
#include <nn/nex/crypto/MD5.h>
#include <nn/nex/crypto/RC4.h>
#include <nn/nex/types.h>

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

} // namespace nn::nex
