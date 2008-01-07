#include "check-irc-helper.h"

void
check_server (EmpathyIrcServer *server,
              const gchar *_address,
              guint _port,
              gboolean _ssl)
{
  gchar *address;
  guint port;
  gboolean ssl;

  fail_if (server == NULL);

  g_object_get (server,
      "address", &address,
      "port", &port,
      "ssl", &ssl,
      NULL);

  fail_if (address == NULL || strcmp (address, _address) != 0);
  fail_if (port != _port);
  fail_if (ssl != _ssl);

  g_free (address);
}
