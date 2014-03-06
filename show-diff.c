/*
 * GIT - The information manager from hell
 *
 * Copyright (C) Linus Torvalds, 2005
 */
#include "cache.h"

#define MTIME_CHANGED	0x0001
#define CTIME_CHANGED	0x0002
#define OWNER_CHANGED	0x0004
#define MODE_CHANGED    0x0008
#define INODE_CHANGED   0x0010
#define DATA_CHANGED    0x0020

static int match_stat(struct cache_entry *ce, struct stat *st)
{
	unsigned int changed = 0;

	if (ce->mtime.sec  != (unsigned int)st->st_mtim.tv_sec ||
	    ce->mtime.nsec != (unsigned int)st->st_mtim.tv_nsec)
		changed |= MTIME_CHANGED;
	if (ce->ctime.sec  != (unsigned int)st->st_ctim.tv_sec ||
	    ce->ctime.nsec != (unsigned int)st->st_ctim.tv_nsec)
		changed |= CTIME_CHANGED;
	if (ce->st_uid != (unsigned int)st->st_uid ||
	    ce->st_gid != (unsigned int)st->st_gid)
		changed |= OWNER_CHANGED;
	if (ce->st_mode != (unsigned int)st->st_mode)
		changed |= MODE_CHANGED;
	if (ce->st_dev != (unsigned int)st->st_dev ||
	    ce->st_ino != (unsigned int)st->st_ino)
		changed |= INODE_CHANGED;
	if (ce->st_size != (unsigned int)st->st_size)
		changed |= DATA_CHANGED;
	return changed;
}

static void show_differences(struct cache_entry *ce, struct stat *cur,
	void *old_contents, unsigned long long old_size)
{
	static char cmd[1000];
	FILE *f;

    /* -u: 3 line context 
     * -: diff between stdin and file */
	snprintf(cmd, sizeof(cmd), "diff -u - %s", ce->name);
	f = popen(cmd, "w");
    /* write old content to stdin of diff */
	fwrite(old_contents, old_size, 1, f);
	pclose(f);
}

int main(int argc, char **argv)
{
	int entries = read_cache();
	int i;

	if (entries < 0) {
		perror("read_cache");
		exit(1);
	}
    /* check files that cached */
	for (i = 0; i < entries; i++) {
		struct stat st;
		struct cache_entry *ce = active_cache[i];
		int n, changed;
		unsigned int mode;
		unsigned long size;
		char type[20];
		void *new;

        /* get current stat of file */
		if (stat(ce->name, &st) < 0) {
			printf("%s: %s\n", ce->name, strerror(errno));
			continue;
		}
        /* check filt stat */
		changed = match_stat(ce, &st);
		if (!changed) {
			printf("%s: ok\n", ce->name);
			continue;
		}
        /* if file stat changed */
		printf("%.*s:  ", ce->namelen, ce->name);
		for (n = 0; n < 20; n++)
			printf("%02x", ce->sha1[n]);
		printf("\n");
        /* read file content from cache */
		new = read_sha1_file(ce->sha1, type, &size);
        /* show diff between current ce->name and old content, using diff */
		show_differences(ce, &st, new, size);
		free(new);
	}
	return 0;
}
