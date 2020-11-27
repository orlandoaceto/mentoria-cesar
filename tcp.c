
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <glib.h>

static GMainLoop *loop = NULL;

static int tcp_listen(void)
{
  struct sockaddr_in addr;
  int err, sock;
  int enable = 1;

  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    return -errno;

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                 &enable, sizeof(enable)) == -1) {
    err = errno;
    close(sock);
    return -err;
  }

  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(8081);

  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    err = errno;
    close(sock);
    return -err;
  }

  if (listen(sock, 1) == -1) {
	  err = -errno;
	  close(sock);
	  return err;
  }

  return sock;
}

static void read_destroy(gpointer user_data)
{
	fprintf(stdout, "destroyed\n");
}

static gboolean read_cb(GIOChannel *io,
                        GIOCondition cond, gpointer user_data)
{
	char buffer[1024];
	ssize_t sz;
	int sock;
	int err;

	/* Read per client */
	if (cond & G_IO_HUP)
		return FALSE;

	sock = g_io_channel_unix_get_fd(io);
	memset(buffer, 0, sizeof(buffer));
	sz = read(sock, buffer, sizeof(buffer));
	if (sz == -1) {
		err = errno;
		fprintf(stdout, "read(): %s(%d)\n", strerror(err), err);
		return FALSE;
	}

	/* TODO: Show IP of the sender */
	fprintf(stdout, "sock: %d buffer: %s\n", sock, buffer);

	return TRUE;
}

static gboolean accept_cb(GIOChannel *io,
                        GIOCondition cond, gpointer user_data)
{
	GIOCondition cli_cond = (GIOCondition) (G_IO_IN | G_IO_HUP);
	GIOChannel *cli_io;
	char buf[255];
	ssize_t sz;
	int sock;
	int client_sock;

	sock = g_io_channel_unix_get_fd(io);

	/* TODO: show accept infos */
	client_sock = accept(sock, NULL, NULL);

	/* TODO: Add tracking all clients connected */
	cli_io = g_io_channel_unix_new(client_sock);
	g_io_channel_set_close_on_unref(cli_io, TRUE);
	g_io_channel_set_flags(cli_io, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_encoding(cli_io, NULL, NULL);
	g_io_channel_set_buffered(cli_io, TRUE);

	g_io_add_watch_full(cli_io, G_PRIORITY_LOW, cli_cond, read_cb, NULL, read_destroy);
	g_io_channel_unref(cli_io);

	return TRUE;
}


int main(int argc, char *argv[])
{
	GIOCondition cond = (GIOCondition) (G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL);
	GIOChannel *io;
	int sock;

	loop = g_main_loop_new(NULL, FALSE);
	sock = tcp_listen();
	if (sock < 0)
		return sock;

	io = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(io, TRUE);
	g_io_channel_set_flags(io, G_IO_FLAG_NONBLOCK, NULL);

	g_io_add_watch_full(io, G_PRIORITY_LOW, cond, accept_cb, NULL, NULL);
	g_io_channel_unref(io);

	g_main_loop_run(loop);

	/* TODO: Cleanup on exit */

	return 0;
}
