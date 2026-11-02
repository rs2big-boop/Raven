#include <gtest/gtest.h>
#include "raven/runtime/memory/managed_object.hpp"
#include "raven/runtime/memory/reference_counter.hpp"
#include "raven/runtime/memory/root_registry.hpp"
#include "raven/runtime/memory/managed_heap.hpp"
#include "raven/runtime/memory/managed_ptr.hpp"
#include "raven/gc.hpp"

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
