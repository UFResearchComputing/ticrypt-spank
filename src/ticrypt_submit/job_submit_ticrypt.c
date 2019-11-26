/* ************************************************************************** */
/*                                                                            */
/*                                DESCRIPTION                                 */
/*                                                                            */
/* ************************************************************************** */ 
/*
 
   ~~                TiCrypt job submit plugin for Slurm                   ~~

   Copyright (C) 2019 University of Florida - UFIT Research Computing

   Produced at The University of Florida 
               Gainesville, Florida

   Written by William Howell <whowell@rc.ufl.edu> with 
 
   This is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* ************************************************************************** */
/*                                                                            */
/*                                INCLUDES                                    */
/*                                                                            */
/* ************************************************************************** */ 
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <libconfig.h>
#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>
#include "../../slurm/src/common/slurm_xlator.h"
#include "../../slurm/src/slurmctld/slurmctld.h"

/* ************************************************************************** */
/*                                                                            */
/*                       REQUIRED JOB SUBMIT VARIABLES                        */
/*                                                                            */
/* ************************************************************************** */
const char plugin_name[]       	= "Job submit ticrypt plugin";
const char plugin_type[]       	= "job_submit/ticrypt";
const uint32_t plugin_version   = SLURM_VERSION_NUMBER;

/* ************************************************************************** */
/*                                                                            */
/*                                 DEFINES                                    */
/*                                                                            */
/* ************************************************************************** */
/* Standards */ 
#define BUFLEN   256
#define KEYLEN   9
/* Booleans */
#define TRUE     0
#define FALSE    1
/* Configuration Defaults */
#define DEFAULT_CONFIG "/etc/ticrypt-spank.conf"
#define OPTION_CHECK "_SLURM_SPANK_OPTION_ticrypt_ticrypt=(null)"

/* ************************************************************************** */
/*                                                                            */
/*                    ticrypt_settings_t and methods                          */
/*                                                                            */
/* ************************************************************************** */
typedef struct ticrypt_settings	 {
  char      *config_file;
  int       allow_drain;
  int       allow_requeue;
  char      **partitions;
  char      **accounts;
  char      **features;
  int       n_partitions;
  int       n_accounts;
  int       n_features;
} ticrypt_settings_t;

/* ****************************************************** */
/*              ticrypt_settings_free()                   */
/* - cleanup memory allocated to a ticrypt_settings_t     */
/* ****************************************************** */
int ticrypt_settings_free(ticrypt_settings_t *settings) {
  int i=0;
  /* config_file */
  if ( settings -> config_file != NULL ) {
    free(settings -> config_file);
  }
  /* partitions setting */
  if ( settings -> partitions != NULL ) {
    for ( i=0; i<settings -> n_partitions; i++ ) {
      if ( (settings -> partitions)[i] != NULL ) {
        free((settings -> partitions[i]));
      }
    }
    free(settings -> partitions);
  }
  /* accounts setting */
  if ( settings -> accounts != NULL ) {
    for ( i=0; i<settings -> n_accounts; i++ ) {
      if ( (settings -> accounts)[i] != NULL ) {
        free((settings -> accounts[i]));
      }
    }
    free(settings -> accounts);
  }
  /* features setting */
  if ( settings -> features != NULL ) {
    for ( i=0; i<settings -> n_features; i++ ) {
      if ( (settings -> features)[i] != NULL ) {
        free((settings -> features[i]));
      }
    }
    free(settings -> features);
  }

  return SLURM_SUCCESS;
}

/* ****************************************************** */
/*              ticrypt_settings_init()                   */
/* - allocate memory to members of a ticrypt_settings_t   */
/* - parse config into ticrypt_settings_t                 */
/* ****************************************************** */
int ticrypt_settings_init(ticrypt_settings_t *settings) {
  int index = 0;
  char message[BUFLEN];
  settings -> config_file = NULL;
  settings -> allow_drain = TRUE;
  settings -> allow_requeue = TRUE;
  settings -> accounts = NULL;
  settings -> partitions = NULL;
  settings -> features = NULL;
  settings -> n_partitions = 0;
  settings -> n_accounts = 0;
  settings -> n_features = 0;

  /* config_file */ 
  const char* env_config_file = getenv("TICRYPT_SPANK_CONFIG");
  if ( env_config_file != NULL ) {
    settings->config_file=(char *)malloc(strlen(env_config_file)*sizeof(char));
    sprintf(settings -> config_file,
            "%s",
            env_config_file);
  } else {
    settings->config_file=(char *)malloc(strlen(DEFAULT_CONFIG)*sizeof(char));
    sprintf(settings -> config_file,
            "%s",
            DEFAULT_CONFIG);
  }

  /* initialize and read the configuration */
  sprintf(message,"(%s) reading configuration from %s",
                  plugin_type,
                  settings -> config_file);
  debug(message);
  config_t config;
  config_init(&config);
  if(! config_read_file(&config,settings -> config_file)) {
    sprintf(message,"(%s) config error %s:%d: %s",
            plugin_type,
            config_error_file(&config),
            config_error_line(&config),
            config_error_text(&config)
    );
    error(message);
    config_destroy(&config);
    return SLURM_ERROR;
  }

  /* check for optional 'drain' option */
  sprintf(message,"(%s) checking node drain settings", plugin_type);
  debug(message);
  settings -> allow_drain = TRUE;
  if ( config_lookup_bool(&config, "drain", 
                          &(settings -> allow_drain)) == CONFIG_TRUE ) {
    if ( settings -> allow_drain == CONFIG_TRUE ) {
      settings -> allow_drain = TRUE;
    } else {
      settings -> allow_drain = FALSE;
    }
  }

  /* check for optional 'requeue' option */
  sprintf(message,"(%s) checking job requeue settings", plugin_type);
  debug(message);
  settings -> allow_requeue = TRUE;
  if ( config_lookup_bool(&config, "requeue", 
                          &(settings -> allow_requeue)) == CONFIG_TRUE ) {
    if ( settings -> allow_requeue == CONFIG_TRUE ) {
      settings -> allow_requeue = TRUE;
    } else {
      settings -> allow_requeue = FALSE;
    }
  } 

  /* process allowed 'partitions' option */
  sprintf(message,"(%s) checking partition settings", plugin_type);
  debug(message);
  config_setting_t *partitions_opt;
  index = 0;
  partitions_opt = config_lookup(&config,"partitions");
  char add_partition[BUFLEN] = "";
  if ( partitions_opt != NULL ) {   
    if ( config_setting_type(partitions_opt) != CONFIG_TYPE_ARRAY ) {
      sprintf(message,"(%s) setting 'partitions' is not an array in %s",
                       plugin_type,
                       settings -> config_file);
      error(message);
      config_destroy(&config);
      return SLURM_ERROR; 
    }
    settings -> partitions = malloc(index + 1 * sizeof(char*));
    while ( config_setting_get_string_elem(partitions_opt, index) != NULL ) {
      if ( index > 0 ) {
        settings -> partitions = realloc(settings -> partitions,
                                         index + 1 * sizeof(char*));
      }
      sprintf(add_partition,"%s",
                            config_setting_get_string_elem(partitions_opt, index));
      (settings -> partitions)[index] = malloc(strlen(add_partition)*sizeof(char));
      sprintf((settings -> partitions)[index],
              "%s",
              config_setting_get_string_elem(partitions_opt, index));
      index++;
      settings -> n_partitions++;
    }
  }

  /* process allowed 'accounts' options */
  sprintf(message,"(%s) checking account settings", plugin_type);
  debug(message);
  config_setting_t *accounts_opt;
  index = 0;
  accounts_opt = config_lookup(&config,"accounts");
  char add_account[BUFLEN] = "";
  if ( accounts_opt != NULL ) {   
    if ( config_setting_type(accounts_opt) != CONFIG_TYPE_ARRAY ) {
      sprintf(message,"(%s) setting 'accounts' is not an array in %s",
                       plugin_type,
                       settings -> config_file);
      error(message);
      config_destroy(&config);
      return SLURM_ERROR; 
    }
    settings -> accounts = malloc(index + 1 * sizeof(char*));
    while ( config_setting_get_string_elem(accounts_opt, index) != NULL ) {
      if ( index > 0 ) {
        settings -> accounts = realloc(settings -> accounts,
                                         index + 1 * sizeof(char*));
      }
      sprintf(add_account,"%s",
                            config_setting_get_string_elem(accounts_opt, index));
      (settings -> accounts)[index] = malloc(strlen(add_account)*sizeof(char));
      sprintf((settings -> accounts)[index],
              "%s",
              config_setting_get_string_elem(accounts_opt, index));
      index++;
      settings -> n_accounts++;
    }
  }
  
  /* process required 'features'*/
  sprintf(message,"(%s) checking feature settings", plugin_type);
  debug(message);
  config_setting_t *features_opt;
  index = 0;
  features_opt = config_lookup(&config,"features");
  char add_feature[BUFLEN] = "";
  if ( features_opt != NULL ) {   
    if ( config_setting_type(features_opt) != CONFIG_TYPE_ARRAY ) {
      sprintf(message,"(%s) setting 'features' is not an array in %s",
                       plugin_type,
                       settings -> config_file);
      error(message);
      config_destroy(&config);
      return SLURM_ERROR; 
    }
    settings -> features = malloc(index + 1 * sizeof(char*));
    while ( config_setting_get_string_elem(features_opt, index) != NULL ) {
      if ( index > 0 ) {
        settings -> features = realloc(settings -> features,
                                         index + 1 * sizeof(char*));
      }
      sprintf(add_feature,"%s",
                            config_setting_get_string_elem(features_opt, index));
      (settings -> features)[index] = malloc(strlen(add_feature)*sizeof(char));
      sprintf((settings -> features)[index],
              "%s",
              config_setting_get_string_elem(features_opt, index));
      index++;
      settings -> n_features++;
    }
  }

  return SLURM_SUCCESS;
}

/* ************************************************************************** */
/*                                                                            */
/*                       REQUIRED JOB INIT FUNCTION                          */
/*                                                                            */
/* ************************************************************************** */
extern int init() {
  char message[256] = "";
  sprintf(message,"(%s) initializing",plugin_type);
  debug(message);
  return SLURM_SUCCESS;
}

/* ************************************************************************** */
/*                                                                            */
/*                       REQUIRED JOB SUBMIT FUNCTION                         */
/*                                                                            */
/* ************************************************************************** */
extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid,
		      char **err_msg) {
  int i=0;
  char message[256] = "";

  /* Verify this is a ticrypt job request else exit clean without processing */
  int is_ticrypt = FALSE;
  for( i=0; i<job_desc->spank_job_env_size; i++ ) {
    if( strcmp((job_desc -> spank_job_env)[i],OPTION_CHECK) == 0 ) {
      is_ticrypt = TRUE;
      break;
    }
  }
  if ( is_ticrypt != TRUE ) {
    sprintf(message,"(%s) not a ticrypt job request",plugin_type);
    debug(message);
    return SLURM_SUCCESS;
  }

  /* Debug logging if this is a ticrypt request to indicate start */
  sprintf(message,
          "(%s) processing job submission for ticrypt job submit plugin",
          plugin_type);
  debug(message);

  /* Initialize settings object to check against */
  ticrypt_settings_t settings;
  int rc = ticrypt_settings_init(&settings);
  if ( rc != SLURM_SUCCESS ) {
    sprintf(message,"(%s) error initializing ticrypt settings",plugin_type);
    error(message);
    return rc;
  }

  /* Verify job request is exclusive */
  if ( ! job_desc -> shared || job_desc -> shared != 0 ) {
    sprintf(message,"(%s) forcing job to exclusive access",
                    plugin_type);
    info(message);
    job_desc -> shared = 0; 
  }

  /* Verify partition allowed per ticrypt spank configuration */
  if ( settings.n_partitions == 0 ) {
    sprintf(message,"(%s) configuration allows all partitions",
                    plugin_type);
    debug(message);
  } else {
    if ( job_desc -> partition ) {
      sprintf(message,"(%s) verifying partition %s is allowed",
                      plugin_type,
                      job_desc -> partition);
      debug(message);
      int partition_ok = FALSE;
      for( i=0;i<settings.n_partitions;i++ ) {
        if ( strcmp(settings.partitions[i],job_desc->partition) == 0 ) {
          partition_ok = TRUE;
          break;
        }
      }
      if ( partition_ok == FALSE ) {
        sprintf(message,"(%s) partition %s is not allowed",
                        plugin_type,
                        job_desc -> partition);
        error(message);
        ticrypt_settings_free(&settings);
        return ESLURM_INVALID_PARTITION_NAME;
      }
    } else { 
      sprintf(message,"(%s) setting unspecified partition to first allowed %s",
                      plugin_type,
                      settings.partitions[0]);
      info(message);
      job_desc -> partition = xstrdup(settings.partitions[0]);
    }
  }

  /* Verify account allowed per ticrypt spank configuration */
  if ( settings.n_accounts == 0 ) {
    sprintf(message,"(%s) configuration allows all accounts",
                    plugin_type);
    debug(message);
  } else {
    if ( job_desc -> account ) {
      sprintf(message,"(%s) verifying account %s is allowed",
                      plugin_type,
                      job_desc -> account);
      debug(message);
      int account_ok = FALSE;
      for( i=0;i<settings.n_accounts;i++ ) {
        if ( strcmp(settings.accounts[i],job_desc -> account) == 0 ) {
          account_ok = TRUE;
          break;
        }
      }
      if ( account_ok == FALSE ) {
        sprintf(message,"(%s) account %s is not allowed",
                        plugin_type,
                        job_desc -> account);
        error(message);
        ticrypt_settings_free(&settings);
        return ESLURM_ACCOUNTING_POLICY;
      }
    } else {
      sprintf(message,"(%s) require explicit account declaration by user",
                      plugin_type);
      error(message);
      ticrypt_settings_free(&settings);
      return ESLURM_INVALID_ACCOUNT;
    }
  }

  /* Verify required features present in job request */
  if ( settings.n_features < 1 ) {
    sprintf(message,"(%s) configuration does not require specific features",
                    plugin_type);
    debug(message);
  } else {
    sprintf(message,"(%s) verifying required features in request",plugin_type);
    debug(message);
    sprintf(message,"(%s) job has features (%s) in request",
                    plugin_type,
                    job_desc -> features);
    debug(message);
    int n_job_features = 0;
    char **job_features = malloc(1 * sizeof(char*));
    if ( job_desc -> features != NULL ) {
      char *ftoken;
      ftoken = strtok(job_desc -> features,",");
      char feature[256];
      while ( ftoken != NULL ) {
        job_features = realloc(job_features,(n_job_features + 1)*sizeof(char*));
        sscanf(ftoken,"%s",feature);
        job_features[n_job_features] = malloc(strlen(feature)*sizeof(char));
        sprintf(job_features[n_job_features],"%s",feature);
        n_job_features++;
        ftoken = strtok(NULL,",");
      }
    }
    
    for( i=0; i<settings.n_features; i++ ) {
      sprintf(message,"(%s) checking for required feature %s",
                      plugin_type,
                      settings.features[i]);
      debug(message);
      int job_has_feature = FALSE;
      int j = 0;
      for ( j=0; j<n_job_features; j++ ) {
        if ( strcmp(job_features[j],settings.features[i]) == 0 ) {
          job_has_feature = TRUE;
        }
      }  
      if ( n_job_features == 0 || job_has_feature == FALSE ) {
        sprintf(message,"(%s) updating job with missing required feature %s",
                        plugin_type,
                        settings.features[i]);
        info(message);
        char new_feature_string[BUFLEN] = "";
        if ( ! job_desc -> features ) {
          job_desc -> features = xstrdup(settings.features[i]);
        } else {
          sprintf(new_feature_string,"%s,%s",
                                     job_desc -> features,
                                     settings.features[i]);
          job_desc -> features = xstrdup(new_feature_string);
        }
      }
    }
    sprintf(message,"(%s) after processing, job requests features (%s)",
                    plugin_type,
                    job_desc -> features);
    debug(message);
    if ( job_features != NULL ) {
      for( i=0; i<n_job_features; i++ ) {
        if ( job_features[i] != NULL ) {
          free(job_features[i]);
        }
      }
      free(job_features);
    } 
  }


  ticrypt_settings_free(&settings);
  return SLURM_SUCCESS;
}

/* ************************************************************************** */
/*                                                                            */
/*                       REQUIRED JOB MODIFY FUNCTION                         */
/*                                                                            */
/* ************************************************************************** */
extern int job_modify(struct job_descriptor *job_desc,
		      struct job_record *job_ptr, uint32_t submit_uid) {
  return SLURM_SUCCESS;
}

/* ************************************************************************** */
/*                                                                            */
/*                        REQUIRED JOB FINI FUNCTION                          */
/*                                                                            */
/* ************************************************************************** */
extern int fini() {
  char message[256] = "";
  sprintf(message,"(%s) finalizing",plugin_type);
  debug(message);
  return SLURM_SUCCESS;
}
