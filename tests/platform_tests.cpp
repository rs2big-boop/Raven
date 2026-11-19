#include <filesystem>
#include <gtest/gtest.h>

#include "raven/platform/filesystem.hpp"
#include "raven/platform/crypto.hpp"
#include "raven/runtime/string_pool.hpp"
#include "raven/runtime/nan_box.hpp"
#include "raven/concurrency/scheduler.hpp"
#include <thread>
#include <vector>

TEST(PlatformFilesystemTest, ExecutablePathIsPortable) {
    std::string exe_path = raven::platform::fs_executable_path();
    std::string exe_dir = raven::platform::fs_executable_dir();

    EXPECT_FALSE(exe_path.empty());
    EXPECT_FALSE(exe_dir.empty());
    EXPECT_EQ(std::filesystem::path(exe_path).parent_path().string(), exe_dir);
}

TEST(StringPoolTest, DeduplicationAndPointerIdentity) {
    auto& pool = raven::StringPool::instance();
    pool.clear();

    std::string s1 = "hello_world";
    std::string s2 = "hello_" + std::string("world");

    auto v1 = pool.intern(s1);
    auto v2 = pool.intern(s2);

    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v1.data(), v2.data()); // Pointer equality: exact same memory!
    EXPECT_TRUE(raven::StringPool::is_same(v1, v2));
    EXPECT_EQ(pool.size(), 1UL);
}

TEST(StringPoolTest, ConcurrentInterningIsThreadSafe) {
    auto& pool = raven::StringPool::instance();
    pool.clear();

    const int num_threads = 8;
    const int ops_per_thread = 500;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &pool]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string s = "key_" + std::to_string(i % 50);
                auto v = pool.intern(s);
                EXPECT_FALSE(v.empty());
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(pool.size(), 50UL);
}

TEST(NanBoxedTest, ValueSizeIsExactlyEightBytes) {
    EXPECT_EQ(sizeof(raven::runtime::NanBoxedValue), 8UL);
}

TEST(NanBoxedTest, NumberEncodingAndRetrieval) {
    raven::runtime::NanBoxedValue n1(42.0);
    EXPECT_TRUE(n1.is_number());
    EXPECT_FALSE(n1.is_nil());
    EXPECT_FALSE(n1.is_bool());
    EXPECT_FALSE(n1.is_pointer());
    EXPECT_DOUBLE_EQ(n1.as_number(), 42.0);

    raven::runtime::NanBoxedValue n2(-3.1415926535);
    EXPECT_TRUE(n2.is_number());
    EXPECT_DOUBLE_EQ(n2.as_number(), -3.1415926535);

    raven::runtime::NanBoxedValue n3(0.0);
    EXPECT_TRUE(n3.is_number());
    EXPECT_DOUBLE_EQ(n3.as_number(), 0.0);
}

TEST(NanBoxedTest, BooleansAndNilEncoding) {
    auto nil_val = raven::runtime::NanBoxedValue::from_nil();
    EXPECT_TRUE(nil_val.is_nil());
    EXPECT_FALSE(nil_val.is_bool());
    EXPECT_FALSE(nil_val.is_number());

    auto true_val = raven::runtime::NanBoxedValue::from_bool(true);
    EXPECT_TRUE(true_val.is_bool());
    EXPECT_TRUE(true_val.as_bool());
    EXPECT_FALSE(true_val.is_nil());

    auto false_val = raven::runtime::NanBoxedValue::from_bool(false);
    EXPECT_TRUE(false_val.is_bool());
    EXPECT_FALSE(false_val.as_bool());
}

TEST(NanBoxedTest, PointerPayloadEncoding) {
    int dummy = 12345;
    void* ptr = &dummy;

    auto ptr_val = raven::runtime::NanBoxedValue::from_pointer(ptr);
    EXPECT_TRUE(ptr_val.is_pointer());
    EXPECT_FALSE(ptr_val.is_number());
    EXPECT_FALSE(ptr_val.is_nil());
    EXPECT_FALSE(ptr_val.is_bool());
    EXPECT_EQ(ptr_val.as_pointer(), ptr);
}

TEST(TaskSchedulerTest, SchedulesAndExecutesMassiveBatch) {
    auto& scheduler = raven::concurrency::TaskScheduler::instance();
    EXPECT_GE(scheduler.worker_count(), 2UL);

    std::atomic<int> counter{0};
    const int task_count = 1000;

    for (int i = 0; i < task_count; ++i) {
        scheduler.submit([&counter]() {
            counter++;
        });
    }

    scheduler.wait_all();
    EXPECT_EQ(counter.load(), task_count);
}

TEST(PlatformCryptoTest, Sha1HexOutputMatchesStandard) {
    // Standard test vector: SHA1("") = da39a3ee5e6b4b0d3255bfef95601890afd80709
    EXPECT_EQ(raven::platform::sha1_hex(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    // Standard test vector: SHA1("The quick brown fox jumps over the lazy dog") = 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12
    EXPECT_EQ(raven::platform::sha1_hex("The quick brown fox jumps over the lazy dog"),
              "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

TEST(PlatformCryptoTest, WebSocketAcceptKeyRfc6455) {
    // Official RFC 6455 test vector (§1.3 / §4.2.2):
    // Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
    // Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
    EXPECT_EQ(raven::platform::websocket_accept_key("dGhlIHNhbXBsZSBub25jZQ=="),
              "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST(PlatformWebSocketTest, FrameCreateAndParseUnmasked) {
    std::string message = "{\"type\":\"chat\",\"room\":\"naruto_ep_12\",\"text\":\"Dattebayo!\"}";
    std::string wire_frame = raven::platform::websocket_create_frame(1, message);

    auto parsed = raven::platform::websocket_parse_frame(wire_frame);
    EXPECT_TRUE(parsed.valid);
    EXPECT_FALSE(parsed.need_more);
    EXPECT_TRUE(parsed.fin);
    EXPECT_EQ(parsed.opcode, 1);
    EXPECT_EQ(parsed.payload, message);
    EXPECT_EQ(parsed.consumed, wire_frame.size());
}

TEST(PlatformWebSocketTest, FrameMaskedBrowserPayload) {
    // Browser masked frame: opcode=1 (text), payload="Hello"
    std::string masked_frame;
    masked_frame.push_back(static_cast<char>(0x81)); // FIN + text
    masked_frame.push_back(static_cast<char>(0x85)); // MASK + len 5
    masked_frame.push_back(static_cast<char>(0x37)); // Mask key [0]
    masked_frame.push_back(static_cast<char>(0xfa)); // Mask key [1]
    masked_frame.push_back(static_cast<char>(0x21)); // Mask key [2]
    masked_frame.push_back(static_cast<char>(0x3d)); // Mask key [3]
    // Masked 'Hello':
    masked_frame.push_back(static_cast<char>('H' ^ 0x37));
    masked_frame.push_back(static_cast<char>('e' ^ 0xfa));
    masked_frame.push_back(static_cast<char>('l' ^ 0x21));
    masked_frame.push_back(static_cast<char>('l' ^ 0x3d));
    masked_frame.push_back(static_cast<char>('o' ^ 0x37));

    auto parsed = raven::platform::websocket_parse_frame(masked_frame);
    EXPECT_TRUE(parsed.valid);
    EXPECT_FALSE(parsed.need_more);
    EXPECT_TRUE(parsed.fin);
    EXPECT_EQ(parsed.opcode, 1);
    EXPECT_EQ(parsed.payload, "Hello");
    EXPECT_EQ(parsed.consumed, masked_frame.size());
}

TEST(PlatformWebSocketTest, FramePartialBufferNeedsMore) {
    std::string partial = "AB"; // Incomplete header
    auto parsed = raven::platform::websocket_parse_frame(partial);
    EXPECT_FALSE(parsed.valid);
    EXPECT_TRUE(parsed.need_more);
}
