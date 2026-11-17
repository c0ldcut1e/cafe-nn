#include <cstdio>
#include <cstring>
#include <nn/nex/platform/InetAddress.h>
#include <nn/nex/runtime/Job.h>
#include <nn/nex/runtime/Scheduler.h>
#include <nn/pia/common/ModuleStackChecker.h>
#include <nn/pia/common/Scheduler.h>
#include <nn/pia/common/StationAddress.h>
#include <nn/pia/common/StepSequenceJob.h>

using namespace nn::pia;
using namespace nn::pia::common;

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

static void TestResult()
{
    CHECK(ResultSuccess().GetErrorCode() == 0);
    CHECK(ResultInvalidArgument().GetErrorCode() == 0x120167);
    CHECK(Result(0xac6d).GetErrorCode() == 0x12054b);
    CHECK(Result(0x10c14).GetErrorCode() == 0x1201c5);
    CHECK(Result(0x12345).GetErrorCode() == Result::UnknownErrorCode);
    CHECK(Result(Result::NexErrorValue).GetErrorCode() == Result::UnknownErrorCode);
    Result::SetNexNetworkErrorCode(0x10101);
    CHECK(Result(Result::NexErrorValue).GetErrorCode() == 0x10101);
    Result::SetNexNetworkErrorCode(0);
}

static void TestModuleStackChecker()
{
    using M = ModuleStackChecker;
    CHECK(M::Register((M::ModuleType) 1).GetValue() == Result::InvalidArgumentValue);
    CHECK(M::Register((M::ModuleType) 15).GetValue() == Result::InvalidStateValue);
    CHECK(M::Register(M::ModuleType_Common).IsSuccess());
    CHECK(M::Register((M::ModuleType) 4).IsSuccess());
    CHECK(M::Register((M::ModuleType) 15).GetValue() == Result::InvalidStateValue);
    CHECK(M::Register((M::ModuleType) 7).IsSuccess());
    CHECK(M::Unregister((M::ModuleType) 4).GetValue() == Result::InvalidStateValue);
    CHECK(M::Unregister((M::ModuleType) 7).IsSuccess());
    CHECK(M::Unregister((M::ModuleType) 4).IsSuccess());
    CHECK(M::Unregister(M::ModuleType_Common).IsSuccess());
    CHECK(M::GetCount() == 0);
    CHECK(M::Unregister(M::ModuleType_Common).GetValue() == Result::InvalidStateValue);
}

static void TestAddress()
{
    InetAddress address(0xc0a80102, 0x1234);
    u8 buffer[6];
    CHECK(address.Serialize(buffer, 5).GetValue() == Result::InvalidArgumentValue);
    CHECK(address.Serialize(nullptr, 6).GetValue() == Result::InvalidArgumentValue);
    CHECK(address.Serialize(buffer, sizeof(buffer)).IsSuccess());
    static const u8 expected[6] = {0xc0, 0xa8, 0x01, 0x02, 0x12, 0x34};
    CHECK(memcmp(buffer, expected, 6) == 0);
    InetAddress back;
    CHECK(back.Deserialize(buffer, 6).IsSuccess() && back == address);
    CHECK(address.IsPrivate());
    CHECK(InetAddress(0xac1f0001, 1).IsPrivate());
    CHECK(!InetAddress(0xac200001, 1).IsPrivate());
    CHECK(!InetAddress(0x08080808, 53).IsPrivate());
    char text[22];
    address.ToString(text, sizeof(text));
    CHECK(strcmp(text, "192.168.1.2:4660") == 0);

    StationAddress low(InetAddress(0x01000000, 1)), high(InetAddress(0x80000000, 1));
    CHECK(high < low);
    CHECK(low.Compare(low) == 0);
}

namespace
{
    class CountJob : public Job
    {
    public:
        u32 runs   = 0;
        u32 stopAt = 3;
        ExecuteResult ExecuteCore() override
        {
            return ++runs >= stopAt ? Completed() : WaitNextDispatch();
        }
    };

    class StepJob : public StepSequenceJob
    {
    public:
        u32 trace    = 0;
        bool cleaned = false;
        StepJob()
        {
            SetStep(&StepJob::First);
        }
        ExecuteResult First()
        {
            trace = trace * 10 + 1;
            SetStep(&StepJob::Second);
            return Continue();
        }
        ExecuteResult Second()
        {
            trace = trace * 10 + 2;
            SetStep(&StepJob::Third);
            return Suspend();
        }
        ExecuteResult Third()
        {
            trace = trace * 10 + 3;
            return Completed();
        }
        void CancelCleanup() override
        {
            cleaned = true;
        }
    };
} // namespace

static void TestScheduler()
{
    CountJob unscheduled;
    CHECK(unscheduled.Ready().GetValue() == Result::InvalidStateValue);

    CHECK(Scheduler::CreateInstance().IsSuccess());
    CHECK(Scheduler::CreateInstance().GetValue() == Result::InvalidStateValue);
    Scheduler *scheduler = Scheduler::GetInstance();

    CountJob job;
    CHECK(job.Ready().IsSuccess());
    CHECK(job.GetState() == Job::State_Ready);
    CHECK(job.Ready().GetValue() == Result::InvalidStateValue);
    scheduler->Dispatch();
    CHECK(job.runs == 1 && job.GetState() == Job::State_Waiting);
    scheduler->Dispatch();
    scheduler->Dispatch();
    CHECK(job.runs == 3 && job.GetState() == Job::State_Completed && job.GetExecuteCount() == 3);
    scheduler->Dispatch();
    CHECK(job.runs == 3);
    job.Reset();
    CHECK(job.GetState() == Job::State_None && job.GetExecuteCount() == 0);

    StepJob steps;
    CHECK(steps.Ready().IsSuccess());
    scheduler->Dispatch();
    CHECK(steps.trace == 12 && steps.GetState() == Job::State_Suspended);
    CHECK(steps.Resume().IsSuccess());
    scheduler->Dispatch();
    CHECK(steps.trace == 123 && steps.GetState() == Job::State_Completed);

    StepJob cancelled;
    cancelled.Cancel();
    CHECK(cancelled.Ready().IsSuccess());
    scheduler->Dispatch();
    CHECK(cancelled.cleaned && cancelled.trace == 0 && !cancelled.IsCancelRequested());

    CountJob dropped;
    CHECK(dropped.Ready().IsSuccess());
    dropped.Reset();
    scheduler->Dispatch();
    CHECK(dropped.runs == 0);

    CHECK(BackgroundScheduler::CreateInstance().IsSuccess());
    CountJob background;
    background.stopAt = 1;
    CHECK(background.Ready(true).IsSuccess());
    scheduler->Dispatch();
    CHECK(background.runs == 0);
    BackgroundScheduler::GetInstance()->Dispatch();
    CHECK(background.runs == 1 && background.GetState() == Job::State_Completed);
    BackgroundScheduler::DestroyInstance();
    Scheduler::DestroyInstance();
}

int main()
{
    TestResult();
    TestModuleStackChecker();
    TestAddress();
    TestScheduler();
    if (g_failures)
    {
        printf("%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("pia tests passed\n");
    return 0;
}
