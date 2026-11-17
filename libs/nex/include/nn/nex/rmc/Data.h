/**
 * @file
 * @brief This is the base of every class a holder can carry, and the type name is how the receiver picks the class.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <nn/nex/core/String.h>
#include <nn/nex/rmc/Message.h>

namespace nn::nex
{

    /**
     * @brief This is the base of every class a holder can carry, and the type name is how the receiver picks the class.
     */
    class Data
    {
    public:
        Data()          = default;
        virtual ~Data() = default;
        virtual const char *GetDataTypeName() const
        {
            return "Data";
        }
        virtual Data *Clone() const
        {
            return new Data(*this);
        }
        virtual void StreamOut(Message &m) const
        {
            (void) m;
        }
        virtual bool StreamIn(Message &m)
        {
            return m.IsValid();
        }
    };

    /**
     * @brief We look classes up by name here because there is no RTTI on the console build.
     */
    class DataFactory
    {
    public:
        using Creator = Data *(*)();
        static void Register(const char *name, Creator creator);
        static Data *Create(const String &name);
    };

    template<typename T>
    struct DataFactoryRegistration
    {
        explicit DataFactoryRegistration(const char *name)
        {
            DataFactory::Register(name,
                                  []() -> Data *
                                  {
                                      return new T();
                                  });
        }
    };

} // namespace nn::nex
