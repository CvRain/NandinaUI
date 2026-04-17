#include <gtest/gtest.h>

#include <string>

import nandina.log;

// ============================================================
// 测试环境：在所有用例执行前初始化日志系统，执行后关闭
// ============================================================
class LogEnvironment : public testing::Environment {
public:
    void SetUp() override {
        nandina::log::init("nandina_test", nandina::log::Level::Trace);
    }

    void TearDown() override {
        nandina::log::shutdown();
    }
};

// ============================================================
// 默认 logger 快捷函数
// ============================================================

TEST(NandinaLog, DefaultTrace) {
    EXPECT_NO_THROW(nandina::log::trace("trace: value={}", 42));
}

TEST(NandinaLog, DefaultDebug) {
    EXPECT_NO_THROW(nandina::log::debug("debug: msg={}", "hello"));
}

TEST(NandinaLog, DefaultInfo) {
    EXPECT_NO_THROW(nandina::log::info("info message"));
}

TEST(NandinaLog, DefaultWarn) {
    EXPECT_NO_THROW(nandina::log::warn("warn: ratio={}", 3.14));
}

TEST(NandinaLog, DefaultError) {
    EXPECT_NO_THROW(nandina::log::error("error: code={}", -1));
}

TEST(NandinaLog, DefaultCritical) {
    EXPECT_NO_THROW(nandina::log::critical("critical: {}", "unrecoverable"));
}

// ============================================================
// 具名 Logger — 获取与基本属性
// ============================================================

TEST(NandinaLog, GetNamedLoggerReturnsCorrectName) {
    auto logger = nandina::log::get("test.foundation");
    EXPECT_EQ(logger.name(), "test.foundation");
}

TEST(NandinaLog, SameNameSharesLoggerName) {
    // 同名两次 get() 应返回指向同一底层 logger 的句柄
    auto a = nandina::log::get("test.shared");
    auto b = nandina::log::get("test.shared");
    EXPECT_EQ(a.name(), b.name());
}

TEST(NandinaLog, DifferentNamesAreDistinct) {
    auto a = nandina::log::get("test.module_a");
    auto b = nandina::log::get("test.module_b");
    EXPECT_NE(a.name(), b.name());
}

// ============================================================
// 具名 Logger — 各级别输出
// ============================================================

TEST(NandinaLog, NamedLoggerAllLevels) {
    auto logger = nandina::log::get("test.all_levels");
    EXPECT_NO_THROW(logger.trace("trace {}", 1));
    EXPECT_NO_THROW(logger.debug("debug {}", 2));
    EXPECT_NO_THROW(logger.info("info {}", 3));
    EXPECT_NO_THROW(logger.warn("warn {}", 4));
    EXPECT_NO_THROW(logger.error("error {}", 5));
    EXPECT_NO_THROW(logger.critical("critical {}", 6));
}

// ============================================================
// 格式串
// ============================================================

TEST(NandinaLog, MultipleFormatArguments) {
    auto logger = nandina::log::get("test.format");
    EXPECT_NO_THROW(logger.info("x={} y={} label={}", 10, 3.14f, "origin"));
}

TEST(NandinaLog, NoFormatArguments) {
    auto logger = nandina::log::get("test.noargs");
    EXPECT_NO_THROW(logger.info("plain message with no placeholders"));
}

// ============================================================
// 级别控制
// ============================================================

TEST(NandinaLog, SetGlobalLevelDoesNotThrow) {
    EXPECT_NO_THROW(nandina::log::set_level(nandina::log::Level::Warn));
    // 恢复，避免影响后续用例
    EXPECT_NO_THROW(nandina::log::set_level(nandina::log::Level::Trace));
}

TEST(NandinaLog, NamedLoggerSetLevelDoesNotThrow) {
    auto logger = nandina::log::get("test.level_ctrl");
    EXPECT_NO_THROW(logger.set_level(nandina::log::Level::Error));
    EXPECT_NO_THROW(logger.set_level(nandina::log::Level::Debug));
}

// ============================================================
// 刷写
// ============================================================

TEST(NandinaLog, GlobalFlushDoesNotThrow) {
    EXPECT_NO_THROW(nandina::log::flush());
}

TEST(NandinaLog, NamedLoggerFlushDoesNotThrow) {
    auto logger = nandina::log::get("test.flush");
    logger.info("before flush");
    EXPECT_NO_THROW(logger.flush());
}

// ============================================================
// init 幂等性
// ============================================================

TEST(NandinaLog, RepeatedInitIsIdempotent) {
    // 已由 LogEnvironment::SetUp 初始化，再次调用不应崩溃或重复注册
    EXPECT_NO_THROW(nandina::log::init("duplicate_init", nandina::log::Level::Debug));
}

// ============================================================
// 入口
// ============================================================

int main(int argc, char **argv) {
    testing::AddGlobalTestEnvironment(new LogEnvironment);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
