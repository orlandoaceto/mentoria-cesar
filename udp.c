
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#include <glib.h>

static GMainLoop *loop = NULL;

static int udp_listen(void)
{
  struct sockaddr_in addr;
  int err, sock;
  int enable = 1;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
  addr.sin_port = htons(8080);

  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    err = errno;
    close(sock);
    return -err;
  }

  return sock;
}

static gboolean read_cb(GIOChannel *io,
                        GIOCondition cond, gpointer user_data)
{
  char buf[255];
  ssize_t sz;
  int sock;

  sock = g_io_channel_unix_get_fd(io);

  /* TODO: Print sender infos */
  sz = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
  if (sz == -1)
    return TRUE;

  fprintf(stdout, "%s\n", buf);

  return TRUE;
}


int main(int argc, char *argv[])
{
	GIOCondition cond = (GIOCondition) (G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL);
	GIOChannel *io;
	int sock;

	loop = g_main_loop_new(NULL, FALSE);
	sock = udp_listen();
	if (sock < 0)
		return sock;

	io = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(io, TRUE);
	g_io_channel_set_flags(io, G_IO_FLAG_NONBLOCK, NULL);

	g_io_add_watch_full(io, G_PRIORITY_LOW, cond, read_cb, NULL, NULL);
	g_io_channel_unref(io);

	g_main_loop_run(loop);

	return 0;
}
