#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "raven/runtime/memory/managed_object.hpp"
#include "raven/runtime/memory/reference_counter.hpp"
#include "raven/runtime/memory/root_registry.hpp"
#include "raven/runtime/memory/managed_heap.hpp"
#include "raven/runtime/memory/managed_ptr.hpp"
#include "raven/concurrency/container_lock.hpp"
#include "raven/environment.hpp"
#include "raven/gc.hpp"
#include "raven/runtime/resource_handle.hpp"

using namespace raven::runtime::memory;

TEST(MemoryTest, ManagedObjectRefCounting) {
    auto* obj = new ManagedObject();
    EXPECT_EQ(obj->ref_count(), 1u);

    obj->inc_ref();
    EXPECT_EQ(obj->ref_count(), 2u);

    uint32_t after_dec = obj->dec_ref();
    EXPECT_EQ(after_dec, 1u);
    EXPECT_EQ(obj->ref_count(), 1u);

    delete obj;
}

TEST(MemoryTest, ManagedObjectGCColor) {
    ManagedObject obj;
    EXPECT_EQ(obj.color(), GCColor::Black);

    obj.set_color(GCColor::Gray);
    EXPECT_EQ(obj.color(), GCColor::Gray);

    obj.set_color(GCColor::White);
    EXPECT_EQ(obj.color(), GCColor::White);
}

TEST(MemoryTest, ManagedObjectBuffered) {
    ManagedObject obj;
    EXPECT_FALSE(obj.buffered());

    obj.set_buffered(true);
    EXPECT_TRUE(obj.buffered());
}

TEST(MemoryTest, ReferenceCounterStatics) {
    std::atomic<uint32_t> counter{1};

    EXPECT_EQ(ReferenceCounter::load(counter), 1u);
    EXPECT_EQ(ReferenceCounter::inc(counter), 2u);
    EXPECT_EQ(ReferenceCounter::dec(counter), 1u);
    EXPECT_EQ(ReferenceCounter::load(counter), 1u);
}

TEST(MemoryTest, ManagedPtrBasic) {
    auto* raw = new ManagedObject();
    EXPECT_EQ(raw->ref_count(), 1u);

    {
        ManagedPtr<ManagedObject> ptr(raw);
        EXPECT_EQ(ptr->ref_count(), 1u);

        ManagedPtr<ManagedObject> ptr2 = ptr;
        EXPECT_EQ(raw->ref_count(), 2u);
    }
}

TEST(MemoryTest, ManagedPtrMove) {
    auto* raw = new ManagedObject();
    ManagedPtr<ManagedObject> ptr(raw);
    EXPECT_EQ(ptr->ref_count(), 1u);

    ManagedPtr<ManagedObject> moved = std::move(ptr);
    EXPECT_FALSE(static_cast<bool>(ptr));
    EXPECT_TRUE(static_cast<bool>(moved));
    EXPECT_EQ(moved->ref_count(), 1u);
}

TEST(MemoryTest, RootRegistrySingleton) {
    auto& reg1 = RootRegistry::instance();
    auto& reg2 = RootRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST(MemoryTest, RootRegistryAddRemove) {
    auto& reg = RootRegistry::instance();
    size_t initial = reg.root_count();

    ManagedObject obj;
    reg.add_root(&obj);
    EXPECT_EQ(reg.root_count(), initial + 1);

    reg.remove_root(&obj);
    EXPECT_EQ(reg.root_count(), initial);
}

TEST(MemoryTest, ManagedHeapSingleton) {
    auto& h1 = ManagedHeap::instance();
    auto& h2 = ManagedHeap::instance();
    EXPECT_EQ(&h1, &h2);
}

TEST(MemoryTest, ManagedHeapStats) {
    auto& heap = ManagedHeap::instance();
    auto stats = heap.stats();
    EXPECT_GE(stats.total_alloc_ops, 0u);
    EXPECT_GE(stats.peak_allocated, stats.total_allocated);
}

TEST(MemoryTest, MakeManagedHelper) {
    auto ptr = make_managed<ManagedObject>(ObjectType::String);
    EXPECT_TRUE(static_cast<bool>(ptr));
    EXPECT_EQ(ptr->type(), ObjectType::String);
    EXPECT_EQ(ptr->ref_count(), 1u);
}

TEST(MemoryTest, GCRegistrySingleton) {
    auto& gc1 = raven::gc::GCRegistry::instance();
    auto& gc2 = raven::gc::GCRegistry::instance();
    EXPECT_EQ(&gc1, &gc2);
}

TEST(MemoryTest, GCCycleCollection) {
    auto& gc = raven::gc::GCRegistry::instance();
    gc.reset_stats();

    // Create a self-referencing cyclic array: a = []; a[0] = a;
    auto cyclic_arr = std::make_shared<std::vector<raven::Value>>();
    cyclic_arr->push_back(raven::Value(cyclic_arr));
    gc.track_array(cyclic_arr);

    // Without registering root, collect() should detect and break the unreachable cycle
    size_t broken = gc.collect();
    EXPECT_GE(broken, 1u);
    EXPECT_TRUE(cyclic_arr->empty());
    EXPECT_GE(gc.stats().cycles_broken, 1u);
}

TEST(MemoryTest, GCRootProtection) {
    auto& gc = raven::gc::GCRegistry::instance();
    gc.reset_stats();

    auto live_arr = std::make_shared<std::vector<raven::Value>>();
    live_arr->push_back(raven::Value(42.0));
    gc.track_array(live_arr);

    raven::Value root_val(live_arr);
    raven::gc::ScopedRoot root(root_val);

    size_t broken = gc.collect();
    EXPECT_EQ(broken, 0u);
    EXPECT_EQ(live_arr->size(), 1u);
    EXPECT_EQ((*live_arr)[0].as_number(), 42.0);
}

TEST(MemoryTest, GCNonCyclicSafety) {
    // Verifies that a non-cyclic object held by C++ is NOT destroyed by collect()
    // even if it was not explicitly registered as a GC root.
    auto& gc = raven::gc::GCRegistry::instance();
    gc.reset_stats();

    auto safe_arr = std::make_shared<std::vector<raven::Value>>();
    safe_arr->push_back(raven::Value(123.0));
    safe_arr->push_back(raven::Value("hello"));
    gc.track_array(safe_arr);

    // Notice: NOT added to roots!
    size_t broken = gc.collect();
    EXPECT_EQ(broken, 0u);
    EXPECT_EQ(safe_arr->size(), 2u);
    EXPECT_EQ((*safe_arr)[0].as_number(), 123.0);
    EXPECT_EQ((*safe_arr)[1].as_string(), "hello");
}

TEST(MemoryTest, EnvironmentConcurrentAccess) {
    auto env = std::make_shared<raven::Environment>();
    env->define("initial", raven::Value(100.0));

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([env, t]() {
            for (int i = 0; i < 200; ++i) {
                env->define("key_" + std::to_string(t) + "_" + std::to_string(i), raven::Value(static_cast<double>(i)));
                raven::Token tok{raven::TokenType::Identifier, "initial", 1, 1};
                auto val = env->get(tok);
                EXPECT_EQ(val.as_number(), 100.0);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
}

TEST(MemoryTest, ConcurrentContainerMutations) {
    auto arr = std::make_shared<std::vector<raven::Value>>();
    auto map = std::make_shared<std::unordered_map<std::string, raven::Value>>();

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([arr, map, t]() {
            for (int i = 0; i < 200; ++i) {
                {
                    std::lock_guard<std::mutex> lk(raven::concurrency::ContainerLock::get_mutex(arr.get()));
                    arr->push_back(raven::Value(static_cast<double>(t * 1000 + i)));
                }
                {
                    std::lock_guard<std::mutex> lk(raven::concurrency::ContainerLock::get_mutex(map.get()));
                    (*map)["k_" + std::to_string(t) + "_" + std::to_string(i)] = raven::Value(static_cast<double>(i));
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(arr->size(), 8 * 200u);
    EXPECT_EQ(map->size(), 8 * 200u);
}

TEST(MemoryTest, ResourceControlBlockSingleAndDoubleClose) {
    int close_invocations = 0;
    raven::ResourceControlBlock rcb(42, "socket://127.0.0.1:8080", [&](int fd) {
        EXPECT_EQ(fd, 42);
        close_invocations++;
    });

    EXPECT_TRUE(rcb.is_open());
    EXPECT_EQ(rcb.native_fd(), 42);

    // First close succeeds
    EXPECT_TRUE(rcb.close());
    EXPECT_EQ(close_invocations, 1);
    EXPECT_TRUE(rcb.is_closed());

    // Second close is atomic no-op
    EXPECT_FALSE(rcb.close());
    EXPECT_EQ(close_invocations, 1);
}

TEST(MemoryTest, ResourceOwnerHandleMoveInvalidation) {
    auto rcb = std::make_shared<raven::ResourceControlBlock>(10, "data.txt");
    raven::ResourceOwnerHandle owner1(rcb);
    EXPECT_TRUE(owner1.is_valid());
    EXPECT_FALSE(owner1.is_moved());

    // Move ownership from owner1 to owner2
    raven::ResourceOwnerHandle owner2 = std::move(owner1);
    EXPECT_TRUE(owner1.is_moved());
    EXPECT_FALSE(owner1.is_valid());
    EXPECT_TRUE(owner2.is_valid());
    EXPECT_FALSE(owner2.is_moved());

    // owner1 cannot acquire lease anymore
    EXPECT_THROW(owner1.acquire_lease(), std::runtime_error);

    // owner2 can acquire lease and operate
    {
        raven::ResourceLease lease = owner2.acquire_lease();
        EXPECT_EQ(lease.native_fd(), 10);
    }

    // owner2 closes resource
    EXPECT_TRUE(owner2.close());
    EXPECT_FALSE(owner2.is_valid());
    EXPECT_TRUE(rcb->is_closed());
}

TEST(MemoryTest, ResourceLeaseDrainsBeforeClose) {
    std::atomic<bool> lease_held{false};
    std::atomic<bool> close_finished{false};

    auto rcb = std::make_shared<raven::ResourceControlBlock>(25, "db_socket", [](int) {});

    std::thread worker([rcb, &lease_held, &close_finished]() {
        raven::ResourceLease lease(rcb);
        lease_held.store(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_FALSE(close_finished.load()); // close MUST NOT finish while lease is active
    });

    while (!lease_held.load()) {
        std::this_thread::yield();
    }

    // Attempt to close while lease is active: must wait until worker finishes!
    EXPECT_TRUE(rcb->close());
    close_finished.store(true);

    worker.join();
    EXPECT_TRUE(rcb->is_closed());
}

TEST(MemoryTest, ResourceReaperQueueAsyncDrain) {
    auto& reaper = raven::ResourceReaperQueue::instance();
    int close_count = 0;

    for (int i = 0; i < 5; ++i) {
        auto rcb = std::make_shared<raven::ResourceControlBlock>(100 + i, "stream", [&](int) {
            close_count++;
        });
        reaper.enqueue(rcb);
    }

    EXPECT_EQ(reaper.pending_count(), 5u);
    size_t closed = reaper.drain();
    EXPECT_EQ(closed, 5u);
    EXPECT_EQ(close_count, 5);
    EXPECT_EQ(reaper.pending_count(), 0u);
}

TEST(MemoryTest, ResourceControlBlockConcurrentCloseRace) {
    std::atomic<int> close_count{0};
    auto rcb = std::make_shared<raven::ResourceControlBlock>(99, "conn", [&](int) {
        close_count.fetch_add(1, std::memory_order_relaxed);
    });

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    for (int i = 0; i < 16; ++i) {
        threads.emplace_back([rcb, &success_count]() {
            if (rcb->close()) {
                success_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(close_count.load(), 1);
    EXPECT_EQ(success_count.load(), 1);
    EXPECT_TRUE(rcb->is_closed());
}
