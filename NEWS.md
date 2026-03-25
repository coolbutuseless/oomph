
# oomph 0.1.4.9000 2026-03-25

* Add counter for total key length

# oomph 0.1.4 2026-03-09

* Switch from chaining to open-addressing/linear probing.
* Revert to 0-indexing in the core C code

# oomph 0.1.3  2026-03-02

* Switch from FNV1 to ChibiHash

# oomph 0.1.2  2025-07-07

* Refactored hashmap to be more re-usable
    * no longer assumes string keys
    * keys are copied.  hashmap previously relied on R keeping a reference to 
      the CHARSXP and hashmap just stored the `const char *` to that string.
      This was always wrong, as that string could disappear at any time since
      we never kept a lock on them to prevent garbage collection.

# oomph 0.1.1  2024-12-20

* Replaced third party hashing code by bespoke hashmap

# oomph 0.1.0  2024-12-19

* Initial release
