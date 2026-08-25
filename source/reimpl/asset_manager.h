#ifndef ANDROID_ASSET_MANAGER_H
#define ANDROID_ASSET_MANAGER_H

#include <sys/cdefs.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AAssetManager;
/**
 * @brief {@link AAssetManager} provides access to an application's raw assets by creating {@link AAsset} objects.
 * @note See `docs/source/reimpl/asset_manager.md:12` for detailed design rationale.
 */
typedef struct AAssetManager AAssetManager;

struct AAssetDir;
/**
 * @brief {@link AAssetDir} provides access to a chunk of the asset hierarchy as if it were a single directory.
 * @note See `docs/source/reimpl/asset_manager.md:28` for detailed design rationale.
 */
typedef struct AAssetDir AAssetDir;

struct AAsset;
/**
 * @brief {@link AAsset} provides access to a read-only asset.
 * @note See `docs/source/reimpl/asset_manager.md:38` for detailed design rationale.
 */
typedef struct AAsset AAsset;

/**
 * @brief Available access modes for opening assets with {@link AAssetManager_open}.
 * @note See `docs/source/reimpl/asset_manager.md:46` for detailed design rationale.
 */
enum {
    /**
     * @brief No specific information about how data will be accessed.
     * @note See `docs/source/reimpl/asset_manager.md:48` for detailed design rationale.
     */
    AASSET_MODE_UNKNOWN      = 0,
    /**
     * @brief Read chunks, and seek forward and backward.
     * @note See `docs/source/reimpl/asset_manager.md:50` for detailed design rationale.
     */
    AASSET_MODE_RANDOM       = 1,
    /**
     * @brief Read sequentially, with an occasional forward seek.
     * @note See `docs/source/reimpl/asset_manager.md:52` for detailed design rationale.
     */
    AASSET_MODE_STREAMING    = 2,
    /**
     * @brief Caller plans to ask for a read-only buffer with all data.
     * @note See `docs/source/reimpl/asset_manager.md:54` for detailed design rationale.
     */
    AASSET_MODE_BUFFER       = 3
};

/**
 * @brief [Non-Standard]: Create new AAssetManager object.
 * @note See `docs/source/reimpl/asset_manager.md:58` for detailed design rationale.
 */

AAssetManager * AAssetManager_create();

/**
 * @brief Open the named directory within the asset hierarchy.
 * @note See `docs/source/reimpl/asset_manager.md:64` for detailed design rationale.
 */
AAssetDir* AAssetManager_openDir(AAssetManager* mgr, const char* dirName);

/**
 * @brief Open an asset. The object returned here should be freed by calling AAsset_close().
 * @note See `docs/source/reimpl/asset_manager.md:73` for detailed design rationale.
 */
AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode);

/**
 * @brief Close the asset, freeing all associated resources.
 * @note See `docs/source/reimpl/asset_manager.md:80` for detailed design rationale.
 */
void AAsset_close(AAsset* asset);

/**
 * @brief Attempt to read 'count' bytes of data from the current offset.
 * @note See `docs/source/reimpl/asset_manager.md:85` for detailed design rationale.
 */
int AAsset_read(AAsset* asset, void* buf, size_t count);

/**
 * @brief Seek to the specified offset within the asset data.
 * @note See `docs/source/reimpl/asset_manager.md:92` for detailed design rationale.
 */
off_t AAsset_seek(AAsset* asset, off_t offset, int whence);

/**
 * @brief Report the total amount of asset data that can be read from the current position.
 * @note See `docs/source/reimpl/asset_manager.md:100` for detailed design rationale.
 */
off_t AAsset_getRemainingLength(AAsset* asset);

/**
 * @brief Report the total size of the asset data.
 * @note See `docs/source/reimpl/asset_manager.md:105` for detailed design rationale.
 */
off_t AAsset_getLength(AAsset* asset);

/**
 * @brief Close an opened AAssetDir, freeing any related resources.
 * @note See `docs/source/reimpl/asset_manager.md:110` for detailed design rationale.
 */
void AAssetDir_close(AAssetDir* assetDir);


#ifdef __cplusplus
};
#endif

#endif      // ANDROID_ASSET_MANAGER_H
