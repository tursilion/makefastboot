A simple tool to patch a cartridge ROM with a fastboot header.

The current version of the fastboot header has these features:

-Works from bare console or JagCD
-Boots in under 2 seconds instead of over 5
-Performs a GPU-based checksum of the cartridge to detect bad contacts
-Checksums are stored in unencrypted memory and can be altered without re-signing
-allows to change the cartridge width and speed headers
