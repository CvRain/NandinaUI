# Notebook example

A notes/todo desktop application used as the end-to-end example for application
architecture, resources, and persistence. The upper UI and service layers are still
being written; this document describes the completed data layer and its startup
contract.

## Build and run

```bash
meson setup buildDir --wrap-mode=nodownload
meson compile -C buildDir nandina_examples_notebook
./buildDir/examples/notebook/nandina_examples_notebook
```

The executable locates `resources/resources.db` beside itself, mounts it through
`NanApplication`, then opens `./notebook.db` (working-directory relative). The first run
creates the app database and applies every declared migration; later runs report
`0 migration(s) applied`. Moving the database under the platform data root
(`PlatformResourceLocator::data_root()`) is a planned follow-up once the app-data helper
lands; the CWD-relative path keeps the first data-layer batch easy to inspect.

## Data layer

| Path | Responsibility |
| --- | --- |
| `src/core/models/` | Plain user, note-group, note and todo records |
| `src/data/database.*` | SQLite connection, connection pragmas, prepared statements and transactions |
| `src/data/statement.*` | Typed binding, stepping and column reads |
| `src/data/migration.*` | Resource-driven migration loading, validation and application |
| `src/data/mappers/` | Explicit table column to model mapping |
| `src/data/repositories/` | User, note-group, note and todo persistence APIs |
| `src/data/migrations/` | Canonical SQL migrations |

`Database` enables foreign keys, WAL, a five-second busy timeout and normal
synchronous mode. Each migration runs in its own `BEGIN IMMEDIATE` transaction and is
recorded in `schema_migrations`; `PRAGMA user_version` mirrors the current schema.
A database containing a migration version the application does not know is rejected
instead of being silently downgraded.

Repositories return `std::expected<T, DataError>` and treat "row not found" as an
optional/bool result, not an exception. Mutating note and todo operations require the
owning `user_id`; note groups are additionally checked to belong to that user. SQL
triggers enforce the same group ownership for direct SQL. Deleting a user cascades to
their notes, groups and todos; deleting a group leaves its notes ungrouped.

## Using the repositories

Every query returns `DataResult<T>`, which is `std::expected<T, DataError>`. Check
`has_value()` before dereferencing; a failure carries `DataError::code`,
`operation` and `message`.

```cpp
data::UserRepository users(database);

auto total = users.length();
if (!total) {
    log(data::describe(total.error()));
    return;
}
if (*total == 0) {
    // first launch: no account exists yet
    router.start<RegisterPage>();
}
else {
    router.start<NotesPage>();
}
```

`find_by_id` / `find_by_username` add another layer: the outer error means the
query failed, the inner empty optional means "no such row".

```cpp
auto found = users.find_by_username("alice");
if (!found) {
    // database error: found.error()
}
else if (!*found) {
    // no user named alice
}
else {
    const models::User& alice = **found;
}
```

Create takes a `NewUser`; `password_hash` must already be hashed by
`AuthService`:

```cpp
auto created = users.create({
    .username = "alice",
    .password_hash = hash_password(password),
    .email = std::nullopt,
    .avatar = std::nullopt,
});
if (!created) {
    if (created.error().code == data::DataErrorCode::constraint_violation) {
        // username or email already exists
    }
}
```

`update` and `remove` return `DataResult<bool>`; the bool is false when no row
matched. Note, group and todo repositories follow the same pattern:

```cpp
data::NoteGroupRepository groups(database);
data::NoteRepository notes(database);
data::TodoRepository todos(database);

auto group = groups.create(alice.id, "Inbox");       // optional grouping
auto note = notes.create(alice.id, "Title", "Body", group->id);
auto todo = todos.create(alice.id, "Buy milk");
auto visible_notes = notes.list_all();               // anonymous read
```

## Migrations and nanres

`resources.toml` declares each SQL file with an explicit logical key and
`media_type = "application/sql"`. Meson runs `nanres_build_helper.py build/keys` to
produce `resources/resources.db` and the generated `resource_keys.hpp`. The data
library consumes that header in `migration.cpp`, so deleting or renaming a declaration
fails at compile time instead of at runtime.

`main.cpp` performs the startup sequence:

1. mount the resource package through `NanApplication`;
2. call `bootstrap_database(path, application.resources())`;
3. hand `bootstrap->database` to the repository/service layer.

To add a migration:

1. add `src/data/migrations/NNN_<name>.sql`; do not put `BEGIN`/`COMMIT` in it;
2. add the key/path/media-type entry to `resources.toml`;
3. add the generated constant to `kMigrationResources` in `migration.cpp` with the
   next contiguous version;
4. rebuild so nanres regenerates the package and key header;
5. run `meson test -C buildDir notebook-data`.

Never edit an already-applied migration. Add a new version so existing databases can
be upgraded deterministically.

## Tests

```bash
meson test -C buildDir notebook-data --print-errorlogs
```

Test placement: deterministic data-layer tests stay in the repository
(`examples/notebook/tests/`, registered as the `notebook-data` unit test, so they run in
CI and on Windows). UI/interaction and long-running app tests live in the untracked local
`playground/` instead of the repository. Reusable widgets are authored in `showcase/`
(with its visual acceptance) and the notebook composes them.

The test opens the real nanres package, checks the ordered migration payloads, boots
a file database twice, exercises repository CRUD/ownership/cascades, and verifies
that gaps, duplicate versions, renamed migrations and newer schemas are rejected.
