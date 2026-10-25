# Pre-split resource baseline

`resources.lock.toml` and `expected.json` originated from the pre-refactor
`buildDir/nanres` saved before P2.1a (source HEAD `1263099`); P2.3b migrated the
record IDs to the frozen `package_id + key` derivation while keeping the same
logical records, aliases, storage classes and payloads. The fixture has two
distinct records with identical bytes, an external record and two aliases.

`resource_equivalence_test.py` copies the same policy, assets and existing lock
to independent output roots and checks the resulting logical package against
this fixed baseline. It does not regenerate expected data using the tool under
test. Ordering differences are ignored; duplicate keys, ID/alias/storage changes
and payload changes fail. This does not claim byte-identical SQLite databases
across SQLite versions or validate concurrent publication.
