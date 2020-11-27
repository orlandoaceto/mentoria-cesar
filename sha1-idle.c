#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib.h>


static GMainLoop *loop = NULL;
static GChecksum *sha1;
static int fd;

static void sha1_idle_destroy(gpointer user_data)
{
	fprintf(stdout, "SHA1: %s\n", g_checksum_get_string(sha1));
	g_main_loop_quit(loop);
}

static gboolean sha1_idle(gpointer user_data)
{
	unsigned char buffer[1048576];
	ssize_t sz;
	int err;

	sz = read(fd, buffer, sizeof(buffer));
	if (sz == 0)
		return FALSE;

	if (sz == -1) {
		err = errno;
		fprintf(stderr, "read(): %s(%d)\n", strerror(err), err); 
		return FALSE;
	}

	g_checksum_update(sha1, buffer, sz);
	return TRUE;
}

int main(int argc, char *argv[])
{
	int err;

	fprintf(stdout, "Checking SHA1 for :%s\n", argv[1]);

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		err = errno;
		fprintf(stderr, "open(): %s(%d)\n", strerror(err), err); 
		goto done;
	}

	sha1 = g_checksum_new(G_CHECKSUM_SHA1);

	loop = g_main_loop_new(NULL, FALSE);
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
			sha1_idle, NULL,
			sha1_idle_destroy);

	g_main_loop_run(loop);
	g_checksum_free(sha1);

done:
	close(fd);

	return 0;
}
