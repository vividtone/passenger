/*
 *  Phusion Passenger - http://www.modrails.com/
 *  Copyright (c) 2010 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#include <algorithm>
#include <cstdlib>

/* ap_config.h checks whether the compiler has support for C99's designated
 * initializers, and defines AP_HAVE_DESIGNATED_INITIALIZER if it does. However,
 * g++ does not support designated initializers, even when ap_config.h thinks
 * it does. Here we undefine the macro to force httpd_config.h to not use
 * designated initializers. This should fix compilation problems on some systems.
 */
#include <ap_config.h>
#undef AP_HAVE_DESIGNATED_INITIALIZER

#include "Configuration.h"
#include "Utils.h"

/* The APR headers must come after the Passenger headers. See Hooks.cpp
 * to learn why.
 */
#include <apr_strings.h>

using namespace Passenger;

extern "C" module AP_MODULE_DECLARE_DATA passenger_module;

#define MERGE_THREEWAY_CONFIG(field) \
	config->field = (add->field == DirConfig::UNSET) ? base->field : add->field
#define MERGE_STR_CONFIG(field) \
	config->field = (add->field == NULL) ? base->field : add->field
#define MERGE_STRING_CONFIG(field) \
	config->field = (add->field.empty()) ? base->field : add->field

template<typename T> static apr_status_t
destroy_config_struct(void *x) {
	delete (T *) x;
	return APR_SUCCESS;
}

static DirConfig *
create_dir_config_struct(apr_pool_t *pool) {
	DirConfig *config = new DirConfig();
	apr_pool_cleanup_register(pool, config, destroy_config_struct<DirConfig>, apr_pool_cleanup_null);
	return config;
}

static ServerConfig *
create_server_config_struct(apr_pool_t *pool) {
	ServerConfig *config = new ServerConfig();
	apr_pool_cleanup_register(pool, config, destroy_config_struct<ServerConfig>, apr_pool_cleanup_null);
	return config;
}

extern "C" {

void *
passenger_config_create_dir(apr_pool_t *p, char *dirspec) {
	DirConfig *config = create_dir_config_struct(p);
	config->enabled = DirConfig::UNSET;
	config->autoDetectRails = DirConfig::UNSET;
	config->autoDetectRack = DirConfig::UNSET;
	config->autoDetectWSGI = DirConfig::UNSET;
	config->railsEnv = NULL;
	config->rackEnv = NULL;
	config->appRoot = NULL;
	config->spawnMethod = DirConfig::SM_UNSET;
	config->frameworkSpawnerTimeout = -1;
	config->appSpawnerTimeout = -1;
	config->maxRequests = 0;
	config->maxRequestsSpecified = false;
	config->minInstances = 0;
	config->minInstancesSpecified = false;
	config->highPerformance = DirConfig::UNSET;
	config->useGlobalQueue = DirConfig::UNSET;
	config->resolveSymlinksInDocRoot = DirConfig::UNSET;
	config->allowEncodedSlashes = DirConfig::UNSET;
	config->statThrottleRate = 0;
	config->statThrottleRateSpecified = false;
	config->restartDir = NULL;
	config->uploadBufferDir = NULL;
	config->friendlyErrorPages = DirConfig::UNSET;
	/*************************************/
	return config;
}

void *
passenger_config_merge_dir(apr_pool_t *p, void *basev, void *addv) {
	DirConfig *config = create_dir_config_struct(p);
	DirConfig *base = (DirConfig *) basev;
	DirConfig *add = (DirConfig *) addv;
	
	config->enabled = (add->enabled == DirConfig::UNSET) ? base->enabled : add->enabled;
	
	config->railsBaseURIs = base->railsBaseURIs;
	for (set<string>::const_iterator it(add->railsBaseURIs.begin()); it != add->railsBaseURIs.end(); it++) {
		config->railsBaseURIs.insert(*it);
	}
	config->rackBaseURIs = base->rackBaseURIs;
	for (set<string>::const_iterator it(add->rackBaseURIs.begin()); it != add->rackBaseURIs.end(); it++) {
		config->rackBaseURIs.insert(*it);
	}
	
	MERGE_THREEWAY_CONFIG(autoDetectRails);
	MERGE_THREEWAY_CONFIG(autoDetectRack);
	MERGE_THREEWAY_CONFIG(autoDetectWSGI);
	MERGE_STR_CONFIG(railsEnv);
	MERGE_STR_CONFIG(rackEnv);
	MERGE_STR_CONFIG(appRoot);
	config->spawnMethod = (add->spawnMethod == DirConfig::SM_UNSET) ? base->spawnMethod : add->spawnMethod;
	config->frameworkSpawnerTimeout = (add->frameworkSpawnerTimeout == -1) ? base->frameworkSpawnerTimeout : add->frameworkSpawnerTimeout;
	config->appSpawnerTimeout = (add->appSpawnerTimeout == -1) ? base->appSpawnerTimeout : add->appSpawnerTimeout;
	config->maxRequests = (add->maxRequestsSpecified) ? add->maxRequests : base->maxRequests;
	config->maxRequestsSpecified = base->maxRequestsSpecified || add->maxRequestsSpecified;
	config->minInstances = (add->minInstancesSpecified) ? add->minInstances : base->minInstances;
	config->minInstancesSpecified = base->minInstancesSpecified || add->minInstancesSpecified;
	MERGE_THREEWAY_CONFIG(highPerformance);
	MERGE_THREEWAY_CONFIG(useGlobalQueue);
	config->statThrottleRate = (add->statThrottleRateSpecified) ? add->statThrottleRate : base->statThrottleRate;
	config->statThrottleRateSpecified = base->statThrottleRateSpecified || add->statThrottleRateSpecified;
	MERGE_STR_CONFIG(restartDir);
	MERGE_STR_CONFIG(uploadBufferDir);
	MERGE_THREEWAY_CONFIG(resolveSymlinksInDocRoot);
	MERGE_THREEWAY_CONFIG(allowEncodedSlashes);
	MERGE_THREEWAY_CONFIG(friendlyErrorPages);
	/*************************************/
	return config;
}

void *
passenger_config_create_server(apr_pool_t *p, server_rec *s) {
	ServerConfig *config = create_server_config_struct(p);
	config->ruby = NULL;
	config->root = NULL;
	config->logLevel = DEFAULT_LOG_LEVEL;
	config->maxPoolSize = DEFAULT_MAX_POOL_SIZE;
	config->maxPoolSizeSpecified = false;
	config->maxInstancesPerApp = DEFAULT_MAX_INSTANCES_PER_APP;
	config->maxInstancesPerAppSpecified = false;
	config->poolIdleTime = DEFAULT_POOL_IDLE_TIME;
	config->poolIdleTimeSpecified = false;
	config->userSwitching = true;
	config->userSwitchingSpecified = false;
	config->defaultUser = NULL;
	config->tempDir = NULL;
	config->analyticsLogUser = NULL;
	config->analyticsLogGroup = NULL;
	config->analyticsLogPermissions = NULL;
	return config;
}

void *
passenger_config_merge_server(apr_pool_t *p, void *basev, void *addv) {
	ServerConfig *config = create_server_config_struct(p);
	ServerConfig *base = (ServerConfig *) basev;
	ServerConfig *add = (ServerConfig *) addv;
	
	MERGE_STR_CONFIG(ruby);
	MERGE_STR_CONFIG(root);
	config->logLevel = (add->logLevel) ? base->logLevel : add->logLevel;
	config->maxPoolSize = (add->maxPoolSizeSpecified) ? base->maxPoolSize : add->maxPoolSize;
	config->maxPoolSizeSpecified = base->maxPoolSizeSpecified || add->maxPoolSizeSpecified;
	config->maxInstancesPerApp = (add->maxInstancesPerAppSpecified) ? base->maxInstancesPerApp : add->maxInstancesPerApp;
	config->maxInstancesPerAppSpecified = base->maxInstancesPerAppSpecified || add->maxInstancesPerAppSpecified;
	config->poolIdleTime = (add->poolIdleTime) ? base->poolIdleTime : add->poolIdleTime;
	config->poolIdleTimeSpecified = base->poolIdleTimeSpecified || add->poolIdleTimeSpecified;
	config->userSwitching = (add->userSwitchingSpecified) ? add->userSwitching : base->userSwitching;
	config->userSwitchingSpecified = base->userSwitchingSpecified || add->userSwitchingSpecified;
	config->defaultUser = (add->defaultUser == NULL) ? base->defaultUser : add->defaultUser;
	MERGE_STR_CONFIG(defaultUser);
	MERGE_STR_CONFIG(tempDir);
	MERGE_STRING_CONFIG(analyticsLogDir);
	MERGE_STR_CONFIG(analyticsLogUser);
	MERGE_STR_CONFIG(analyticsLogGroup);
	MERGE_STR_CONFIG(analyticsLogPermissions);
	return config;
}

void
passenger_config_merge_all_servers(apr_pool_t *pool, server_rec *main_server) {
	ServerConfig *final = (ServerConfig *) passenger_config_create_server(pool, main_server);
	server_rec *s;
	
	#define MERGE_SERVER_STR_CONFIGS(field) \
		final->field = (final->field != NULL) ? final->field : config->field
	#define MERGE_SERVER_STRING_CONFIGS(field) \
		final->field = (!final->field.empty()) ? final->field : config->field
	
	for (s = main_server; s != NULL; s = s->next) {
		ServerConfig *config = (ServerConfig *) ap_get_module_config(s->module_config, &passenger_module);
		MERGE_SERVER_STR_CONFIGS(ruby);
		MERGE_SERVER_STR_CONFIGS(root);
		final->logLevel = (final->logLevel != 0) ? final->logLevel : config->logLevel;
		final->maxPoolSize = (final->maxPoolSizeSpecified) ? final->maxPoolSize : config->maxPoolSize;
		final->maxPoolSizeSpecified = final->maxPoolSizeSpecified || config->maxPoolSizeSpecified;
		final->maxInstancesPerApp = (final->maxInstancesPerAppSpecified) ? final->maxInstancesPerApp : config->maxInstancesPerApp;
		final->maxInstancesPerAppSpecified = final->maxInstancesPerAppSpecified || config->maxInstancesPerAppSpecified;
		final->poolIdleTime = (final->poolIdleTimeSpecified) ? final->poolIdleTime : config->poolIdleTime;
		final->poolIdleTimeSpecified = final->poolIdleTimeSpecified || config->poolIdleTimeSpecified;
		final->userSwitching = (config->userSwitchingSpecified) ? config->userSwitching : final->userSwitching;
		final->userSwitchingSpecified = final->userSwitchingSpecified || config->userSwitchingSpecified;
		MERGE_SERVER_STR_CONFIGS(defaultUser);
		MERGE_SERVER_STR_CONFIGS(tempDir);
		MERGE_SERVER_STRING_CONFIGS(analyticsLogDir);
		MERGE_SERVER_STR_CONFIGS(analyticsLogUser);
		MERGE_SERVER_STR_CONFIGS(analyticsLogGroup);
		MERGE_SERVER_STR_CONFIGS(analyticsLogPermissions);
	}
	final->finalize();
	for (s = main_server; s != NULL; s = s->next) {
		ServerConfig *config = (ServerConfig *) ap_get_module_config(s->module_config, &passenger_module);
		*config = *final;
	}
}


/*************************************************
 * Passenger settings
 *************************************************/

static const char *
cmd_passenger_root(cmd_parms *cmd, void *pcfg, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	config->root = arg;
	return NULL;
}

static const char *
cmd_passenger_log_level(cmd_parms *cmd, void *pcfg, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerLogLevel.";
	} else if (result < 0 || result > 9) {
		return "Value for PassengerLogLevel must be between 0 and 9.";
	} else {
		config->logLevel = (unsigned int) result;
		return NULL;
	}
}

static const char *
cmd_passenger_ruby(cmd_parms *cmd, void *pcfg, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	config->ruby = arg;
	return NULL;
}

static const char *
cmd_passenger_max_pool_size(cmd_parms *cmd, void *pcfg, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerMaxPoolSize.";
	} else if (result <= 0) {
		return "Value for PassengerMaxPoolSize must be greater than 0.";
	} else {
		config->maxPoolSize = (unsigned int) result;
		config->maxPoolSizeSpecified = true;
		return NULL;
	}
}

static const char *
cmd_passenger_min_instances(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerMinInstances.";
	} else if (result < 0) {
		return "Value for PassengerMinInstances must be greater than or equal to 0.";
	} else {
		config->minInstances = (unsigned long) result;
		config->minInstancesSpecified = true;
		return NULL;
	}
}

static const char *
cmd_passenger_max_instances_per_app(cmd_parms *cmd, void *pcfg, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerMaxInstancesPerApp.";
	} else {
		config->maxInstancesPerApp = (unsigned int) result;
		config->maxInstancesPerAppSpecified = true;
		return NULL;
	}
}

static const char *
cmd_passenger_pool_idle_time(cmd_parms *cmd, void *pcfg, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerPoolIdleTime.";
	} else if (result < 0) {
		return "Value for PassengerPoolIdleTime must be greater than or equal to 0.";
	} else {
		config->poolIdleTime = (unsigned int) result;
		config->poolIdleTimeSpecified = true;
		return NULL;
	}
}

static const char *
cmd_passenger_use_global_queue(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	if (arg) {
		config->useGlobalQueue = DirConfig::ENABLED;
	} else {
		config->useGlobalQueue = DirConfig::DISABLED;
	}
	return NULL;
}

static const char *
cmd_passenger_user_switching(cmd_parms *cmd, void *pcfg, int arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	config->userSwitching = arg;
	config->userSwitchingSpecified = true;
	return NULL;
}

static const char *
cmd_passenger_default_user(cmd_parms *cmd, void *dummy, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	config->defaultUser = arg;
	return NULL;
}

static const char *
cmd_passenger_temp_dir(cmd_parms *cmd, void *dummy, const char *arg) {
	ServerConfig *config = (ServerConfig *) ap_get_module_config(
		cmd->server->module_config, &passenger_module);
	config->tempDir = arg;
	return NULL;
}

static const char *
cmd_passenger_max_requests(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerMaxRequests.";
	} else if (result < 0) {
		return "Value for PassengerMaxRequests must be greater than or equal to 0.";
	} else {
		config->maxRequests = (unsigned long) result;
		config->maxRequestsSpecified = true;
		return NULL;
	}
}

static const char *
cmd_passenger_high_performance(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	if (arg) {
		config->highPerformance = DirConfig::ENABLED;
	} else {
		config->highPerformance = DirConfig::DISABLED;
	}
	return NULL;
}

static const char *
cmd_passenger_enabled(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	if (arg) {
		config->enabled = DirConfig::ENABLED;
	} else {
		config->enabled = DirConfig::DISABLED;
	}
	return NULL;
}

static const char *
cmd_passenger_stat_throttle_rate(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for PassengerStatThrottleRate.";
	} else if (result < 0) {
		return "Value for PassengerStatThrottleRate must be greater than or equal to 0.";
	} else {
		config->statThrottleRate = (unsigned long) result;
		config->statThrottleRateSpecified = true;
		return NULL;
	}
}

static const char *
cmd_passenger_restart_dir(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->restartDir = arg;
	return NULL;
}

static const char *
cmd_passenger_app_root(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->appRoot = arg;
	return NULL;
}

static const char *
cmd_passenger_upload_buffer_dir(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->uploadBufferDir = arg;
	return NULL;
}

static const char *
cmd_passenger_resolve_symlinks_in_document_root(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->resolveSymlinksInDocRoot = (arg) ? DirConfig::ENABLED : DirConfig::DISABLED;
	return NULL;
}

static const char *
cmd_passenger_allow_encoded_slashes(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->allowEncodedSlashes = (arg) ? DirConfig::ENABLED : DirConfig::DISABLED;
	return NULL;
}

static const char *
cmd_passenger_friendly_error_pages(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->friendlyErrorPages = (arg) ? DirConfig::ENABLED : DirConfig::DISABLED;
	return NULL;
}

static const char *
cmd_passenger_spawn_method(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	if (strcmp(arg, "smart") == 0) {
		config->spawnMethod = DirConfig::SM_SMART;
	} else if (strcmp(arg, "smart-lv2") == 0) {
		config->spawnMethod = DirConfig::SM_SMART_LV2;
	} else if (strcmp(arg, "conservative") == 0) {
		config->spawnMethod = DirConfig::SM_CONSERVATIVE;
	} else {
		return "PassengerSpawnMethod may only be 'smart', 'smart-lv2' or 'conservative'.";
	}
	return NULL;
}


/*************************************************
 * Rails-specific settings
 *************************************************/

static const char *
cmd_rails_base_uri(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	if (strlen(arg) == 0) {
		return "RailsBaseURI may not be set to the empty string";
	} else if (arg[0] != '/') {
		return "RailsBaseURI must start with a slash (/)";
	} else if (strlen(arg) > 1 && arg[strlen(arg) - 1] == '/') {
		return "RailsBaseURI must not end with a slash (/)";
	} else {
		config->railsBaseURIs.insert(arg);
		return NULL;
	}
}

static const char *
cmd_rails_auto_detect(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->autoDetectRails = (arg) ? DirConfig::ENABLED : DirConfig::DISABLED;
	return NULL;
}

static const char *
cmd_rails_env(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->railsEnv = arg;
	return NULL;
}

static const char *
cmd_rails_framework_spawner_idle_time(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for RailsFrameworkSpawnerIdleTime.";
	} else if (result < 0) {
		return "Value for RailsFrameworkSpawnerIdleTime must be at least 0.";
	} else {
		config->frameworkSpawnerTimeout = result;
		return NULL;
	}
}

static const char *
cmd_rails_app_spawner_idle_time(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	char *end;
	long int result;
	
	result = strtol(arg, &end, 10);
	if (*end != '\0') {
		return "Invalid number specified for RailsAppSpawnerIdleTime.";
	} else if (result < 0) {
		return "Value for RailsAppSpawnerIdleTime must be at least 0.";
	} else {
		config->appSpawnerTimeout = result;
		return NULL;
	}
}


/*************************************************
 * Rack-specific settings
 *************************************************/

static const char *
cmd_rack_base_uri(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	if (strlen(arg) == 0) {
		return "RackBaseURI may not be set to the empty string";
	} else if (arg[0] != '/') {
		return "RackBaseURI must start with a slash (/)";
	} else if (strlen(arg) > 1 && arg[strlen(arg) - 1] == '/') {
		return "RackBaseURI must not end with a slash (/)";
	} else {
		config->rackBaseURIs.insert(arg);
		return NULL;
	}
}

static const char *
cmd_rack_auto_detect(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->autoDetectRack = (arg) ? DirConfig::ENABLED : DirConfig::DISABLED;
	return NULL;
}

static const char *
cmd_rack_env(cmd_parms *cmd, void *pcfg, const char *arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->rackEnv = arg;
	return NULL;
}


/*************************************************
 * WSGI-specific settings
 *************************************************/

static const char *
cmd_wsgi_auto_detect(cmd_parms *cmd, void *pcfg, int arg) {
	DirConfig *config = (DirConfig *) pcfg;
	config->autoDetectWSGI = (arg) ? DirConfig::ENABLED : DirConfig::DISABLED;
	return NULL;
}


/*************************************************
 * Obsolete settings
 *************************************************/

static const char *
cmd_rails_spawn_server(cmd_parms *cmd, void *pcfg, const char *arg) {
	fprintf(stderr, "WARNING: The 'RailsSpawnServer' option is obsolete. "
		"Please specify 'PassengerRoot' instead. The correct value was "
		"given to you by 'passenger-install-apache2-module'.");
	fflush(stderr);
	return NULL;
}

static const char *
cmd_rails_allow_mod_rewrite(cmd_parms *cmd, void *pcfg, int arg) {
	fprintf(stderr, "WARNING: The 'RailsAllowModRewrite' option is obsolete: "
		"Phusion Passenger now fully supports mod_rewrite. "
		"Please remove this option from your configuration file.");
	fflush(stderr);
	return NULL;
}


typedef const char * (*Take1Func)();
typedef const char * (*FlagFunc)();

const command_rec passenger_commands[] = {
	// Passenger settings.
	AP_INIT_TAKE1("PassengerRoot",
		(Take1Func) cmd_passenger_root,
		NULL,
		RSRC_CONF,
		"The Passenger root folder."),
	AP_INIT_TAKE1("PassengerLogLevel",
		(Take1Func) cmd_passenger_log_level,
		NULL,
		RSRC_CONF,
		"Passenger log verbosity."),
	AP_INIT_TAKE1("PassengerRuby",
		(Take1Func) cmd_passenger_ruby,
		NULL,
		RSRC_CONF,
		"The Ruby interpreter to use."),
	AP_INIT_TAKE1("PassengerMaxPoolSize",
		(Take1Func) cmd_passenger_max_pool_size,
		NULL,
		RSRC_CONF,
		"The maximum number of simultaneously alive application instances."),
	AP_INIT_TAKE1("PassengerMinInstances",
		(Take1Func) cmd_passenger_min_instances,
		NULL,
		OR_LIMIT | ACCESS_CONF | RSRC_CONF,
		"The minimum number of application instances to keep when cleaning idle instances."),
	AP_INIT_TAKE1("PassengerMaxInstancesPerApp",
		(Take1Func) cmd_passenger_max_instances_per_app,
		NULL,
		RSRC_CONF,
		"The maximum number of simultaneously alive application instances a single application may occupy."),
	AP_INIT_TAKE1("PassengerPoolIdleTime",
		(Take1Func) cmd_passenger_pool_idle_time,
		NULL,
		RSRC_CONF,
		"The maximum number of seconds that an application may be idle before it gets terminated."),
	AP_INIT_FLAG("PassengerUseGlobalQueue",
		(FlagFunc) cmd_passenger_use_global_queue,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"Enable or disable Passenger's global queuing mode mode."),
	AP_INIT_FLAG("PassengerUserSwitching",
		(FlagFunc) cmd_passenger_user_switching,
		NULL,
		RSRC_CONF,
		"Whether to enable user switching support."),
	AP_INIT_TAKE1("PassengerDefaultUser",
		(Take1Func) cmd_passenger_default_user,
		NULL,
		RSRC_CONF,
		"The user that Rails/Rack applications must run as when user switching fails or is disabled."),
	AP_INIT_TAKE1("PassengerTempDir",
		(Take1Func) cmd_passenger_temp_dir,
		NULL,
		RSRC_CONF,
		"The temp directory that Passenger should use."),
	AP_INIT_TAKE1("PassengerMaxRequests",
		(Take1Func) cmd_passenger_max_requests,
		NULL,
		OR_LIMIT | ACCESS_CONF | RSRC_CONF,
		"The maximum number of requests that an application instance may process."),
	AP_INIT_FLAG("PassengerHighPerformance",
		(FlagFunc) cmd_passenger_high_performance,
		NULL,
		OR_ALL,
		"Enable or disable Passenger's high performance mode."),
	AP_INIT_FLAG("PassengerEnabled",
		(FlagFunc) cmd_passenger_enabled,
		NULL,
		OR_ALL,
		"Enable or disable Phusion Passenger."),
	AP_INIT_TAKE1("PassengerStatThrottleRate",
		(Take1Func) cmd_passenger_stat_throttle_rate,
		NULL,
		OR_LIMIT | ACCESS_CONF | RSRC_CONF,
		"Limit the number of stat calls to once per given seconds."),
	AP_INIT_TAKE1("PassengerRestartDir",
		(Take1Func) cmd_passenger_restart_dir,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"The directory in which Passenger should look for restart.txt."),
	AP_INIT_TAKE1("PassengerAppRoot",
		(Take1Func) cmd_passenger_app_root,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"The application's root directory."),
	AP_INIT_TAKE1("PassengerUploadBufferDir",
		(Take1Func) cmd_passenger_upload_buffer_dir,
		NULL,
		OR_OPTIONS,
		"The directory in which upload buffer files should be placed."),
	AP_INIT_FLAG("PassengerResolveSymlinksInDocumentRoot",
		(FlagFunc) cmd_passenger_resolve_symlinks_in_document_root,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"Whether to resolve symlinks in the DocumentRoot path"),
	AP_INIT_FLAG("PassengerAllowEncodedSlashes",
		(FlagFunc) cmd_passenger_allow_encoded_slashes,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"Whether to support encoded slashes in the URL"),
	AP_INIT_FLAG("PassengerFriendlyErrorPages",
		(FlagFunc) cmd_passenger_friendly_error_pages,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"Whether to display friendly error pages when something goes wrong"),
	AP_INIT_TAKE1("PassengerSpawnMethod",
		(Take1Func) cmd_passenger_spawn_method,
		NULL,
		RSRC_CONF,
		"The spawn method to use."),
	
	/*****************************/

	// Rails-specific settings.
	AP_INIT_TAKE1("RailsBaseURI",
		(Take1Func) cmd_rails_base_uri,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"Reserve the given URI to a Rails application."),
	AP_INIT_FLAG("RailsAutoDetect",
		(FlagFunc) cmd_rails_auto_detect,
		NULL,
		RSRC_CONF,
		"Whether auto-detection of Ruby on Rails applications should be enabled."),
	AP_INIT_TAKE1("RailsEnv",
		(Take1Func) cmd_rails_env,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"The environment under which a Rails app must run."),
	AP_INIT_TAKE1("RailsFrameworkSpawnerIdleTime",
		(Take1Func) cmd_rails_framework_spawner_idle_time,
		NULL,
		RSRC_CONF,
		"The maximum number of seconds that a framework spawner may be idle before it is shutdown."),
	AP_INIT_TAKE1("RailsAppSpawnerIdleTime",
		(Take1Func) cmd_rails_app_spawner_idle_time,
		NULL,
		RSRC_CONF,
		"The maximum number of seconds that an application spawner may be idle before it is shutdown."),
	
	// Rack-specific settings.
	AP_INIT_TAKE1("RackBaseURI",
		(Take1Func) cmd_rack_base_uri,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"Reserve the given URI to a Rack application."),
	AP_INIT_FLAG("RackAutoDetect",
		(FlagFunc) cmd_rack_auto_detect,
		NULL,
		RSRC_CONF,
		"Whether auto-detection of Rack applications should be enabled."),
	AP_INIT_TAKE1("RackEnv",
		(Take1Func) cmd_rack_env,
		NULL,
		OR_OPTIONS | ACCESS_CONF | RSRC_CONF,
		"The environment under which a Rack app must run."),
	
	// WSGI-specific settings.
	AP_INIT_FLAG("PassengerWSGIAutoDetect",
		(FlagFunc) cmd_wsgi_auto_detect,
		NULL,
		RSRC_CONF,
		"Whether auto-detection of WSGI applications should be enabled."),
	
	// Backwards compatibility options.
	AP_INIT_TAKE1("RailsRuby",
		(Take1Func) cmd_passenger_ruby,
		NULL,
		RSRC_CONF,
		"Deprecated option."),
	AP_INIT_TAKE1("RailsMaxPoolSize",
		(Take1Func) cmd_passenger_max_pool_size,
		NULL,
		RSRC_CONF,
		"Deprecated option."),
	AP_INIT_TAKE1("RailsMaxInstancesPerApp",
		(Take1Func) cmd_passenger_max_instances_per_app,
		NULL,
		RSRC_CONF,
		"Deprecated option"),
	AP_INIT_TAKE1("RailsPoolIdleTime",
		(Take1Func) cmd_passenger_pool_idle_time,
		NULL,
		RSRC_CONF,
		"Deprecated option."),
	AP_INIT_FLAG("RailsUserSwitching",
		(FlagFunc) cmd_passenger_user_switching,
		NULL,
		RSRC_CONF,
		"Deprecated option."),
	AP_INIT_TAKE1("RailsDefaultUser",
		(Take1Func) cmd_passenger_default_user,
		NULL,
		RSRC_CONF,
		"Deprecated option."),
	AP_INIT_TAKE1("RailsSpawnMethod",
		(Take1Func) cmd_passenger_spawn_method,
		NULL,
		RSRC_CONF,
		"Deprecated option."),
	
	// Obsolete options.
	AP_INIT_TAKE1("RailsSpawnServer",
		(Take1Func) cmd_rails_spawn_server,
		NULL,
		RSRC_CONF,
		"Obsolete option."),
	AP_INIT_FLAG("RailsAllowModRewrite",
		(FlagFunc) cmd_rails_allow_mod_rewrite,
		NULL,
		RSRC_CONF,
		"Whether custom mod_rewrite rules should be allowed."),
	
	{ NULL }
};

} // extern "C"
