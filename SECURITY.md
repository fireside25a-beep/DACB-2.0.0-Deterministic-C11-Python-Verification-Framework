# Security

DACB is an offline deterministic benchmark. The supplied C11 and Python sources do not require network access, credentials, elevated privileges, or external services.

Security-relevant properties in the release gate include fixed-width unsigned arithmetic, invalid-input rejection, sanitizer execution, GCC static analysis, and source-tree hygiene.

The recurrence itself is not intended for cryptographic use.
