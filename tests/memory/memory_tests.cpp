#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
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
#include "raven/runtime/arc_root_registry.hpp"
#include "raven/runtime/safepoint_manager.hpp"
#include "raven/vm/stack_map.hpp"
#include "raven/runtime/memory/card_table.hpp"
#include "raven/interpreter.hpp"
#include "raven/compiler.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"

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

TEST(MemoryTest, ArcRegistrationAndSnapshot) {
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    struct DummyGCNode { int id; };
    DummyGCNode node1{42};
    DummyGCNode node2{99};

    {
        raven::Arc<DummyGCNode> arc1(&node1);
        EXPECT_EQ(arc1.use_count(), 1u);
        EXPECT_EQ(reg.root_count(), 1u);

        {
            raven::Arc<DummyGCNode> arc2 = arc1;
            EXPECT_EQ(arc1.use_count(), 2u);
            EXPECT_EQ(arc2.use_count(), 2u);
            EXPECT_EQ(reg.root_count(), 1u);

            raven::Arc<DummyGCNode> arc3(&node2);
            EXPECT_EQ(reg.root_count(), 2u);

            auto snapshot = reg.snapshot_roots();
            EXPECT_EQ(snapshot.size(), 2u);
        }

        // arc2 and arc3 out of scope
        EXPECT_EQ(arc1.use_count(), 1u);
        EXPECT_EQ(reg.root_count(), 1u);
    }

    // arc1 out of scope
    EXPECT_EQ(reg.root_count(), 0u);
    EXPECT_TRUE(reg.snapshot_roots().empty());
}

TEST(MemoryTest, ArcRootRegistryWriteBarrierDuringMarking) {
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    struct DummyGCNode { int val; };
    DummyGCNode node{100};

    // When Idle, no barrier grey roots queued
    reg.set_gc_phase(raven::GCPhase::Idle);
    {
        raven::Arc<DummyGCNode> arc(&node);
        auto drained = reg.drain_grey_roots();
        EXPECT_TRUE(drained.empty());
    }

    // When in IncrementalMarking phase, write barrier activates
    reg.set_gc_phase(raven::GCPhase::IncrementalMarking);
    {
        raven::Arc<DummyGCNode> arc(&node);
        auto drained = reg.drain_grey_roots();
        EXPECT_EQ(drained.size(), 1u);
        EXPECT_EQ(drained[0], static_cast<void*>(&node));

        // Cloning also triggers write barrier to ensure newly established references are greyed
        raven::Arc<DummyGCNode> arc_copy = arc;
        drained = reg.drain_grey_roots();
        EXPECT_EQ(drained.size(), 1u);
        EXPECT_EQ(drained[0], static_cast<void*>(&node));
    }

    reg.set_gc_phase(raven::GCPhase::Idle);
    reg.clear();
}

TEST(MemoryTest, ArcRootRegistryConcurrentMultiThreadMutations) {
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    struct Node { int x; };
    const int kNumNodes = 32;
    std::vector<Node> nodes(kNumNodes);
    for (int i = 0; i < kNumNodes; ++i) nodes[i].x = i;

    std::atomic<bool> stop_flag{false};
    std::atomic<uint64_t> total_snapshots{0};

    // Background GC scanner thread taking snapshots
    std::thread gc_scanner([&]() {
        while (!stop_flag.load(std::memory_order_relaxed)) {
            auto roots = reg.snapshot_roots();
            total_snapshots.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::yield();
        }
    });

    // 16 concurrent mutator threads creating, cloning, copying, and dropping Arcs
    const int kMutatorThreads = 16;
    const int kIterations = 500;
    std::vector<std::thread> mutators;

    for (int t = 0; t < kMutatorThreads; ++t) {
        mutators.emplace_back([&, t]() {
            for (int i = 0; i < kIterations; ++i) {
                int node_idx = (t + i) % kNumNodes;
                raven::Arc<Node> arc(&nodes[node_idx]);
                raven::Arc<Node> clone = arc;
                raven::Arc<Node> moved = std::move(clone);
                EXPECT_TRUE(moved.get() != nullptr);
            }
        });
    }

    for (auto& m : mutators) {
        m.join();
    }

    stop_flag.store(true, std::memory_order_release);
    gc_scanner.join();

    EXPECT_GT(total_snapshots.load(), 0u);
    EXPECT_EQ(reg.root_count(), 0u);
    reg.clear();
}

TEST(MemoryTest, StackMapTableLookup) {
    raven::StackMapTable table;
    EXPECT_TRUE(table.empty());

    raven::StackMap sm1{16, {true, false, true}, {false, true}};
    raven::StackMap sm2{48, {false}, {true, true, false}};

    table.add_map(sm1);
    table.add_map(sm2);

    EXPECT_EQ(table.size(), 2u);

    const auto* found = table.find_map(16);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->bytecode_offset, 16u);
    EXPECT_EQ(found->operand_stack_bitmap, (std::vector<bool>{true, false, true}));
    EXPECT_EQ(found->local_slots_bitmap, (std::vector<bool>{false, true}));

    EXPECT_EQ(table.find_map(32), nullptr);

    const auto* found2 = table.find_map(48);
    ASSERT_NE(found2, nullptr);
    EXPECT_EQ(found2->bytecode_offset, 48u);
}

TEST(MemoryTest, SafepointNativeThreadParkingDoesNotBlockGC) {
    auto& sp = raven::SafepointManager::instance();
    sp.reset();

    std::atomic<bool> worker_in_native{false};
    std::atomic<bool> release_worker{false};
    std::atomic<bool> worker_done{false};

    std::thread worker([&]() {
        sp.register_thread();
        sp.enter_native();
        worker_in_native.store(true, std::memory_order_release);

        while (!release_worker.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        sp.exit_native();
        sp.unregister_thread();
        worker_done.store(true, std::memory_order_release);
    });

    while (!worker_in_native.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // GC initiates STW: worker is in native, so GC must proceed immediately without blocking!
    sp.request_safepoint();

    // request_safepoint() returned while worker is parked in native!
    EXPECT_EQ(sp.parked_thread_count(), 1u);
    EXPECT_EQ(sp.get_thread_state(worker.get_id()), raven::MutatorState::InNativeBlocked);

    release_worker.store(true, std::memory_order_release);
    sp.resume_mutators();
    worker.join();
    EXPECT_TRUE(worker_done.load());
    sp.reset();
}

TEST(MemoryTest, SafepointMutatorPollingSuspendsCleanly) {
    auto& sp = raven::SafepointManager::instance();
    sp.reset();

    std::atomic<bool> mutator_running{false};
    std::atomic<int> iteration_count{0};
    std::atomic<bool> stop_flag{false};

    std::thread mutator([&]() {
        sp.register_thread();
        mutator_running.store(true, std::memory_order_release);

        while (!stop_flag.load(std::memory_order_relaxed)) {
            iteration_count.fetch_add(1, std::memory_order_relaxed);
            sp.poll_safepoint();
            std::this_thread::yield();
        }

        sp.unregister_thread();
    });

    while (!mutator_running.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // STW Safepoint requested
    sp.request_safepoint();
    EXPECT_EQ(sp.get_thread_state(mutator.get_id()), raven::MutatorState::AtSafepoint);

    int count_during_pause = iteration_count.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    // Thread must be frozen at safepoint during STW pause!
    EXPECT_EQ(iteration_count.load(), count_during_pause);

    // Resume mutators
    sp.resume_mutators();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // Thread has resumed advancing!
    EXPECT_GT(iteration_count.load(), count_during_pause);

    stop_flag.store(true, std::memory_order_release);
    mutator.join();
    sp.reset();
}

TEST(MemoryTest, CardTableIndexingAndDirtyMarking) {
    auto& ct = raven::runtime::memory::CardTable::instance();
    uintptr_t heap_base = 0x100000;
    uintptr_t heap_end  = 0x120000; // 128 KB -> 256 cards (512 bytes each)
    ct.set_heap_bounds(heap_base, heap_end);

    EXPECT_EQ(ct.card_count(), 256u);
    EXPECT_TRUE(ct.get_dirty_cards().empty());

    void* obj1 = reinterpret_cast<void*>(heap_base + 1024); // card 2
    size_t idx1 = ct.card_index(obj1);
    EXPECT_EQ(idx1, 2u);

    ct.mark_dirty(obj1);
    EXPECT_TRUE(ct.is_dirty(idx1));
    EXPECT_FALSE(ct.is_dirty(0));
    EXPECT_FALSE(ct.is_dirty(1));

    auto dirty_cards = ct.get_dirty_cards();
    EXPECT_EQ(dirty_cards.size(), 1u);
    EXPECT_EQ(dirty_cards[0], 2u);

    ct.clear_card(idx1);
    EXPECT_FALSE(ct.is_dirty(idx1));
    EXPECT_TRUE(ct.get_dirty_cards().empty());
}

TEST(MemoryTest, GenerationalBarrierFiltersOldToYoungOnly) {
    auto& ct = raven::runtime::memory::CardTable::instance();
    uintptr_t heap_base = 0x200000;
    uintptr_t heap_end  = 0x210000;
    ct.set_heap_bounds(heap_base, heap_end);
    ct.clear_all_cards();

    void* old_obj   = reinterpret_cast<void*>(heap_base + 512); // card 1
    void* young_val = reinterpret_cast<void*>(heap_base + 2048);

    using raven::runtime::memory::Generation;
    using raven::runtime::memory::write_barrier_generational;

    // Young -> Young: No card marked
    write_barrier_generational(Generation::Young, old_obj, Generation::Young, young_val);
    EXPECT_TRUE(ct.get_dirty_cards().empty());

    // Old -> Old: No card marked
    write_barrier_generational(Generation::Old, old_obj, Generation::Old, young_val);
    EXPECT_TRUE(ct.get_dirty_cards().empty());

    // Old -> Young: Card 1 marked DIRTY!
    write_barrier_generational(Generation::Old, old_obj, Generation::Young, young_val);
    auto dirty = ct.get_dirty_cards();
    EXPECT_EQ(dirty.size(), 1u);
    EXPECT_EQ(dirty[0], 1u);

    ct.clear_all_cards();
}

TEST(MemoryTest, IncrementalMarkingBarrierTriColorInvariant) {
    using raven::runtime::memory::GCColor;
    using raven::runtime::memory::write_barrier_marking;

    std::vector<void*> mark_stack;
    int dummy = 42;

    // Case 1: Black source points to White target -> target shades Gray, pushed to mark stack
    GCColor target_white = GCColor::White;
    bool triggered = write_barrier_marking(GCColor::Black, target_white, mark_stack, &dummy);
    EXPECT_TRUE(triggered);
    EXPECT_EQ(target_white, GCColor::Gray);
    EXPECT_EQ(mark_stack.size(), 1u);
    EXPECT_EQ(mark_stack[0], &dummy);

    // Case 2: Black source points to Gray target -> No trigger
    mark_stack.clear();
    GCColor target_gray = GCColor::Gray;
    triggered = write_barrier_marking(GCColor::Black, target_gray, mark_stack, &dummy);
    EXPECT_FALSE(triggered);
    EXPECT_TRUE(mark_stack.empty());

    // Case 3: Gray source points to White target -> No trigger (Gray will trace its own children)
    mark_stack.clear();
    GCColor target_white2 = GCColor::White;
    triggered = write_barrier_marking(GCColor::Gray, target_white2, mark_stack, &dummy);
    EXPECT_FALSE(triggered);
    EXPECT_EQ(target_white2, GCColor::White);
    EXPECT_TRUE(mark_stack.empty());
}

TEST(MemoryTest, ObjectCreatedDuringIncrementalMarkingIsGreyed) {
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    reg.set_gc_phase(raven::GCPhase::IncrementalMarking);
    int dummy = 123;
    reg.register_root(&dummy);

    auto grey = reg.drain_grey_roots();
    EXPECT_EQ(grey.size(), 1u);
    if (!grey.empty()) {
        EXPECT_EQ(grey[0], &dummy);
    }

    reg.unregister_root(&dummy);
    reg.clear();
}

TEST(MemoryTest, RootDeletedDuringSnapshotGraceful) {
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    constexpr size_t kItems = 200;
    std::vector<int> dummy_ptrs(kItems, 0);
    for (size_t i = 0; i < kItems; ++i) {
        reg.register_root(&dummy_ptrs[i]);
    }

    std::atomic<bool> done{false};
    std::thread mutator([&]() {
        while (!done.load(std::memory_order_relaxed)) {
            for (size_t i = 0; i < kItems / 2; ++i) {
                reg.unregister_root(&dummy_ptrs[i]);
                reg.register_root(&dummy_ptrs[i]);
            }
        }
    });

    for (int iter = 0; iter < 100; ++iter) {
        auto snapshot = reg.snapshot_roots();
        EXPECT_GE(snapshot.size(), kItems / 2);
    }

    done.store(true);
    mutator.join();
    reg.clear();
}

TEST(MemoryTest, GenerationalPromotionYoungToOldWithCardMarking) {
    auto& ct = raven::runtime::memory::CardTable::instance();
    uintptr_t heap_base = 0x300000;
    uintptr_t heap_end  = 0x310000;
    ct.set_heap_bounds(heap_base, heap_end);
    ct.clear_all_cards();

    void* promoted_old_obj = reinterpret_cast<void*>(heap_base + 1024); // card 2
    void* young_val        = reinterpret_cast<void*>(heap_base + 3000); // card 5

    raven::runtime::memory::Generation old_gen = raven::runtime::memory::Generation::Old;
    raven::runtime::memory::Generation young_gen = raven::runtime::memory::Generation::Young;

    raven::runtime::memory::write_barrier_generational(old_gen, promoted_old_obj, young_gen, young_val);

    auto dirty = ct.get_dirty_cards();
    EXPECT_EQ(dirty.size(), 1u);
    EXPECT_EQ(dirty[0], 2u);
    EXPECT_TRUE(ct.is_dirty(2u));

    ct.clear_card(2u);
    EXPECT_FALSE(ct.is_dirty(2u));
    EXPECT_TRUE(ct.get_dirty_cards().empty());
}

TEST(MemoryTest, ConcurrentAllocationDuringGCPhaseTransitions) {
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    std::atomic<bool> done{false};
    constexpr int kThreads = 4;
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            std::vector<int> dummy(50);
            while (!done.load(std::memory_order_relaxed)) {
                for (size_t i = 0; i < dummy.size(); ++i) {
                    reg.register_root(&dummy[i]);
                }
                for (size_t i = 0; i < dummy.size(); ++i) {
                    reg.unregister_root(&dummy[i]);
                }
            }
        });
    }

    std::vector<raven::GCPhase> phases = {
        raven::GCPhase::Idle,
        raven::GCPhase::InitialMark,
        raven::GCPhase::IncrementalMarking,
        raven::GCPhase::FinalRemark,
        raven::GCPhase::Sweep,
        raven::GCPhase::Idle
    };

    for (int cycle = 0; cycle < 10; ++cycle) {
        for (auto phase : phases) {
            reg.set_gc_phase(phase);
            if (phase == raven::GCPhase::InitialMark) {
                auto snapshot = reg.snapshot_roots();
                (void)snapshot;
            } else if (phase == raven::GCPhase::IncrementalMarking) {
                auto grey = reg.drain_grey_roots();
                (void)grey;
            }
            std::this_thread::yield();
        }
    }

    done.store(true);
    for (auto& w : workers) {
        w.join();
    }
    reg.clear();
}

TEST(MemoryTest, ThreadJoiningDuringActiveSafepointParksImmediately) {
    auto& sm = raven::SafepointManager::instance();
    sm.reset();

    sm.request_safepoint();
    EXPECT_TRUE(sm.is_safepoint_requested());

    std::atomic<bool> thread_started{false};
    std::atomic<bool> thread_completed_register{false};

    std::thread joiner([&]() {
        thread_started.store(true);
        sm.register_thread();
        thread_completed_register.store(true);
        sm.unregister_thread();
    });

    while (!thread_started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    // Bounded wait: poll until the joiner thread is blocked inside register_thread
    // (parking on the safepoint barrier). A 2-second wall-clock limit prevents
    // the test from hanging indefinitely on a loaded machine while still
    // catching regressions where the thread never parks.
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!sm.is_safepoint_requested() ||
               thread_completed_register.load(std::memory_order_acquire)) {
            if (std::chrono::steady_clock::now() > deadline) {
                FAIL() << "Timed out waiting for joiner thread to park at safepoint";
            }
            std::this_thread::yield();
        }
    }

    EXPECT_FALSE(thread_completed_register.load(std::memory_order_acquire));

    sm.resume_mutators();
    joiner.join();

    EXPECT_TRUE(thread_completed_register.load());
    sm.reset();
}

TEST(MemoryTest, ThreadExitingDuringActiveSafepointNotifiesGC) {
    auto& sm = raven::SafepointManager::instance();
    sm.reset();

    std::atomic<bool> t1_ready{false};
    std::atomic<bool> t2_ready{false};
    std::atomic<bool> coordinator_waiting{false};

    std::thread t1([&]() {
        sm.register_thread();
        t1_ready.store(true);
        while (!sm.is_safepoint_requested()) {
            std::this_thread::yield();
        }
        sm.poll_safepoint();
        sm.unregister_thread();
    });

    std::thread t2([&]() {
        sm.register_thread();
        t2_ready.store(true);
        while (!coordinator_waiting.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        sm.unregister_thread();
    });

    while (!t1_ready.load() || !t2_ready.load()) {
        std::this_thread::yield();
    }

    std::thread coordinator([&]() {
        coordinator_waiting.store(true);
        sm.request_safepoint();
        sm.resume_mutators();
    });

    coordinator.join();
    t1.join();
    t2.join();

    sm.reset();
}

TEST(MemoryTest, ResourceLeaseConcurrentWithCloseDrainsAndBlocksDescriptorReuse) {
    std::atomic<bool> closed_called{false};
    auto block = std::make_shared<raven::ResourceControlBlock>(
        99, "test_file.txt", [&](int fd) {
            EXPECT_EQ(fd, 99);
            closed_called.store(true);
        });

    std::atomic<bool> lease_acquired{false};
    std::atomic<bool> lease_release_permitted{false};
    std::atomic<bool> close_completed{false};

    std::thread worker([&]() {
        raven::ResourceLease lease(block);
        EXPECT_TRUE(lease.is_valid());
        EXPECT_EQ(lease.native_fd(), 99);
        lease_acquired.store(true);

        while (!lease_release_permitted.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    });

    while (!lease_acquired.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    std::thread closer([&]() {
        block->close();
        close_completed.store(true);
    });

    // Bounded wait: poll until block->state() transitions to Closing.
    // The closer thread calls block->close() which sets the state to Closing
    // while a lease is held; we must observe that before asserting.
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (block->state() != raven::ResourceState::Closing) {
            if (std::chrono::steady_clock::now() > deadline) {
                FAIL() << "Timed out waiting for block state to reach Closing";
            }
            std::this_thread::yield();
        }
    }

    EXPECT_EQ(block->state(), raven::ResourceState::Closing);
    EXPECT_FALSE(closed_called.load());
    EXPECT_FALSE(close_completed.load());

    EXPECT_THROW({
        raven::ResourceLease bad_lease(block);
    }, std::runtime_error);

    lease_release_permitted.store(true);
    worker.join();
    closer.join();

    EXPECT_TRUE(close_completed.load());
    EXPECT_TRUE(closed_called.load());
    EXPECT_EQ(block->state(), raven::ResourceState::Closed);
    EXPECT_EQ(block->native_fd(), -1);
}

TEST(MemoryTest, StackMapCompletenessAudit) {
    // Source exercises Loop (back-edges), Call (function calls),
    // BuildArray and BuildMap (heap allocation sites) — all four must
    // have an exact StackMap entry in chunk.stack_maps.
    std::string source =
        "fn calculate(n) {\n"
        "    let sum = 0;\n"
        "    let i = 0;\n"
        "    while (i < n) {\n"
        "        sum = sum + i;\n"
        "        i = i + 1;\n"
        "    }\n"
        "    return sum;\n"
        "}\n"
        "let res = calculate(10);\n"
        "let arr = [1, 2, 3];\n"
        "let m = {\"a\": 1, \"b\": 2};\n";

    raven::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    raven::Chunk chunk;
    raven::Compiler compiler(chunk);
    bool compiled = compiler.compile(stmts);
    EXPECT_TRUE(compiled);

    // Audit every instruction in the chunk:
    // Loop, Call, BuildArray, BuildMap — every heap-allocating or
    // safepoint-triggering opcode MUST have a corresponding StackMap.
    int offset = 0;
    size_t safepoint_count = 0;
    std::ostringstream devnull;
    while (offset < static_cast<int>(chunk.code.size())) {
        raven::OpCode op = static_cast<raven::OpCode>(chunk.code[offset]);
        if (op == raven::OpCode::Loop  ||
            op == raven::OpCode::Call  ||
            op == raven::OpCode::BuildArray ||
            op == raven::OpCode::BuildMap) {
            safepoint_count++;
            const raven::StackMap* sm = chunk.stack_maps.find_map(static_cast<uint32_t>(offset));
            ASSERT_NE(sm, nullptr) << "Missing StackMap at bytecode offset " << offset
                                   << " for opcode " << static_cast<int>(op);
            EXPECT_EQ(sm->bytecode_offset, static_cast<uint32_t>(offset));
        }
        offset = chunk.disassemble_instruction(devnull, offset);
    }

    EXPECT_GT(safepoint_count, 0u);
    EXPECT_FALSE(chunk.stack_maps.empty());
}

TEST(MemoryTest, ReturnValueRemainsRootedDuringNestedUnwindingAndCleanup) {
    // Proves that when returning heap objects through nested scopes and RAII using-blocks,
    // the returned object remains rooted throughout the unwinding sequence, LIFO cleanup handlers
    // complete, and the caller receives an uncorrupted, fully-accessible data structure.
    std::string source =
        "fn make_resource(log_arr, tag) {\n"
        "    return {\n"
        "        \"tag\": tag,\n"
        "        \"close\": fn() {\n"
        "            push(log_arr, tag + \"_closed\");\n"
        "        }\n"
        "    };\n"
        "}\n"
        "fn create_nested_data(log_arr) {\n"
        "    using r1 = make_resource(log_arr, \"outer\") {\n"
        "        using r2 = make_resource(log_arr, \"middle\") {\n"
        "            using r3 = make_resource(log_arr, \"inner\") {\n"
        "                let complex_data = [\n"
        "                    {\"id\": 1, \"name\": \"payload_alpha\"},\n"
        "                    {\"id\": 2, \"name\": \"payload_beta\"},\n"
        "                    {\"nested\": [10, 20, 30]}\n"
        "                ];\n"
        "                return complex_data;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n"
        "let cleanup_log = [];\n"
        "let res = create_nested_data(cleanup_log);\n"
        "println(len(cleanup_log));\n"
        "println(cleanup_log[0]);\n"
        "println(cleanup_log[1]);\n"
        "println(cleanup_log[2]);\n"
        "println(res[0][\"name\"]);\n"
        "println(res[1][\"id\"]);\n"
        "println(res[2][\"nested\"][1]);\n";

    raven::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto statements = parser.parse();

    std::ostringstream out;
    raven::Interpreter interpreter(out);
    interpreter.interpret(statements);

    std::string expected =
        "3\n"
        "inner_closed\n"
        "middle_closed\n"
        "outer_closed\n"
        "payload_alpha\n"
        "2\n"
        "20\n";
    EXPECT_EQ(out.str(), expected);
}

TEST(MemoryTest, ResourceLeaseConcurrentWithRuntimeShutdown) {
    // Tests Scenario A: Multi-threaded lease contention vs concurrent close / shutdown.
    // Verifies:
    // 1. Multiple threads can acquire leases concurrently while Open.
    // 2. Once close() starts (Closing state), subsequent lease acquisitions are rejected.
    // 3. close() blocks until all in-flight active leases are released.
    // 4. close_fn is called exactly once.
    std::atomic<int> close_fn_count{0};
    int dummy_fd = 42;
    auto block = std::make_shared<raven::ResourceControlBlock>(
        dummy_fd, "socket://api.cluster.internal:8080",
        [&](int /*fd*/) {
            close_fn_count.fetch_add(1);
        });

    std::atomic<bool> start_flag{false};
    std::atomic<bool> shutdown_initiated{false};
    std::atomic<size_t> successful_ops{0};
    std::atomic<size_t> rejected_ops{0};

    constexpr size_t kWorkers = 8;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);

    for (size_t i = 0; i < kWorkers; ++i) {
        workers.emplace_back([&]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (int op = 0; op < 500; ++op) {
                try {
                    raven::ResourceLease lease(block);
                    EXPECT_EQ(lease.native_fd(), dummy_fd);
                    successful_ops.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                } catch (const std::runtime_error&) {
                    // Lease rejected because close() was called
                    rejected_ops.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    start_flag.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Concurrently trigger shutdown/close
    shutdown_initiated.store(true, std::memory_order_release);
    bool closed = block->close();
    EXPECT_TRUE(closed);
    EXPECT_EQ(block->state(), raven::ResourceState::Closed);
    EXPECT_EQ(block->native_fd(), -1);

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_GT(successful_ops.load(), 0u);
    EXPECT_GT(rejected_ops.load(), 0u);
    EXPECT_EQ(close_fn_count.load(), 1);

    // Duplicate close is a safe no-op
    EXPECT_FALSE(block->close());
    EXPECT_EQ(close_fn_count.load(), 1);
}

TEST(MemoryTest, ArcRootRegistryConcurrentPhaseTransitionsUnderStress) {
    // Tests Scenario B: ArcRootRegistry under heavy concurrent register/unregister
    // while GC rapidly transitions phases (Idle -> InitialMark -> IncrementalMarking -> FinalRemark -> Sweep).
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    std::atomic<bool> running{true};
    std::atomic<size_t> total_registered{0};
    constexpr size_t kNumMutators = 6;
    std::vector<std::thread> mutators;
    mutators.reserve(kNumMutators);

    for (size_t i = 0; i < kNumMutators; ++i) {
        mutators.emplace_back([&, i]() {
            uintptr_t base_addr = 0x10000000 + i * 0x100000;
            size_t local_count = 0;
            while (running.load(std::memory_order_relaxed)) {
                void* ptr = reinterpret_cast<void*>(base_addr + (local_count % 100) * 64);
                reg.register_root(ptr);
                total_registered.fetch_add(1, std::memory_order_relaxed);
                if ((local_count % 3) == 0) {
                    reg.unregister_root(ptr);
                }
                local_count++;
                if ((local_count % 50) == 0) {
                    std::this_thread::yield();
                }
            }
        });
    }

    // GC coordinator simulating multiple full collection cycles
    for (int cycle = 0; cycle < 10; ++cycle) {
        reg.set_gc_phase(raven::GCPhase::InitialMark);
        auto roots = reg.snapshot_roots();
        (void)roots;

        reg.set_gc_phase(raven::GCPhase::IncrementalMarking);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        reg.set_gc_phase(raven::GCPhase::FinalRemark);
        auto grey = reg.drain_grey_roots();
        (void)grey;

        reg.set_gc_phase(raven::GCPhase::Sweep);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        reg.set_gc_phase(raven::GCPhase::Idle);
    }

    running.store(false, std::memory_order_release);
    for (auto& t : mutators) {
        t.join();
    }

    EXPECT_GT(total_registered.load(), 1000u);
    reg.clear();
    EXPECT_EQ(reg.root_count(), 0u);
}

TEST(MemoryTest, SafepointManagerNativeBlockingExitsSafelyWithoutStaleRoots) {
    // Tests Scenario C: Mutators transitioning between Native Blocking and Running
    // while Safepoints are requested. Ensures no thread is considered safe if it's
    // still modifying heap, and that exit_native properly parks if a safepoint is active.
    auto& sp_mgr = raven::SafepointManager::instance();
    sp_mgr.reset();

    std::atomic<bool> running{true};
    std::atomic<size_t> native_trips{0};
    std::atomic<size_t> safepoints_observed{0};

    constexpr size_t kMutators = 4;
    std::vector<std::thread> threads;
    threads.reserve(kMutators);

    for (size_t i = 0; i < kMutators; ++i) {
        threads.emplace_back([&]() {
            sp_mgr.register_thread();

            while (running.load(std::memory_order_relaxed)) {
                // Mutator running Raven work
                sp_mgr.poll_safepoint();

                // Mutator transitions to native blocked (e.g. socket read, sleep)
                sp_mgr.enter_native();
                native_trips.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                sp_mgr.exit_native();

                sp_mgr.poll_safepoint();
            }

            sp_mgr.unregister_thread();
        });
    }

    // GC coordinator fires 5 Stop-The-World safepoints
    for (int pause = 0; pause < 5; ++pause) {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        sp_mgr.request_safepoint();

        // While STW is active, all registered threads MUST be either AtSafepoint or InNativeBlocked
        EXPECT_EQ(sp_mgr.parked_thread_count(), sp_mgr.registered_thread_count());

        safepoints_observed.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        sp_mgr.resume_mutators();
    }

    running.store(false, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(safepoints_observed.load(), 5u);
    EXPECT_GT(native_trips.load(), 50u);
    EXPECT_EQ(sp_mgr.registered_thread_count(), 0u);
    sp_mgr.reset();
}

TEST(MemoryTest, ResourceLeaseMoveSemanticsAndAtomicDescriptorGuarantees) {
    // Tests M11 Checklist #1 and #6:
    // 1. ResourceLease move construction and move assignment transfer the lease.
    // 2. Closed resource captures native exception into last_close_error without swallowing.
    // 3. native_fd is an atomic read.
    auto block = std::make_shared<raven::ResourceControlBlock>(
        101, "socket://m11-hardened.internal",
        [](int) {
            throw std::runtime_error("Simulated kernel close EIO failure");
        });

    EXPECT_EQ(block->native_fd(), 101);
    EXPECT_FALSE(block->had_close_error());

    {
        raven::ResourceLease lease1(block);
        EXPECT_TRUE(lease1.is_valid());
        EXPECT_EQ(lease1.native_fd(), 101);
        EXPECT_EQ(block->active_operations(), 1u);

        // Move construct
        raven::ResourceLease lease2(std::move(lease1));
        EXPECT_FALSE(lease1.is_valid());
        EXPECT_TRUE(lease2.is_valid());
        EXPECT_EQ(lease2.native_fd(), 101);
        EXPECT_EQ(block->active_operations(), 1u);

        // Move assign
        raven::ResourceLease lease3;
        EXPECT_FALSE(lease3.is_valid());
        lease3 = std::move(lease2);
        EXPECT_FALSE(lease2.is_valid());
        EXPECT_TRUE(lease3.is_valid());
        EXPECT_EQ(lease3.native_fd(), 101);
        EXPECT_EQ(block->active_operations(), 1u);
    }

    EXPECT_EQ(block->active_operations(), 0u);

    // Closing invokes native closer and catches exception cleanly into last_close_error
    bool closed = block->close();
    EXPECT_TRUE(closed);
    EXPECT_EQ(block->native_fd(), -1);
    EXPECT_TRUE(block->had_close_error());
    EXPECT_NE(block->last_close_error().find("Simulated kernel close EIO failure"), std::string::npos);
}

TEST(MemoryTest, ArcRootRegistryGreyRootLifetimePinThroughIncrementalMarking) {
    // Tests M11 Checklist #2:
    // Proves that when a root is shaded grey during IncrementalMarking, its lifetime
    // remains pinned in the registry even if the mutator unregisters its Arc before FinalRemark.
    auto& reg = raven::ArcRootRegistry::instance();
    reg.clear();

    void* test_obj = reinterpret_cast<void*>(0xbeefc001);

    // 1. Start in IncrementalMarking
    reg.set_gc_phase(raven::GCPhase::IncrementalMarking);

    // 2. Mutator registers root: write barrier shades to grey and pins lifetime
    reg.register_root(test_obj);
    EXPECT_TRUE(reg.is_root_pinned_for_gc(test_obj));

    // 3. Mutator drops Arc (strong count goes to 0) during IncrementalMarking
    reg.unregister_root(test_obj);
    EXPECT_EQ(reg.strong_count(test_obj), 0u);
    // Crucial: Object MUST remain pinned for the active GC cycle!
    EXPECT_TRUE(reg.is_root_pinned_for_gc(test_obj));

    // 4. GC transitions to FinalRemark and drains grey roots
    reg.set_gc_phase(raven::GCPhase::FinalRemark);
    auto grey = reg.drain_grey_roots();
    ASSERT_EQ(grey.size(), 1u);
    EXPECT_EQ(grey[0], test_obj);

    // 5. Sweep finishes and GC phase returns to Idle
    reg.set_gc_phase(raven::GCPhase::Sweep);
    reg.set_gc_phase(raven::GCPhase::Idle);

    // Now that cycle is over, pinned state is cleared
    EXPECT_FALSE(reg.is_root_pinned_for_gc(test_obj));
    reg.clear();
}

TEST(MemoryTest, SafepointManagerSingleGCOwnerAndSerializedCoordinators) {
    // Tests M11 Checklist #3:
    // 1. Single-GC-Owner semantics: only the thread that called request_safepoint can resume.
    // 2. Non-owner resume_mutators throws runtime_error.
    // 3. Multiple concurrent GC coordinators serialize cleanly without race or deadlock.
    auto& sp_mgr = raven::SafepointManager::instance();
    sp_mgr.reset();

    std::thread::id caller_tid = std::this_thread::get_id();
    sp_mgr.request_safepoint();
    EXPECT_EQ(sp_mgr.active_gc_owner(), caller_tid);
    EXPECT_GT(sp_mgr.safepoint_epoch(), 0u);

    // Another thread trying to call resume_mutators must fail
    std::atomic<bool> unauthorized_rejected{false};
    std::thread rouge_thread([&]() {
        try {
            sp_mgr.resume_mutators();
        } catch (const std::runtime_error&) {
            unauthorized_rejected.store(true);
        }
    });
    rouge_thread.join();
    EXPECT_TRUE(unauthorized_rejected.load());

    // Legitimate owner resumes
    sp_mgr.resume_mutators();
    EXPECT_EQ(sp_mgr.active_gc_owner(), std::thread::id{});
    EXPECT_FALSE(sp_mgr.is_safepoint_requested());

    // Concurrently request safepoints from two separate coordinator threads
    std::atomic<int> completed_coordinators{0};
    auto coordinator_work = [&]() {
        sp_mgr.request_safepoint();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        sp_mgr.resume_mutators();
        completed_coordinators.fetch_add(1);
    };

    std::thread c1(coordinator_work);
    std::thread c2(coordinator_work);
    c1.join();
    c2.join();

    EXPECT_EQ(completed_coordinators.load(), 2);
    sp_mgr.reset();
}

TEST(MemoryTest, SafepointManagerUnregisterDuringActiveSTWDoesNotHang) {
    // Tests M11 Checklist #4:
    // When a mutator thread unregisters during an active safepoint request (e.g. exit/crash),
    // the SafepointManager must immediately notify cv_gc_ so GC does not hang.
    auto& sp_mgr = raven::SafepointManager::instance();
    sp_mgr.reset();

    std::atomic<bool> thread_registered{false};
    std::atomic<bool> unregister_done{false};

    std::thread mutator([&]() {
        sp_mgr.register_thread();
        thread_registered.store(true);

        // Wait until safepoint requested
        while (!sp_mgr.is_safepoint_requested()) {
            std::this_thread::yield();
        }

        // Thread terminates and unregisters without calling poll_safepoint
        sp_mgr.unregister_thread();
        unregister_done.store(true);
    });

    while (!thread_registered.load()) {
        std::this_thread::yield();
    }

    // GC requests safepoint with 2-second timeout
    sp_mgr.request_safepoint(std::chrono::milliseconds(2000));
    EXPECT_TRUE(unregister_done.load());
    EXPECT_EQ(sp_mgr.registered_thread_count(), 0u);

    sp_mgr.resume_mutators();
    mutator.join();
    sp_mgr.reset();
}


