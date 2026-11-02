#include <gtest/gtest.h>
#include "raven/packages/lockfile.hpp"
#include "raven/packages/dependency_resolver.hpp"
#include "raven/packages/package_manager.hpp"
#include <filesystem>

namespace fs = std::filesystem;
using namespace raven::packages;

TEST(PackageTest, LockfileReadWriteRoundtrip) {
    std::string test_path = "/tmp/test_raven.lock";
    std::vector<LockedPackage> original = {
        {"math-lib", "1.2.0", "github.com/raven/math-lib", "sha256:abc123"},
        {"net-http", "0.5.1", "github.com/raven/net-http", "sha256:def456"}
    };

    bool ok = Lockfile::write(test_path, original);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(Lockfile::exists(test_path));

    auto parsed = Lockfile::read(test_path);
    ASSERT_EQ(parsed.size(), 2u);
    EXPECT_EQ(parsed[0].name, "math-lib");
    EXPECT_EQ(parsed[0].version, "1.2.0");
    EXPECT_EQ(parsed[0].source, "github.com/raven/math-lib");
    EXPECT_EQ(parsed[0].hash, "sha256:abc123");

    EXPECT_EQ(parsed[1].name, "net-http");
    EXPECT_EQ(parsed[1].version, "0.5.1");

    fs::remove(test_path);
}

TEST(PackageTest, DependencyResolverLinear) {
    DependencyResolver resolver;
    resolver.add_package({"A", ">=1.0", {"B"}});
    resolver.add_package({"B", ">=1.0", {"C"}});
    resolver.add_package({"C", ">=1.0", {}});

    std::vector<std::string> order;
    std::string err;
    bool ok = resolver.resolve(order, err);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(err.empty());
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "C");
    EXPECT_EQ(order[1], "B");
    EXPECT_EQ(order[2], "A");
}

TEST(PackageTest, DependencyResolverCircularDetection) {
    DependencyResolver resolver;
    resolver.add_package({"pkg1", "1.0", {"pkg2"}});
    resolver.add_package({"pkg2", "1.0", {"pkg1"}});

    std::vector<std::string> order;
    std::string err;
    bool ok = resolver.resolve(order, err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
    EXPECT_NE(err.find("Circular dependency"), std::string::npos);
}

TEST(PackageTest, PackageManagerInitManifest) {
    std::string manifest = "/tmp/test_raven.json";
    fs::remove(manifest);

    bool ok = PackageManager::init_manifest("test_app", manifest);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(manifest));

    // Second init should fail because file exists
    EXPECT_FALSE(PackageManager::init_manifest("test_app", manifest));

    fs::remove(manifest);
}

TEST(PackageTest, PackageManagerSearchRegistry) {
    auto all_pkgs = PackageManager::search_registry("");
    EXPECT_GE(all_pkgs.size(), 5u);

    auto router_pkg = PackageManager::search_registry("router");
    ASSERT_FALSE(router_pkg.empty());
    EXPECT_EQ(router_pkg[0].name, "raven-router");

    auto redis_pkg = PackageManager::search_registry("redis");
    ASSERT_FALSE(redis_pkg.empty());
    EXPECT_EQ(redis_pkg[0].name, "raven-redis");
}

TEST(PackageTest, PackageManagerAddDependencyAndPublish) {
    std::string manifest = "/tmp/test_add_dep.json";
    fs::remove(manifest);

    EXPECT_TRUE(PackageManager::init_manifest("demo_pkg", manifest));
    EXPECT_TRUE(PackageManager::add_dependency("raven-jwt", "0.9.4", manifest));

    std::string pub_msg;
    EXPECT_TRUE(PackageManager::publish_package(manifest, pub_msg));
    EXPECT_NE(pub_msg.find("successfully validated"), std::string::npos);

    fs::remove(manifest);
}
