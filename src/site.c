/**
 *
 * Glewlwyd OAuth2 Authorization Server
 *
 * OAuth2 authentiation server
 * Users are authenticated with a LDAP server
 * or users stored in the database 
 * Provides Json Web Tokens (jwt)
 * 
 * site CRUD services
 *
 * Copyright 2016-2017 Nicolas Mora <mail@babelouest.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU GENERAL PUBLIC LICENSE
 * License as published by the Free Software Foundation;
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU GENERAL PUBLIC LICENSE for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include "glewlwyd.h"

/**
 * Get the list of all sites
 */
json_t * get_site_list(struct config_elements * config) {
  json_t * j_query, * j_result, * j_return;
  int res;
  
  j_query = json_pack("{sss[ss]}",
                      "table",
                      GLEWLWYD_TABLE_SITE,
                      "columns",
                        "gs_name AS name",
                        "gs_description AS description");
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    j_return = json_pack("{siso}", "result", G_OK, "site", j_result);
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "get_site_list error getting site list");
    j_return = json_pack("{si}", "result", G_ERROR_DB);
  }
  return j_return;
}

/**
 * Get a specific site
 */
json_t * get_site(struct config_elements * config, const char * site) {
  json_t * j_query, * j_result, * j_return;
  int res;
  
  j_query = json_pack("{sss[ss]s{ss}}",
                      "table",
                      GLEWLWYD_TABLE_SITE,
                      "columns",
                        "gs_name AS name",
                        "gs_description AS description",
                      "where",
                        "gs_name",
                        site);
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    if (json_array_size(j_result) > 0) {
      j_return = json_pack("{sisO}", "result", G_OK, "site", json_array_get(j_result, 0));
    } else {
      j_return = json_pack("{si}", "result", G_ERROR_NOT_FOUND);
    }
    json_decref(j_result);
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "get_site error getting scoipe list");
    j_return = json_pack("{si}", "result", G_ERROR_DB);
  }
  return j_return;
}

/**
 * Check if the site has valid parameters
 */
json_t * is_site_valid(struct config_elements * config, json_t * j_site, int add) {
  json_t * j_return = json_array(), * j_query, * j_result;
  int res;
  
  if (j_return != NULL) {
    if (json_is_object(j_site)) {
      if (add) {
        if (json_is_string(json_object_get(j_site, "name"))) {
          j_query = json_pack("{sss{ss}}",
                              "table",
                              GLEWLWYD_TABLE_SITE,
                              "where",
                                "gs_name",
                                json_string_value(json_object_get(j_site, "name")));
          res = h_select(config->conn, j_query, &j_result, NULL);
          json_decref(j_query);
          if (res == H_OK) {
            if (json_array_size(j_result) > 0) {
              json_array_append_new(j_return, json_pack("{ss}", "name", "site name alread exist"));
            }
            json_decref(j_result);
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "is_site_valid - Error executing j_query");
          }
        }
        
        if (!json_is_string(json_object_get(j_site, "name")) || json_string_length(json_object_get(j_site, "name")) == 0 || json_string_length(json_object_get(j_site, "name")) > 128 || strchr(json_string_value(json_object_get(j_site, "name")), ' ') != NULL) {
          json_array_append_new(j_return, json_pack("{ss}", "name", "site name must be a non empty string of maximum 128 characters, without space characters"));
        }
        if (json_object_get(j_site, "description") != NULL && (!json_is_string(json_object_get(j_site, "description")) || json_string_length(json_object_get(j_site, "description")) > 512)) {
          json_array_append_new(j_return, json_pack("{ss}", "description", "site description is optional and must be a string of maximum 512 characters"));
        }
      } else {
        if (json_object_get(j_site, "description") == NULL || !json_is_string(json_object_get(j_site, "description")) || json_string_length(json_object_get(j_site, "description")) > 512) {
          json_array_append_new(j_return, json_pack("{ss}", "description", "site description is mandatory and must be a string of maximum 512 characters"));
        }
      }
    } else {
      json_array_append_new(j_return, json_pack("{ss}", "site", "site must be a json object"));
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "is_site_valid - Error allocating resources for j_result");
  }
  return j_return;
}

/**
 * Add a new site
 */
int add_site(struct config_elements * config, json_t * j_site) {
  json_t * j_query;
  int res;
  
  j_query = json_pack("{sss{ss}}",
                      "table",
                      GLEWLWYD_TABLE_SITE,
                      "values",
                        "gs_name",
                        json_string_value(json_object_get(j_site, "name")));
  if (json_object_get(j_site, "description") != NULL) {
    json_object_set(json_object_get(j_query, "values"), "gs_description", json_object_get(j_site, "description"));
  }
  
  res = h_insert(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return G_OK;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "add_site - Error executing j_query");
    return G_ERROR_DB;
  }
}

/**
 * Updates an exising site
 */
int set_site(struct config_elements * config, const char * site, json_t * j_site) {
  json_t * j_query;
  int res;
  
  j_query = json_pack("{sss{ss}s{ss}}",
                      "table",
                      GLEWLWYD_TABLE_SITE,
                      "set",
                        "gs_description",
                        json_object_get(j_site, "description")!=NULL?json_string_value(json_object_get(j_site, "description")):"",
                      "where",
                        "gs_name",
                        site);
  
  res = h_update(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return G_OK;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "set_site - Error executing j_query");
    return G_ERROR_DB;
  }
}

/**
 * Delete an existing site
 */
int delete_site(struct config_elements * config, const char * site) {
  json_t * j_query;
  int res;
  
  j_query = json_pack("{sss{ss}}",
                      "table",
                      GLEWLWYD_TABLE_SITE,
                      "where",
                        "gs_name",
                        site);
  
  res = h_delete(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return G_OK;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "set_site - Error executing j_query");
    return G_ERROR_DB;
  }
}
