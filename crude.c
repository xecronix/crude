/*
File: crude.c
Purpose: CRUDE v1 snapshot tool for Pelles C / Win32 console.

Behavior:
- Takes 2 command-line args: <source_path> <repo_path>
- Derives project name from the source folder name
- Creates snapshots under: <repo_path>\<project_name>\revisions\000001
- Prompts for 2 note sections: description and direction
- Writes crude_notes.txt with exactly those 2 tags
- Copies the entire source tree as-is
- Skips the destination repo tree if it is encountered under the source tree
- Ignores reparse points / junctions
- Deletes the partial snapshot on failure

Notes:
- MAX_PATH rules apply in v1
- No database
- No restore
- No built-in diff UI
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CRUDE_TEXT_CAP 65536
#define CRUDE_ID_CAP 32
#define CRUDE_PATH_CAP MAX_PATH

typedef struct TextBuffer
{
    char *data;
    size_t len;
    size_t cap;
} TextBuffer;

static void print_usage(const char *exe_name);
static int normalize_full_path(const char *input, char *output, size_t output_cap);
static void trim_trailing_slashes(char *path);
static const char *path_basename(const char *path);
static int path_join(char *out, size_t out_cap, const char *left, const char *right);
static int create_directory_recursive(const char *path);
static int directory_exists(const char *path);
static int is_digits_only_6(const char *text);
static int get_next_revision_id(const char *revisions_path, unsigned int *out_next_id);
static int text_buffer_init(TextBuffer *tb, size_t initial_cap);
static void text_buffer_free(TextBuffer *tb);
static int text_buffer_append(TextBuffer *tb, const char *text);
static int text_buffer_append_line(TextBuffer *tb, const char *line);
static int read_multiline_text(const char *label, char **out_text);
static int write_crude_notes(const char *revision_path, const char *description, const char *direction);
static int path_prefix_casei(const char *text, const char *prefix);
static int path_equals_or_is_child_casei(const char *path, const char *root);
static int copy_tree_recursive(const char *src_root, const char *dst_root, const char *repo_root);
static int delete_tree_recursive(const char *path);

int main(int argc, char **argv)
{
    char source_path[CRUDE_PATH_CAP];
    char repo_path[CRUDE_PATH_CAP];
    char project_name[CRUDE_PATH_CAP];
    char project_root[CRUDE_PATH_CAP];
    char revisions_path[CRUDE_PATH_CAP];
    char revision_path[CRUDE_PATH_CAP];
    char revision_id[CRUDE_ID_CAP];
    unsigned int next_id = 0;
    char *description = NULL;
    char *direction = NULL;
    int ok = 0;

    if (argc != 3)
    {
        print_usage((argc > 0 && argv[0] != NULL) ? argv[0] : "crude.exe");
        return 1;
    }

    if (!normalize_full_path(argv[1], source_path, sizeof(source_path)))
    {
        printf("ERROR: Could not normalize source path.\n");
        return 1;
    }

    if (!normalize_full_path(argv[2], repo_path, sizeof(repo_path)))
    {
        printf("ERROR: Could not normalize repo path.\n");
        return 1;
    }

    if (!directory_exists(source_path))
    {
        printf("ERROR: Source path does not exist or is not a directory:\n%s\n", source_path);
        return 1;
    }

    if (path_equals_or_is_child_casei(source_path, repo_path))
    {
        printf("WARNING: Source path is inside the repo path. This is allowed in v1.\n");
        printf("The repo tree will still be skipped if encountered.\n\n");
    }

    if (strlen(path_basename(source_path)) == 0)
    {
        printf("ERROR: Could not derive project name from source path.\n");
        return 1;
    }

    if (strlen(path_basename(source_path)) >= sizeof(project_name))
    {
        printf("ERROR: Project name is too long.\n");
        return 1;
    }

    strcpy(project_name, path_basename(source_path));

    if (!path_join(project_root, sizeof(project_root), repo_path, project_name))
    {
        printf("ERROR: Project root path is too long.\n");
        return 1;
    }

    if (!path_join(revisions_path, sizeof(revisions_path), project_root, "revisions"))
    {
        printf("ERROR: Revisions path is too long.\n");
        return 1;
    }

    if (!create_directory_recursive(revisions_path))
    {
        printf("ERROR: Could not create revisions path:\n%s\n", revisions_path);
        return 1;
    }

    if (!get_next_revision_id(revisions_path, &next_id))
    {
        printf("ERROR: Could not determine next revision id.\n");
        return 1;
    }

    sprintf(revision_id, "%06u", next_id);

    if (!path_join(revision_path, sizeof(revision_path), revisions_path, revision_id))
    {
        printf("ERROR: Revision path is too long.\n");
        return 1;
    }

    printf("CRUDE v1\n");
    printf("Source   : %s\n", source_path);
    printf("Repo     : %s\n", repo_path);
    printf("Project  : %s\n", project_name);
    printf("Revision : %s\n\n", revision_id);

    if (!read_multiline_text("description", &description))
    {
        printf("ERROR: Failed while reading description.\n");
        goto cleanup;
    }

    if (!read_multiline_text("direction", &direction))
    {
        printf("ERROR: Failed while reading direction.\n");
        goto cleanup;
    }

    if (!CreateDirectoryA(revision_path, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            printf("ERROR: Could not create revision folder:\n%s\n", revision_path);
            goto cleanup;
        }
    }

    if (!write_crude_notes(revision_path, description, direction))
    {
        printf("ERROR: Could not write crude_notes.txt\n");
        delete_tree_recursive(revision_path);
        goto cleanup;
    }

    printf("Creating snapshot...\n");

    if (!copy_tree_recursive(source_path, revision_path, repo_path))
    {
        printf("ERROR: Snapshot copy failed. Deleting partial snapshot.\n");
        delete_tree_recursive(revision_path);
        goto cleanup;
    }

    printf("Snapshot created successfully.\n");
    printf("Location: %s\n", revision_path);
    ok = 1;

cleanup:
    if (description != NULL)
    {
        free(description);
        description = NULL;
    }

    if (direction != NULL)
    {
        free(direction);
        direction = NULL;
    }

    return ok ? 0 : 1;
}

static void print_usage(const char *exe_name)
{
    printf("Usage:\n");
    printf("  %s <source_path> <repo_path>\n\n", exe_name);
    printf("Example:\n");
    printf("  %s \"C:\\dev\\FBI_Pursuit\" \"D:\\CRUDE\"\n\n", exe_name);
    printf("This creates:\n");
    printf("  D:\\CRUDE\\FBI_Pursuit\\revisions\\000001\n");
}

static int normalize_full_path(const char *input, char *output, size_t output_cap)
{
    DWORD n;
    size_t i;

    if (input == NULL || output == NULL || output_cap == 0)
    {
        return 0;
    }

    n = GetFullPathNameA(input, (DWORD)output_cap, output, NULL);
    if (n == 0 || n >= output_cap)
    {
        return 0;
    }

    for (i = 0; output[i] != '\0'; ++i)
    {
        if (output[i] == '/')
        {
            output[i] = '\\';
        }
    }

    trim_trailing_slashes(output);
    return 1;
}

static void trim_trailing_slashes(char *path)
{
    size_t len;

    if (path == NULL)
    {
        return;
    }

    len = strlen(path);
    while (len > 0)
    {
        if (len == 3 && path[1] == ':' && path[2] == '\\')
        {
            break;
        }

        if (path[len - 1] != '\\' && path[len - 1] != '/')
        {
            break;
        }

        path[len - 1] = '\0';
        --len;
    }
}

static const char *path_basename(const char *path)
{
    const char *last_slash;

    if (path == NULL || path[0] == '\0')
    {
        return "";
    }

    last_slash = strrchr(path, '\\');
    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

static int path_join(char *out, size_t out_cap, const char *left, const char *right)
{
    size_t left_len;
    size_t right_len;
    int need_slash;

    if (out == NULL || left == NULL || right == NULL)
    {
        return 0;
    }

    left_len = strlen(left);
    right_len = strlen(right);
    need_slash = 0;

    if (left_len > 0 && left[left_len - 1] != '\\')
    {
        need_slash = 1;
    }

    if (left_len + (size_t)need_slash + right_len + 1 > out_cap)
    {
        return 0;
    }

    memcpy(out, left, left_len);
    if (need_slash)
    {
        out[left_len] = '\\';
        ++left_len;
    }

    memcpy(out + left_len, right, right_len);
    out[left_len + right_len] = '\0';
    return 1;
}

static int create_directory_recursive(const char *path)
{
    char temp[CRUDE_PATH_CAP];
    size_t len;
    size_t i;

    if (path == NULL || path[0] == '\0')
    {
        return 0;
    }

    if (strlen(path) >= sizeof(temp))
    {
        return 0;
    }

    strcpy(temp, path);
    trim_trailing_slashes(temp);
    len = strlen(temp);

    if (len == 0)
    {
        return 0;
    }

    for (i = 0; i < len; ++i)
    {
        if (temp[i] == '/')
        {
            temp[i] = '\\';
        }
    }

    for (i = 3; i < len; ++i)
    {
        if (temp[i] == '\\')
        {
            temp[i] = '\0';
            if (strlen(temp) > 0)
            {
                if (!CreateDirectoryA(temp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
                {
                    return 0;
                }
            }
            temp[i] = '\\';
        }
    }

    if (!CreateDirectoryA(temp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return 0;
    }

    return 1;
}

static int directory_exists(const char *path)
{
    DWORD attr;

    if (path == NULL)
    {
        return 0;
    }

    attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        return 0;
    }

    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static int is_digits_only_6(const char *text)
{
    int i;

    if (text == NULL || strlen(text) != 6)
    {
        return 0;
    }

    for (i = 0; i < 6; ++i)
    {
        if (text[i] < '0' || text[i] > '9')
        {
            return 0;
        }
    }

    return 1;
}

static int get_next_revision_id(const char *revisions_path, unsigned int *out_next_id)
{
    char search_path[CRUDE_PATH_CAP];
    WIN32_FIND_DATAA fd;
    HANDLE hfind;
    unsigned int max_id = 0;
    unsigned long current_id;

    if (revisions_path == NULL || out_next_id == NULL)
    {
        return 0;
    }

    if (!path_join(search_path, sizeof(search_path), revisions_path, "*"))
    {
        return 0;
    }

    hfind = FindFirstFileA(search_path, &fd);
    if (hfind == INVALID_HANDLE_VALUE)
    {
        *out_next_id = 1;
        return 1;
    }

    do
    {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            continue;
        }

        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
        {
            continue;
        }

        if (!is_digits_only_6(fd.cFileName))
        {
            continue;
        }

        current_id = strtoul(fd.cFileName, NULL, 10);
        if (current_id > max_id)
        {
            max_id = (unsigned int)current_id;
        }
    }
    while (FindNextFileA(hfind, &fd));

    FindClose(hfind);

    if (max_id >= 999999U)
    {
        return 0;
    }

    *out_next_id = max_id + 1U;
    return 1;
}

static int text_buffer_init(TextBuffer *tb, size_t initial_cap)
{
    if (tb == NULL || initial_cap == 0)
    {
        return 0;
    }

    tb->data = (char *)malloc(initial_cap);
    if (tb->data == NULL)
    {
        return 0;
    }

    tb->data[0] = '\0';
    tb->len = 0;
    tb->cap = initial_cap;
    return 1;
}

static void text_buffer_free(TextBuffer *tb)
{
    if (tb == NULL)
    {
        return;
    }

    if (tb->data != NULL)
    {
        free(tb->data);
        tb->data = NULL;
    }

    tb->len = 0;
    tb->cap = 0;
}

static int text_buffer_append(TextBuffer *tb, const char *text)
{
    size_t add_len;
    size_t needed;
    size_t new_cap;
    char *new_data;

    if (tb == NULL || tb->data == NULL || text == NULL)
    {
        return 0;
    }

    add_len = strlen(text);
    needed = tb->len + add_len + 1;

    if (needed > CRUDE_TEXT_CAP)
    {
        return 0;
    }

    if (needed > tb->cap)
    {
        new_cap = tb->cap;
        while (new_cap < needed)
        {
            new_cap *= 2;
            if (new_cap > CRUDE_TEXT_CAP)
            {
                new_cap = CRUDE_TEXT_CAP;
                break;
            }
        }

        if (new_cap < needed)
        {
            return 0;
        }

        new_data = (char *)realloc(tb->data, new_cap);
        if (new_data == NULL)
        {
            return 0;
        }

        tb->data = new_data;
        tb->cap = new_cap;
    }

    memcpy(tb->data + tb->len, text, add_len);
    tb->len += add_len;
    tb->data[tb->len] = '\0';
    return 1;
}

static int text_buffer_append_line(TextBuffer *tb, const char *line)
{
    if (!text_buffer_append(tb, line))
    {
        return 0;
    }

    if (!text_buffer_append(tb, "\r\n"))
    {
        return 0;
    }

    return 1;
}

static int read_multiline_text(const char *label, char **out_text)
{
    TextBuffer tb;
    char line[4096];
    size_t len;

    if (label == NULL || out_text == NULL)
    {
        return 0;
    }

    *out_text = NULL;

    if (!text_buffer_init(&tb, 1024))
    {
        return 0;
    }

    printf("Enter %s. End with a single . on its own line.\n", label);

    for (;;)
    {
        printf("%s> ", label);
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            break;
        }

        len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[len - 1] = '\0';
            --len;
        }

        if (strcmp(line, ".") == 0)
        {
            break;
        }

        if (!text_buffer_append_line(&tb, line))
        {
            text_buffer_free(&tb);
            return 0;
        }
    }

    *out_text = tb.data;
    return 1;
}

static int write_crude_notes(const char *revision_path, const char *description, const char *direction)
{
    char notes_path[CRUDE_PATH_CAP];
    FILE *fp;

    if (revision_path == NULL || description == NULL || direction == NULL)
    {
        return 0;
    }

    if (!path_join(notes_path, sizeof(notes_path), revision_path, "crude_notes.txt"))
    {
        return 0;
    }

    fp = fopen(notes_path, "wb");
    if (fp == NULL)
    {
        return 0;
    }

    fputs("<description>\r\n", fp);
    fputs(description, fp);
    fputs("</description>\r\n\r\n", fp);
    fputs("<direction>\r\n", fp);
    fputs(direction, fp);
    fputs("</direction>\r\n", fp);

    if (fclose(fp) != 0)
    {
        return 0;
    }

    return 1;
}

static int ascii_tolower_char(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + ('a' - 'A');
    }

    return c;
}

static int path_prefix_casei(const char *text, const char *prefix)
{
    size_t i;

    if (text == NULL || prefix == NULL)
    {
        return 0;
    }

    for (i = 0; prefix[i] != '\0'; ++i)
    {
        if (text[i] == '\0')
        {
            return 0;
        }

        if (ascii_tolower_char((unsigned char)text[i]) != ascii_tolower_char((unsigned char)prefix[i]))
        {
            return 0;
        }
    }

    return 1;
}

static int path_equals_or_is_child_casei(const char *path, const char *root)
{
    size_t root_len;

    if (path == NULL || root == NULL)
    {
        return 0;
    }

    root_len = strlen(root);
    if (!path_prefix_casei(path, root))
    {
        return 0;
    }

    if (strlen(path) == root_len)
    {
        return 1;
    }

    return path[root_len] == '\\' || path[root_len] == '/';
}

static int copy_tree_recursive(const char *src_root, const char *dst_root, const char *repo_root)
{
    char search_path[CRUDE_PATH_CAP];
    WIN32_FIND_DATAA fd;
    HANDLE hfind;

    if (src_root == NULL || dst_root == NULL || repo_root == NULL)
    {
        return 0;
    }

    if (!path_join(search_path, sizeof(search_path), src_root, "*"))
    {
        return 0;
    }

    hfind = FindFirstFileA(search_path, &fd);
    if (hfind == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    do
    {
        char src_entry[CRUDE_PATH_CAP];
        char dst_entry[CRUDE_PATH_CAP];

        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
        {
            continue;
        }

        if (!path_join(src_entry, sizeof(src_entry), src_root, fd.cFileName))
        {
            FindClose(hfind);
            return 0;
        }

        if (path_equals_or_is_child_casei(src_entry, repo_root))
        {
            continue;
        }

        if (!path_join(dst_entry, sizeof(dst_entry), dst_root, fd.cFileName))
        {
            FindClose(hfind);
            return 0;
        }

        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            {
                continue;
            }

            if (!CreateDirectoryA(dst_entry, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                FindClose(hfind);
                return 0;
            }

            if (!copy_tree_recursive(src_entry, dst_entry, repo_root))
            {
                FindClose(hfind);
                return 0;
            }
        }
        else
        {
            if (!CopyFileA(src_entry, dst_entry, FALSE))
            {
                FindClose(hfind);
                return 0;
            }
        }
    }
    while (FindNextFileA(hfind, &fd));

    FindClose(hfind);
    return 1;
}

static int delete_tree_recursive(const char *path)
{
    char search_path[CRUDE_PATH_CAP];
    WIN32_FIND_DATAA fd;
    HANDLE hfind;

    if (path == NULL || path[0] == '\0')
    {
        return 0;
    }

    if (!path_join(search_path, sizeof(search_path), path, "*"))
    {
        return 0;
    }

    hfind = FindFirstFileA(search_path, &fd);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            char entry_path[CRUDE_PATH_CAP];

            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            {
                continue;
            }

            if (!path_join(entry_path, sizeof(entry_path), path, fd.cFileName))
            {
                FindClose(hfind);
                return 0;
            }

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                {
                    RemoveDirectoryA(entry_path);
                }
                else
                {
                    if (!delete_tree_recursive(entry_path))
                    {
                        FindClose(hfind);
                        return 0;
                    }
                }
            }
            else
            {
                if (!DeleteFileA(entry_path))
                {
                    FindClose(hfind);
                    return 0;
                }
            }
        }
        while (FindNextFileA(hfind, &fd));

        FindClose(hfind);
    }

    return RemoveDirectoryA(path) != 0;
}
