#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib.h>

int main(int argc, char *argv[])
{
	GChecksum *sha1;
	unsigned char buffer[1048576];
	ssize_t sz;
	int err;
	int fd;

	fprintf(stdout, "Checking SHA1 for :%s\n", argv[1]);

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		err = errno;
		fprintf(stderr, "open(): %s(%d)\n", strerror(err), err); 
		goto done;
	}

	sz = read(fd, buffer, sizeof(buffer));
	if (sz == -1) {
		err = errno;
		fprintf(stderr, "read(): %s(%d)\n", strerror(err), err); 
		goto done;
	}

	sha1 = g_checksum_new(G_CHECKSUM_SHA1);
	do {
		g_checksum_update(sha1, buffer, sz);
		sz = read(fd, buffer, sizeof(buffer));
	} while (sz > 0);

	fprintf(stdout, "SHA1: %s\n", g_checksum_get_string(sha1));

	g_checksum_free(sha1);
done:
	close(fd);

	return 0;
}
