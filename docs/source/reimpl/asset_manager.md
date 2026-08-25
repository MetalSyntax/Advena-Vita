# `source/reimpl/asset_manager.h` — Design Architecture & Notes

Explanatory and architectural design notes extracted from source code and replaced with concise technical Doxygen blocks. This document preserves the reasoning ('why') separated from technical API documentation.

## `struct` (line ~12)

**Source File:** `source/reimpl/asset_manager.h`

> {@link AAssetManager} provides access to an application's raw assets by
> creating {@link AAsset} objects.
>
> AAssetManager is a wrapper to the low-level native implementation
> of the java {@link AAssetManager}, a pointer can be obtained using
> AAssetManager_fromJava().
>
> The asset hierarchy may be examined like a filesystem, using
> {@link AAssetDir} objects to peruse a single directory.
>
> A native {@link AAssetManager} pointer may be shared across multiple threads.

---

## `struct` (line ~28)

**Source File:** `source/reimpl/asset_manager.h`

> {@link AAssetDir} provides access to a chunk of the asset hierarchy as if
> it were a single directory. The contents are populated by the
> {@link AAssetManager}.
>
> The list of files will be sorted in ascending order by ASCII value.

---

## `struct` (line ~38)

**Source File:** `source/reimpl/asset_manager.h`

> {@link AAsset} provides access to a read-only asset.
>
> {@link AAsset} objects are NOT thread-safe, and should not be shared across
> threads.

---

## `asset_manager.h` (line ~46) (line ~46)

**Source File:** `source/reimpl/asset_manager.h`

> Available access modes for opening assets with {@link AAssetManager_open}

---

## `asset_manager.h` (line ~48) (line ~48)

**Source File:** `source/reimpl/asset_manager.h`

> No specific information about how data will be accessed.

---

## `asset_manager.h` (line ~50) (line ~50)

**Source File:** `source/reimpl/asset_manager.h`

> Read chunks, and seek forward and backward.

---

## `asset_manager.h` (line ~52) (line ~52)

**Source File:** `source/reimpl/asset_manager.h`

> Read sequentially, with an occasional forward seek.

---

## `asset_manager.h` (line ~54) (line ~54)

**Source File:** `source/reimpl/asset_manager.h`

> Caller plans to ask for a read-only buffer with all data.

---

## `asset_manager.h` (line ~58) (line ~58)

**Source File:** `source/reimpl/asset_manager.h`

> [Non-Standard]: Create new AAssetManager object

---

## `asset_manager.h` (line ~64) (line ~64)

**Source File:** `source/reimpl/asset_manager.h`

> Open the named directory within the asset hierarchy.  The directory can then
> be inspected with the AAssetDir functions.  To open the top-level directory,
> pass in "" as the dirName.
>
> The object returned here should be freed by calling AAssetDir_close().

---

## `AAsset_close` (line ~73)

**Source File:** `source/reimpl/asset_manager.h`

> Open an asset.
>
> The object returned here should be freed by calling AAsset_close().

---

## `AAsset_close` (line ~80)

**Source File:** `source/reimpl/asset_manager.h`

> Close the asset, freeing all associated resources.

---

## `AAsset_read` (line ~85)

**Source File:** `source/reimpl/asset_manager.h`

> Attempt to read 'count' bytes of data from the current offset.
>
> Returns the number of bytes read, zero on EOF, or < 0 on error.

---

## `asset_manager.h` (line ~92) (line ~92)

**Source File:** `source/reimpl/asset_manager.h`

> Seek to the specified offset within the asset data.  'whence' uses the
> same constants as lseek()/fseek().
>
> Returns the new position on success, or (off_t) -1 on error.

---

## `AAssetDir_close` (line ~100)

**Source File:** `source/reimpl/asset_manager.h`

> Report the total amount of asset data that can be read from the current position.

---

## `AAssetDir_close` (line ~105)

**Source File:** `source/reimpl/asset_manager.h`

> Report the total size of the asset data.

---

## `AAssetDir_close` (line ~110)

**Source File:** `source/reimpl/asset_manager.h`

> Close an opened AAssetDir, freeing any related resources.

---
