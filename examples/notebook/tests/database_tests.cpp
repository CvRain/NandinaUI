#include <catch2/catch_test_macros.hpp>

#include "app/notebook_store.hpp"
#include "data/database.hpp"
#include "data/migration.hpp"
#include "data/repositories/note_group_repository.hpp"
#include "data/repositories/note_repository.hpp"
#include "data/repositories/todo_repository.hpp"
#include "data/repositories/user_repository.hpp"

#include <nandina/reactive/graph.hpp>
#include <nandina/resource/backends/sqlite_backend.hpp>
#include <nandina/resource/resource_manager.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <utility>
#include <vector>

namespace
{
    namespace data = nan_example::notebook::data;
    namespace models = nan_example::notebook::models;

    [[nodiscard]] auto package_directory() -> std::filesystem::path {
        const auto* directory = std::getenv("NOTEBOOK_RESOURCE_DIR");
        REQUIRE(directory != nullptr);
        return std::filesystem::path(directory);
    }

    [[nodiscard]] auto empty_database() -> data::Database {
        auto database = data::Database::open_memory();
        REQUIRE(database.has_value());
        return std::move(*database);
    }

    [[nodiscard]] auto package_manager() -> nandina::resource::ResourceManager {
        auto backend = nandina::resource::SQLiteBackend::open(
            {.database = package_directory() / "resources.db"}
        );
        REQUIRE(backend.has_value());

        nandina::resource::ResourceManager manager;
        (void)manager.mount(*backend);
        return manager;
    }

    struct Migrated {
        data::Database database;
        data::MigrationReport report;
    };

    [[nodiscard]] auto migrated_database() -> Migrated {
        auto manager = package_manager();
        auto migrations = data::load_migrations(manager);
        REQUIRE(migrations.has_value());

        auto database = data::Database::open_memory();
        REQUIRE(database.has_value());
        auto report = data::apply_migrations(*database, *migrations);
        REQUIRE(report.has_value());
        return {.database = std::move(*database), .report = std::move(*report)};
    }
} // namespace

TEST_CASE("nanres package carries ordered notebook migrations", "[notebook][data]") {
    auto manager = package_manager();
    auto migrations = data::load_migrations(manager);

    REQUIRE(migrations.has_value());
    REQUIRE(migrations->size() == 4);
    CHECK(migrations->at(0).version == 1);
    CHECK(migrations->at(0).name == "001_create_user");
    CHECK(migrations->at(1).version == 2);
    CHECK(migrations->at(2).version == 3);
    CHECK(migrations->at(3).version == 4);
    CHECK(migrations->at(0).sql.find("CREATE TABLE IF NOT EXISTS users") != std::string::npos);
    CHECK(
        migrations->at(1).sql.find("CREATE TABLE IF NOT EXISTS note_groups") != std::string::npos
    );
    CHECK(migrations->at(2).sql.find("CREATE TABLE IF NOT EXISTS notes") != std::string::npos);
    CHECK(migrations->at(3).sql.find("CREATE TABLE IF NOT EXISTS todos") != std::string::npos);
}

TEST_CASE("bootstrap creates the database once and records schema versions", "[notebook][data]") {
    auto manager = package_manager();
    const auto path = package_directory().parent_path() / "notebook-bootstrap-test.db";
    std::filesystem::remove(path);

    {
        auto first = data::bootstrap_database(path, manager);
        REQUIRE(first.has_value());
        CHECK(first->created);
        CHECK(first->migrations.previous_version == 0);
        CHECK(first->migrations.current_version == 4);
        CHECK(first->migrations.applied == std::vector<int> {1, 2, 3, 4});
        CHECK(std::filesystem::is_regular_file(path));
    }
    {
        auto second = data::bootstrap_database(path, manager);
        REQUIRE(second.has_value());
        CHECK_FALSE(second->created);
        CHECK(second->migrations.previous_version == 4);
        CHECK(second->migrations.current_version == 4);
        CHECK(second->migrations.applied.empty());
    }

    std::filesystem::remove(path);
}

TEST_CASE("repositories map users, groups and notes and honor ownership", "[notebook][data]") {
    auto fixture = migrated_database();
    auto& database = fixture.database;

    data::UserRepository users(database);
    data::NoteGroupRepository groups(database);
    data::NoteRepository notes(database);
    data::TodoRepository todos(database);

    auto initial_users = users.length();
    REQUIRE(initial_users.has_value());
    CHECK(*initial_users == 0);

    auto alice = users.create({
        .username = "alice",
        .password_hash = "argon2-placeholder",
        .email = "alice@example.com",
        .avatar = std::vector<std::byte> {std::byte {1}, std::byte {2}, std::byte {3}},
    });
    REQUIRE(alice.has_value());
    CHECK(alice->id > 0);
    CHECK(alice->username == "alice");
    CHECK(alice->email == std::optional<std::string>("alice@example.com"));
    CHECK(alice->avatar.has_value());
    CHECK(alice->avatar->size() == 3);

    auto duplicate = users.create({
        .username = "alice",
        .password_hash = "another-hash",
    });
    REQUIRE_FALSE(duplicate.has_value());
    CHECK(duplicate.error().code == data::DataErrorCode::constraint_violation);

    auto bob = users.create({.username = "bob", .password_hash = "bob-hash"});
    REQUIRE(bob.has_value());
    auto populated_users = users.length();
    REQUIRE(populated_users.has_value());
    CHECK(*populated_users == 2);
    auto found_alice = users.find_by_username("alice");
    REQUIRE(found_alice.has_value());
    REQUIRE(found_alice->has_value());
    auto found_nobody = users.find_by_username("nobody");
    REQUIRE(found_nobody.has_value());
    CHECK_FALSE(found_nobody->has_value());

    auto inbox = groups.create(alice->id, "Inbox", 0);
    REQUIRE(inbox.has_value());
    auto archive = groups.create(alice->id, "Archive", 1);
    REQUIRE(archive.has_value());
    auto listed_groups = groups.list_for_user(alice->id);
    REQUIRE(listed_groups.has_value());
    CHECK(listed_groups->size() == 2);

    auto duplicate_group = groups.create(alice->id, "Inbox", 2);
    REQUIRE_FALSE(duplicate_group.has_value());
    CHECK(duplicate_group.error().code == data::DataErrorCode::constraint_violation);

    auto note = notes.create(alice->id, "First note", "Body", inbox->id);
    REQUIRE(note.has_value());
    CHECK(note->user_id == alice->id);
    CHECK(note->group_id == std::optional<std::int64_t>(inbox->id));

    auto cross_user = notes.create(bob->id, "Wrong group", "", inbox->id);
    REQUIRE_FALSE(cross_user.has_value());
    CHECK(cross_user.error().code == data::DataErrorCode::constraint_violation);

    note->title = "Updated note";
    note->content = "Updated body";
    auto updated = notes.update(*note);
    REQUIRE(updated.has_value());
    CHECK(*updated);

    auto reloaded = notes.find(note->id);
    REQUIRE(reloaded.has_value());
    REQUIRE(reloaded->has_value());
    CHECK((*reloaded)->title == "Updated note");
    CHECK((*reloaded)->content == "Updated body");

    auto all_notes = notes.list_all();
    REQUIRE(all_notes.has_value());
    CHECK(all_notes->size() == 1);
    auto user_notes = notes.list_for_user(alice->id);
    REQUIRE(user_notes.has_value());
    CHECK(user_notes->size() == 1);

    auto grouped = notes.create(alice->id, "Grouped", "", inbox->id);
    REQUIRE(grouped.has_value());
    auto removed_group = groups.remove(inbox->id, alice->id);
    REQUIRE(removed_group.has_value());
    CHECK(*removed_group);
    auto after_group_delete = notes.find(grouped->id);
    REQUIRE(after_group_delete.has_value());
    REQUIRE(after_group_delete->has_value());
    CHECK_FALSE((*after_group_delete)->group_id.has_value());

    auto moved = notes.move_to_group(note->id, alice->id, archive->id);
    REQUIRE(moved.has_value());
    CHECK(*moved);
    auto after_move = notes.find(note->id);
    REQUIRE(after_move.has_value());
    REQUIRE(after_move->has_value());
    CHECK((*after_move)->group_id == std::optional<std::int64_t>(archive->id));

    auto todo = todos.create(alice->id, "Remember", "One item");
    REQUIRE(todo.has_value());
    auto removed_user = users.remove(alice->id);
    REQUIRE(removed_user.has_value());
    CHECK(*removed_user);
    auto remaining_users = users.length();
    REQUIRE(remaining_users.has_value());
    CHECK(*remaining_users == 1);
    auto removed_alice = users.find_by_id(alice->id);
    REQUIRE(removed_alice.has_value());
    CHECK_FALSE(removed_alice->has_value());
    auto removed_notes = notes.list_for_user(alice->id);
    auto removed_groups = groups.list_for_user(alice->id);
    auto removed_todos = todos.list_for_user(alice->id);
    REQUIRE(removed_notes.has_value());
    REQUIRE(removed_groups.has_value());
    REQUIRE(removed_todos.has_value());
    CHECK(removed_notes->empty());
    CHECK(removed_groups->empty());
    CHECK(removed_todos->empty());
}

TEST_CASE("todo repository filters and clears completed rows", "[notebook][data]") {
    auto fixture = migrated_database();
    auto& database = fixture.database;

    data::UserRepository users(database);
    data::TodoRepository todos(database);

    auto owner = users.create({.username = "owner", .password_hash = "hash"});
    REQUIRE(owner.has_value());

    auto first = todos.create(
        owner->id,
        "Ship the example",
        "Database and migrations",
        std::string("2026-10-30T09:00:00Z")
    );
    REQUIRE(first.has_value());
    CHECK_FALSE(first->completed);
    CHECK(first->due_at == std::optional<std::string>("2026-10-30T09:00:00Z"));

    auto second = todos.create(owner->id, "Second");
    REQUIRE(second.has_value());
    auto pending = todos.list_for_user(owner->id, false);
    REQUIRE(pending.has_value());
    CHECK(pending->size() == 2);

    auto completed = todos.set_completed(first->id, owner->id, true);
    REQUIRE(completed.has_value());
    CHECK(*completed);
    auto completed_only = todos.list_for_user(owner->id, true);
    REQUIRE(completed_only.has_value());
    REQUIRE(completed_only->size() == 1);
    CHECK(completed_only->front().id == first->id);

    auto cleared = todos.clear_completed(owner->id);
    REQUIRE(cleared.has_value());
    CHECK(*cleared == 1);
    auto remaining = todos.list_for_user(owner->id);
    REQUIRE(remaining.has_value());
    REQUIRE(remaining->size() == 1);
    CHECK(remaining->front().id == second->id);
}

TEST_CASE("migration runner rejects gaps, duplicates and newer schemas", "[notebook][data]") {
    auto manager = package_manager();
    auto migrations = data::load_migrations(manager);
    REQUIRE(migrations.has_value());

    auto duplicate = std::vector<data::Migration> {
        migrations->at(0),
        migrations->at(0),
    };
    auto duplicate_database = empty_database();
    auto duplicate_report = data::apply_migrations(duplicate_database, duplicate);
    REQUIRE_FALSE(duplicate_report.has_value());
    CHECK(duplicate_report.error().code == data::DataErrorCode::migration_duplicate_version);

    auto gap = std::vector<data::Migration> {
        migrations->at(0),
        migrations->at(2),
    };
    auto gap_database = empty_database();
    auto gap_report = data::apply_migrations(gap_database, gap);
    REQUIRE_FALSE(gap_report.has_value());
    CHECK(gap_report.error().code == data::DataErrorCode::migration_out_of_order);

    auto fixture = migrated_database();
    auto older = std::vector<data::Migration>(migrations->begin(), migrations->end() - 1);
    auto newer_report = data::apply_migrations(fixture.database, older);
    REQUIRE_FALSE(newer_report.has_value());
    CHECK(newer_report.error().code == data::DataErrorCode::migration_newer_schema);

    auto renamed = *migrations;
    renamed.at(1).name = "renamed_migration";
    auto name_report = data::apply_migrations(fixture.database, renamed);
    REQUIRE_FALSE(name_report.has_value());
    CHECK(name_report.error().code == data::DataErrorCode::migration_name_mismatch);
}

TEST_CASE("NotebookStore wires UserService to the migrated database", "[notebook][data]") {
    auto manager = package_manager();
    auto migrations = data::load_migrations(manager);
    REQUIRE(migrations.has_value());

    auto database = data::Database::open_memory();
    REQUIRE(database.has_value());
    auto report = data::apply_migrations(*database, *migrations);
    REQUIRE(report.has_value());

    nandina::reactive::Graph graph;
    nan_example::notebook::app::NotebookStore store(graph, std::move(*database));
    auto& service = store.user_service();

    auto count = service.user_count();
    REQUIRE(count.has_value());
    CHECK(*count == 0);
    auto exists = service.user_exist();
    REQUIRE(exists.has_value());
    CHECK_FALSE(*exists);

    auto created = service.create_user("demo", "demo-password-hash");
    REQUIRE(created.has_value());
    CHECK(created->username == "demo");

    auto populated = service.user_count();
    REQUIRE(populated.has_value());
    CHECK(*populated == 1);
    auto now_exists = service.user_exist();
    REQUIRE(now_exists.has_value());
    CHECK(*now_exists);

    auto fetched = service.get_user_by_name("demo");
    REQUIRE(fetched.has_value());
    REQUIRE(fetched->has_value());
    CHECK((*fetched)->id == created->id);

    auto duplicate = service.create_user("demo", "another-hash");
    REQUIRE_FALSE(duplicate.has_value());
    CHECK(duplicate.error().code == data::DataErrorCode::constraint_violation);
}
