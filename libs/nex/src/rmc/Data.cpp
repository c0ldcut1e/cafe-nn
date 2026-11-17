#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Data.h>

namespace nn::nex
{

    namespace
    {

        std::map<std::string, DataFactory::Creator> &Creators()
        {
            static std::map<std::string, DataFactory::Creator> creators;
            return creators;
        }

    } // namespace

    void DataFactory::Register(const char *name, Creator creator)
    {
        Creators()[name] = creator;
    }

    Data *DataFactory::Create(const String &name)
    {
        auto it = Creators().find(name.ToUtf8());
        return it == Creators().end() ? nullptr : it->second();
    }

} // namespace nn::nex
