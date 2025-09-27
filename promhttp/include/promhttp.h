/*
 Copyright 2019-2020 DigitalOcean Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/**
 * @file promhttp.h
 * @brief Provides a HTTP endpoint for metric exposition
 * References:
 *   * MHD_FLAG: https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dconst
 *   * MHD_AcceptPolicyCallback:
 * https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#index-_002aMHD_005fAcceptPolicyCallback
 */

#include <string.h>

#include "microhttpd.h"
#include "prom_collector_registry.h"

/**
 * @brief Sets the active registry for metric scraping.
 *
 * @param active_registery The target prom_collector_registry_t*. If null is passed, the default registry is used.
 *                         The registry MUST be initialized.
 */
void promhttp_set_active_collector_registry(prom_collector_registry_t *active_registry);

/**
 *  @brief Starts a daemon in the background and returns a pointer to an HMD_Daemon.
 *
 * References:
 *  * https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dinit
 *
 * @return struct MHD_Daemon*
 */
struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
                                         void *apc_cls);

/**
 * @brief Tipo de callback para funciones de lock y unlock.
 *
 * La aplicación puede registrar funciones de bloqueo/desbloqueo (por ejemplo,
 * wrappers sobre pthread_mutex_lock/unlock). Estas se invocarán automáticamente
 * antes y después de acceder al registro de métricas en el handler HTTP.
 */
typedef void (*promhttp_lock_fn)(void *user);

/**
 * @brief Registra callbacks de lock/unlock opcionales para proteger el acceso concurrente al registry.
 *
 * @param lock_cb   Función a ejecutar antes de leer métricas (puede ser NULL).
 * @param unlock_cb Función a ejecutar después de leer métricas (puede ser NULL).
 * @param user      Puntero opaco que se pasará a lock_cb/unlock_cb.
 */
void promhttp_set_lock_callbacks(promhttp_lock_fn lock_cb, promhttp_lock_fn unlock_cb, void *user);
