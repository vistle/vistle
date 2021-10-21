#include <archive.h>
#include <archive_entry.h>
#include <stdint.h>

int main()
{
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    int r = archive_read_open_filename(a, "test.tar", 10240);
    if (r != ARCHIVE_OK)
        return 1;

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        printf("%s\\n", archive_entry_pathname(entry));
        int64_t offset = archive_read_current_position(a);
        archive_read_data_skip(a);
    }

    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
        return 1;

    return 0;
}
