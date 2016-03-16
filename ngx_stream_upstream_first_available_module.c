/*
 * This module defines a new load balanceing method for an upstream of a stream.
 * This method uses a very easy logic. It takes the first server in the list
 * that is available.
 *
 * By Silvio Massari <silvio.massari@gmail.com>
 *
 * Based on nginx's 'ngx_stream_upstream_least_conn_module.c' by Maxim Dounin, Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

static ngx_int_t ngx_stream_upstream_init_first_available_peer(
    ngx_stream_session_t *s, ngx_stream_upstream_srv_conf_t *us);
static ngx_int_t ngx_stream_upstream_get_first_available_peer(
    ngx_peer_connection_t *pc, void *data);
static char *ngx_stream_upstream_first_available(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_stream_upstream_first_available_commands[] = {

    { ngx_string("first_available"),
      NGX_STREAM_UPS_CONF|NGX_CONF_NOARGS,
      ngx_stream_upstream_first_available,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_stream_module_t  ngx_stream_upstream_first_available_module_ctx = {
    NULL,                                    /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */
};


ngx_module_t  ngx_stream_upstream_first_available_module = {
    NGX_MODULE_V1,
    &ngx_stream_upstream_first_available_module_ctx, /* module context */
    ngx_stream_upstream_first_available_commands, /* module directives */
    NGX_STREAM_MODULE,                       /* module type */
    NULL,                                    /* init master */
    NULL,                                    /* init module */
    NULL,                                    /* init process */
    NULL,                                    /* init thread */
    NULL,                                    /* exit thread */
    NULL,                                    /* exit process */
    NULL,                                    /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_stream_upstream_init_first_available(ngx_conf_t *cf,
    ngx_stream_upstream_srv_conf_t *us)
{
    ngx_log_debug0(NGX_LOG_DEBUG_STREAM, cf->log, 0,
                   "init first_available");

    if (ngx_stream_upstream_init_round_robin(cf, us) != NGX_OK) {
        return NGX_ERROR;
    }

    us->peer.init = ngx_stream_upstream_init_first_available_peer;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_upstream_init_first_available_peer(ngx_stream_session_t *s,
    ngx_stream_upstream_srv_conf_t *us)
{
    ngx_log_debug0(NGX_LOG_DEBUG_STREAM, s->connection->log, 0,
                   "init first_available peer");

    if (ngx_stream_upstream_init_round_robin_peer(s, us) != NGX_OK) {
        return NGX_ERROR;
    }

    s->upstream->peer.get = ngx_stream_upstream_get_first_available_peer;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_upstream_get_first_available_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_stream_upstream_rr_peer_data_t *rrp = data;

    time_t                           now;
    uintptr_t                        m;
    ngx_int_t                        rc;
    ngx_uint_t                       i, n, p;
    ngx_stream_upstream_rr_peer_t   *peer, *best;
    ngx_stream_upstream_rr_peers_t  *peers;

    ngx_log_debug1(NGX_LOG_DEBUG_STREAM, pc->log, 0,
                   "get first_available peer, try: %ui", pc->tries);

    if (rrp->peers->single) {
        return ngx_stream_upstream_get_round_robin_peer(pc, rrp);
    }

    pc->connection = NULL;

    now = ngx_time();

    peers = rrp->peers;

    ngx_stream_upstream_rr_peers_wlock(peers);

    best = NULL;

#if (NGX_SUPPRESS_WARN)
    p = 0;
#endif

    for (peer = peers->peer, i = 0;
         peer;
         peer = peer->next, i++)
    {

        n = i / (8 * sizeof(uintptr_t));
        m = (uintptr_t) 1 << i % (8 * sizeof(uintptr_t));

        if (rrp->tried[n] & m) {
            continue;
        }

        if (peer->down) {
            continue;
        }

        if (peer->max_fails
            && peer->fails >= peer->max_fails
            && now - peer->checked <= peer->fail_timeout)
        {
            continue;
        }

        /*
         * select first available
         */

        if (best == NULL) {
            best = peer;
            p = i;
        }
    }

    if (best == NULL) {
        ngx_log_debug0(NGX_LOG_DEBUG_STREAM, pc->log, 0,
                       "get first_available peer, no peer found");

        goto failed;
    }

    if (now - best->checked > best->fail_timeout) {
        best->checked = now;
    }

    pc->sockaddr = best->sockaddr;
    pc->socklen = best->socklen;
    pc->name = &best->name;

    best->conns++;

    rrp->current = best;

    n = p / (8 * sizeof(uintptr_t));
    m = (uintptr_t) 1 << p % (8 * sizeof(uintptr_t));

    rrp->tried[n] |= m;

    ngx_stream_upstream_rr_peers_unlock(peers);

    return NGX_OK;

failed:
    /* all peers failed, mark them as live for quick recovery */

    for (peer = peers->peer; peer; peer = peer->next) {
        peer->fails = 0;
    }

    ngx_stream_upstream_rr_peers_unlock(peers);

    pc->name = peers->name;

    return NGX_BUSY;
}


static char *
ngx_stream_upstream_first_available(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_stream_upstream_srv_conf_t  *uscf;

    uscf = ngx_stream_conf_get_module_srv_conf(cf, ngx_stream_upstream_module);

    if (uscf->peer.init_upstream) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "load balancing method redefined");
    }

    uscf->peer.init_upstream = ngx_stream_upstream_init_first_available;

    uscf->flags = NGX_STREAM_UPSTREAM_CREATE
                  |NGX_STREAM_UPSTREAM_MAX_FAILS
                  |NGX_STREAM_UPSTREAM_FAIL_TIMEOUT
                  |NGX_STREAM_UPSTREAM_DOWN;

    return NGX_CONF_OK;
}
