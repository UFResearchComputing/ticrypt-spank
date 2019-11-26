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
    free(settings -> accounts);
  }

  return SLURM_SUCCESS;
}

/* ****************************************************** */
/*              ticrypt_settings_init()                   */
/* - allocate memory to members of a ticrypt_settings_t   */
/* - parse config into ticrypt_settings_t                 */
/* ****************************************************** */
int ticrypt_settings_init(ticrypt_settings_t *settings) {
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
  int index = 0;
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
  
  /* check config for required 'features' and verify against job request */
  //int n_job_features = 0;
  //char job_features[40][BUFLEN];
  //char *features = (job_spec -> job_array)[0].features;
  //if ( features != NULL ) {
  //  char *ftoken;
  //  ftoken = strtok(features,"&");
  //  char feature[256];
  //  while ( ftoken != NULL ) {
  //    sscanf(ftoken,"%s",feature);
  //    sprintf(job_features[n_job_features],"%s",feature);
  //    n_job_features++;
  //    ftoken = strtok(NULL,"&");
  //  }
  //}
  //config_setting_t *features_opt;
  //char check_feature[BUFLEN];
  //features_opt = config_lookup(&config,"features");
  //settings -> features_ok = FALSE;
  //if ( features_opt != NULL ) {
  //  if ( config_setting_type(features_opt) != CONFIG_TYPE_ARRAY ) {
  //    tlog("setting 'features' is not an array",ERROR);
  //    config_destroy(&config);
  //    slurm_free_job_info_msg(job_spec);
  //    return CONFAIL; 
  //  }
  //  index = 0;
  //  while ( config_setting_get_string_elem(features_opt, index) != NULL ) {
  //    sprintf(check_feature,
  //            "%s",
  //            config_setting_get_string_elem(features_opt, index));
  //    int feature_match = FALSE;
  //    int i = 0;
  //    for (i=0;i<n_job_features;i++) {
  //      if ( strcmp(job_features[i],check_feature) == 0 ) {
  //        feature_match = TRUE;
  //        break;
  //      }
  //    }
  //    if ( feature_match != TRUE ) {
  //      sprintf(message,
  //              "required feature %s not present in job request",
  //             check_feature);
  //      tlog(message,ERROR);
  //      config_destroy(&config);
  //      slurm_free_job_info_msg(job_spec);
  //      return USERFAIL; 
  //    }
  //    index++;
  //  }
  //}
  //settings -> features_ok = TRUE;

  ///* check config for allowed 'accounts' and verify against job charge acct */
  //settings -> account_ok = TRUE;
  //settings -> job_account = (char *) 
  //                          malloc(
  //                                 strlen((job_spec -> job_array)[0].account) *
  //                                  sizeof(char));
  //sprintf(settings -> job_account,
  //                    "%s",
  //                    (job_spec -> job_array)[0].account);
  //config_setting_t *accounts_opt;
  //char check_account[BUFLEN];
  //accounts_opt = config_lookup(&config,"accounts");
  //if ( accounts_opt != NULL ) {
  //  settings -> account_ok = FALSE;
  //  if ( config_setting_type(accounts_opt) != CONFIG_TYPE_ARRAY ) {
  //    tlog("setting 'accounts' is not an array",ERROR);
  //    config_destroy(&config);
  //    slurm_free_job_info_msg(job_spec);
  //    return CONFAIL; 
  //  }
  //  index = 0;
  //  while ( config_setting_get_string_elem(accounts_opt, index) != NULL ) {
  //    sprintf(check_account,
  //            "%s",
  //            config_setting_get_string_elem(accounts_opt, index));
  //    if ( strcmp(settings -> job_account,check_account) == 0 ) {
  //      settings -> account_ok = TRUE;
  //      break;
  //    }
  //    index++;
  //  }
  //}
  //if ( settings -> account_ok == FALSE ) {
  //  sprintf(message,
  //          "account %s is not allowed to run ticrypt jobs",
  //          settings -> job_account);
  //  tlog(message,ERROR);
  //  config_destroy(&config);
  //  slurm_free_job_info_msg(job_spec);
  //  return USERFAIL;
  //}

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
  char message[256] = "";
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
    sprintf(message,"(%s) jobs must request exclusive access, updating",
                    plugin_type);
    debug(message);
    job_desc -> shared = 0; 
  }

  /* Verify partition allowed per ticrypt spank configuration */
  if ( ! job_desc -> account ) {
    sprintf(message,"(%s) jobs must have a valid account",plugin_type);
    error(message);
    return ESLURM_INVALID_ACCOUNT;
  }
  sprintf(message,"(%s) verifying account %s is allowed",
                  plugin_type,
                  job_desc -> account);
  debug(message);

  /* Verify account allowed per ticrypt spank configuration */

  /* Verify required features present in job request */

  /* Verify requeue enabled if allowed per ticrypt spank configuration */

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
