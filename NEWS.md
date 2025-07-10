

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
