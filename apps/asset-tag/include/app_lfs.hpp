#ifndef APP_INCLUDE_APP_LFS_HPP
#define APP_INCLUDE_APP_LFS_HPP

#include <cstring>
#include <string>

#include <fs/fs.h>
#include <fs/littlefs.h>
#include <storage/flash_map.h>


namespace app_lfs {

#define MAX_PATH_LEN 255

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.mnt_point = "/lfs",
	.fs_data = &storage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
};

struct manager_t {
    fs_mount_t *mp = &lfs_storage_mnt;

    manager_t() {
        int rc;

        if(fs_mount(mp) < 0) {
            throw std::runtime_error("Failed to mount");
        }

        struct fs_statvfs sbuf;
        rc = fs_statvfs(mp->mnt_point, &sbuf);
        if (rc < 0) {
            LOG_ERR("FAIL: statvfs: %d", rc);
            throw std::runtime_error("statvfs barf");
        }

        LOG_INF("%s: bsize = %lu ; frsize = %lu ;"
            " blocks = %lu ; bfree = %lu",
            log_strdup(mp->mnt_point),
            sbuf.f_bsize, sbuf.f_frsize,
            sbuf.f_blocks, sbuf.f_bfree);

        update_boot_count();
    }

    void try_wipe() {
        const flash_area* pfa;
        unsigned int id = (uintptr_t)mp->storage_dev; // WTF Zephyr
        if(flash_area_open(static_cast<uint8_t>(id), &pfa) < 0) {
            throw std::runtime_error("Failed to open lfs flash area");
        }

        // Optionally wipe the filesystem
        if(IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
            flash_area_erase(pfa, 0, pfa->fa_size);
        }

        flash_area_close(pfa);
    }

    void update_boot_count() {
        int rc;
        char fname[MAX_PATH_LEN];
        snprintf(fname, sizeof(fname), "%s/boot_count", mp->mnt_point);

        fs_dirent dirent;
        rc = fs_stat(fname, &dirent);
        LOG_INF("%s stat: %d", log_strdup(fname), rc);
        if (rc >= 0) {
            LOG_INF("\tfn '%s' siz %u", log_strdup(dirent.name), dirent.size);
        }

        fs_file_t file;
        std::memset(&file, 0, sizeof(file));

        rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
        if (rc < 0) {
            LOG_ERR("FAIL: open %s: %d", log_strdup(fname), rc);
            throw std::runtime_error("Failed to open boot count file");
        }

        uint32_t boot_count = 0;

        if (rc >= 0) {
            rc = fs_read(&file, &boot_count, sizeof(boot_count));
            LOG_INF("%s read count %u: %d", log_strdup(fname), boot_count, rc);
            rc = fs_seek(&file, 0, FS_SEEK_SET);
            LOG_INF("%s seek start: %d", log_strdup(fname), rc);
        }

        boot_count += 1;
        rc = fs_write(&file, &boot_count, sizeof(boot_count));
        LOG_INF("%s write new boot count %u: %d", log_strdup(fname),
            boot_count, rc);

        rc = fs_close(&file);
        LOG_INF("%s close: %d", log_strdup(fname), rc);
        while(log_process(false));
    }

    bool read(const char* fname_template, char* dst, size_t len) {
        int rc;
        char fname[MAX_PATH_LEN];
        snprintf(fname, sizeof(fname), fname_template, mp->mnt_point);

        fs_dirent dirent;
        rc = fs_stat(fname, &dirent);
        LOG_INF("%s stat: %d", log_strdup(fname), rc);
        if (rc >= 0) {
            LOG_INF("\tfn '%s' siz %u", log_strdup(dirent.name), dirent.size);
        }
        while(log_process(false));

        fs_file_t file;
        std::memset(&file, 0, sizeof(file));

        rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
        if (rc < 0) {
            LOG_ERR("FAIL: open %s: %d", log_strdup(fname), rc);
            return false;
        }

        if (rc >= 0) {
            rc = fs_read(&file, dst, len);
            LOG_INF("%s read '%s': %d", log_strdup(fname), log_strdup(dst), rc);
            rc = fs_seek(&file, 0, FS_SEEK_SET);
            LOG_INF("%s seek start: %d", log_strdup(fname), rc);
        }
        while(log_process(false));

        rc = fs_close(&file);
        LOG_INF("%s close: %d", log_strdup(fname), rc);
        while(log_process(false));
        return rc >= 0;
    }

    bool write_value(std::string_view value) {
        int rc;
        char fname[MAX_PATH_LEN];
        snprintf(fname, sizeof(fname), "%s/value", mp->mnt_point);

        fs_dirent dirent;
        rc = fs_stat(fname, &dirent);
        LOG_INF("%s stat: %d", log_strdup(fname), rc);
        if (rc >= 0) {
            LOG_INF("\tfn '%s' siz %u", log_strdup(dirent.name), dirent.size);
        }

        fs_file_t file;
        std::memset(&file, 0, sizeof(file));

        rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
        if (rc < 0) {
            LOG_ERR("FAIL: open %s: %d", log_strdup(fname), rc);
            return false;
        }

        char prev_value[sizeof(ass_value) + 1] = {0};

        if (rc >= 0) {
            rc = fs_read(&file, prev_value, sizeof(prev_value));
            LOG_INF("%s read value '%s': %d", log_strdup(fname), log_strdup(prev_value), rc);
            rc = fs_seek(&file, 0, FS_SEEK_SET);
            LOG_INF("%s seek start: %d", log_strdup(fname), rc);
        }

        rc = fs_write(&file, value.data(), value.size());
        LOG_INF("%s write value '%s': %d", log_strdup(fname), log_strdup(value.data()), rc);

        rc = fs_close(&file);
        LOG_INF("%s close: %d", log_strdup(fname), rc);
        return true;
    }

    bool write_data(std::string_view value) {
        int rc;
        char fname[MAX_PATH_LEN];
        snprintf(fname, sizeof(fname), "%s/data", mp->mnt_point);

        fs_dirent dirent;
        rc = fs_stat(fname, &dirent);
        LOG_INF("%s stat: %d", log_strdup(fname), rc);
        if (rc >= 0) {
            LOG_INF("\tfn '%s' siz %u", log_strdup(dirent.name), dirent.size);
        }

        fs_file_t file;
        std::memset(&file, 0, sizeof(file));

        rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
        if (rc < 0) {
            LOG_ERR("FAIL: open %s: %d", log_strdup(fname), rc);
            return false;
        }

        char prev_value[sizeof(ass_data) + 1] = {0};

        if (rc >= 0) {
            rc = fs_read(&file, prev_value, sizeof(prev_value));
            LOG_INF("%s read data '%s': %d", log_strdup(fname), log_strdup(prev_value), rc);
            rc = fs_seek(&file, 0, FS_SEEK_SET);
            LOG_INF("%s seek start: %d", log_strdup(fname), rc);
        }

        rc = fs_write(&file, value.data(), value.size());
        LOG_INF("%s write data '%s': %d", log_strdup(fname), log_strdup(value.data()), rc);

        rc = fs_close(&file);
        LOG_INF("%s close: %d", log_strdup(fname), rc);
        return true;
    }


};

}

#endif // APP_INCLUDE_APP_LFS_HPP