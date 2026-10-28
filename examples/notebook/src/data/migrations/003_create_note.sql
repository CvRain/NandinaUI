CREATE TABLE IF NOT EXISTS notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    group_id INTEGER REFERENCES note_groups(id) ON DELETE SET NULL,
    title TEXT NOT NULL DEFAULT '',
    content TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_notes_user_updated
    ON notes (user_id, updated_at DESC, id DESC);

CREATE INDEX IF NOT EXISTS idx_notes_group
    ON notes (group_id);

-- A note may only be placed in a group owned by the same user. The repositories
-- check this too so callers get a typed error instead of a raw constraint failure;
-- the triggers keep direct SQL and future callers consistent.
CREATE TRIGGER IF NOT EXISTS notes_group_owner_insert
BEFORE INSERT ON notes
WHEN NEW.group_id IS NOT NULL
    AND NOT EXISTS (
        SELECT 1 FROM note_groups
        WHERE id = NEW.group_id AND user_id = NEW.user_id
    )
BEGIN
    SELECT RAISE(ABORT, 'note group belongs to another user');
END;

CREATE TRIGGER IF NOT EXISTS notes_group_owner_update
BEFORE UPDATE OF group_id, user_id ON notes
WHEN NEW.group_id IS NOT NULL
    AND NOT EXISTS (
        SELECT 1 FROM note_groups
        WHERE id = NEW.group_id AND user_id = NEW.user_id
    )
BEGIN
    SELECT RAISE(ABORT, 'note group belongs to another user');
END;
