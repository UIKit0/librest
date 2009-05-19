#include <stdio.h>
#include <string.h>
#include <rest/flickr-proxy.h>
#include <rest/rest-xml-parser.h>

static RestXmlNode *
get_xml (RestProxyCall *call)
{
  static RestXmlParser *parser = NULL;
  RestXmlNode *root;

  if (parser == NULL)
    parser = rest_xml_parser_new ();

  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));

  if (strcmp (root->name, "rsp") != 0)
    g_error ("Unexpected response from Flickr");

  if (strcmp (rest_xml_node_get_attr (root, "stat"), "ok") != 0)
    g_error ("Error from Flickr");

  g_object_unref (call);

  return root;
}

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  RestXmlNode *root, *node;
  char *frob, *url, *token;

  g_thread_init (NULL);
  g_type_init ();

  proxy = flickr_proxy_new ("cf4e02fc57240a9b07346ad26e291080", "cdfa2329cb206e50");

  if (argc > 1) {
    flickr_proxy_set_token (FLICKR_PROXY (proxy), argv[1]);
  } else {
    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "flickr.auth.getFrob");

    if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get frob");

    root = get_xml (call);
    frob = g_strdup (rest_xml_node_find (root, "frob")->content);
    g_print ("got frob %s\n", frob);

    url = flickr_proxy_build_login_url (FLICKR_PROXY (proxy), frob);

    g_print ("Login URL %s\n", url);

    getchar ();

    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "flickr.auth.getToken");
    rest_proxy_call_add_param (call, "frob", frob);

    if (!rest_proxy_call_run (call, NULL, NULL))
      g_error ("Cannot get token");

    root = get_xml (call);
    token = g_strdup (rest_xml_node_find (root, "token")->content);
    g_print ("Got token %s\n", token);

    flickr_proxy_set_token (FLICKR_PROXY (proxy), token);
  }

  /* Make an authenticated call */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "flickr.auth.checkToken");

  if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot check token");

  root = get_xml (call);
  node = rest_xml_node_find (root, "user");
  g_print ("Logged in as %s\n",
           rest_xml_node_get_attr (node, "fullname")
           ?: rest_xml_node_get_attr (node, "username"));

  return 0;
}
