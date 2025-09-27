/**
 * Copyright 2019-2020 DigitalOcean Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "promhttp.h"

#include <string.h>

#include "microhttpd.h"
#include "prom.h"

prom_collector_registry_t *PROM_ACTIVE_REGISTRY;

// Callbacks de sincronización (opcionales, seteados por la aplicación)
static promhttp_lock_fn g_lock_cb = NULL;
static promhttp_lock_fn g_unlock_cb = NULL;
static void *g_lock_user = NULL;

void promhttp_set_lock_callbacks(promhttp_lock_fn lock_cb, promhttp_lock_fn unlock_cb, void *user) {
  g_lock_cb = lock_cb;
  g_unlock_cb = unlock_cb;
  g_lock_user = user;
}

void promhttp_set_active_collector_registry(prom_collector_registry_t *active_registry) {
  if (!active_registry) {
    PROM_ACTIVE_REGISTRY = PROM_COLLECTOR_REGISTRY_DEFAULT;
  } else {
    PROM_ACTIVE_REGISTRY = active_registry;
  }
}

enum MHD_Result promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                                 const char *version, const char *upload_data, size_t *upload_data_size,
                                 void **con_cls) {
  if (strcmp(method, "GET") != 0) {
    char *buf = "Invalid HTTP Method\n";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    MHD_destroy_response(response);
    return ret;
  }
  if (strcmp(url, "/metrics") == 0) {
    // Bloqueo antes de leer métricas
    if (g_lock_cb) g_lock_cb(g_lock_user);

    if (!PROM_ACTIVE_REGISTRY) {
      if (g_unlock_cb) g_unlock_cb(g_lock_user);
      const char *err = "Error: no active registry (PROM_ACTIVE_REGISTRY == NULL)\n";
      struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
      enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, resp);
      MHD_destroy_response(resp);
      return ret;
    }

    const char *buf = prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);

    if (g_unlock_cb) g_unlock_cb(g_lock_user);

    if (!buf) {
      const char *err = "Error: prom_collector_registry_bridge returned NULL\n";
      struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
      enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
      MHD_destroy_response(resp);
      return ret;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_MUST_FREE);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
  }

  // Si llegamos aquí, la URL no es válida
  const char *buf = "Bad Request\n";
  struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_PERSISTENT);
  enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, resp);
  MHD_destroy_response(resp);
  return ret;
}

struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
                                         void *apc_cls) {
  return MHD_start_daemon(flags, port, apc, apc_cls, &promhttp_handler, NULL, MHD_OPTION_END);
}
