/* ************************************************************************** */
/*                                                                            */
/*                                DESCRIPTION                                 */
/*                                                                            */
/* ************************************************************************** */ 
/*
 
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
#include <slurm/spank.h>
#include <slurm/slurm.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>

/* ************************************************************************** */
/*                                                                            */
/*                                 DEFINES                                    */
/*                                                                            */
/* ************************************************************************** */
/* Standards */ 
#define BUFLEN   256
#define KEYLEN   9
/* Returns */
#define PASS     0
#define FAIL     -1
#define CONFAIL  -2
#define USERFAIL -3
/* Booleans */
#define TRUE     0
#define FALSE    1
/* Logging */
#define PROGRAM  "ticrypt"
#define ERROR   0
#define INFO    1
#define VERBOSE 2
#define DEBUG   3

/* ************************************************************************** */
/*                                                                            */
/*                            SPANK PLUGIN REGISTRATION                       */
/*                                                                            */
/* ************************************************************************** */
SPANK_PLUGIN (ticrypt, 1);

/* ************************************************************************** */
/*                                                                            */
/*                                    HELPERS                                 */
/*                                                                            */
/* ************************************************************************** */
const char *spank_context_names[] = {
  "S_CTX_ERROR",
  "S_CTX_LOCAL",
  "S_CTX_REMOTE",
  "S_CTX_ALLOCATOR",
  "S_CTX_SLURMD",
  "S_CTX_JOB_SCRIPT"
};

int getSpankContext(char *context) {
  spank_context_t sp_context = spank_context();
  if ( sp_context == S_CTX_ERROR ) {
    sprintf(context,"S_CTX_ERROR");
    return FAIL;
  }
  if ( sp_context == S_CTX_LOCAL ) {
    sprintf(context,"S_CTX_LOCAL");
  } else if ( sp_context == S_CTX_REMOTE ) {
    sprintf(context,"S_CTX_REMOTE");
  } else if ( sp_context == S_CTX_ALLOCATOR ) {
    sprintf(context,"S_CTX_ALLOCATOR");
  } else if ( sp_context == S_CTX_JOB_SCRIPT) {
    sprintf(context,"S_CTX_JOB_SCRIPT");
  } else if ( sp_context == S_CTX_SLURMD) {
    sprintf(context,"S_CTX_SLURMD");
  } else if ( sp_context == S_CTX_ERROR) {
    sprintf(context,"S_CTX_ERROR");
  }
  return PASS;
}

void tlog(char *message, int level) {
  char lmessage[BUFLEN];
  sprintf(lmessage,
          "%s: %s",
           PROGRAM,
           message);
  if      ( level <= 0 ) { slurm_error(lmessage);   }
  else if ( level == 1 ) { slurm_info(lmessage);    }
  else if ( level == 2 ) { slurm_verbose(lmessage); }
  else if ( level >= 3 ) { slurm_debug(lmessage);   }
  return;
}

/* ************************************************************************** */
/*                                                                            */
/*                    ticrypt_settings_t and methods                          */
/*                                                                            */
/* ************************************************************************** */
typedef struct ticrypt_settings	 {
  char      *config_file;
  char      *convert;
  char      *revert;
  int       allow_drain;
  int       allow_requeue;
  uint32_t  job_id;
  char      *job_partition;
  char      *job_account;
  int       partition_ok;
  int       account_ok;
  int       features_ok;
} ticrypt_settings_t;

/* ****************************************************** */
/*              ticrypt_settings_free()                   */
/* - cleanup memory allocated to a ticrypt_settings_t     */
/* ****************************************************** */
int ticrypt_settings_free(ticrypt_settings_t *settings) {
  /* config_file */
  if ( settings -> config_file != NULL ) {
    free(settings -> config_file);
  }
  /* convert */
  if ( settings -> convert != NULL ) {
    free(settings -> convert);
  } 
  /* revert */
  if ( settings -> revert != NULL ) {
    free(settings -> revert);
  } 
  /* job_partition */
  if ( settings -> job_partition != NULL ) {
    free(settings -> job_partition);
  }
  /* job_account */
  if ( settings -> job_account != NULL ) {
    free(settings -> job_account);
  }

  return 0;
}

/* ****************************************************** */
/*              ticrypt_settings_init()                   */
/* - allocate memory to members of a ticrypt_settings_t   */
/* - parse config and job to initialize                   */
/* ****************************************************** */
int ticrypt_settings_init(ticrypt_settings_t *settings,int job_id) {
  char message[BUFLEN];
  settings -> config_file = NULL;
  settings -> convert = NULL;
  settings -> revert = NULL;
  settings -> job_partition = NULL;
  settings -> job_account = NULL;
  settings -> allow_drain = TRUE;
  settings -> allow_requeue = TRUE;
  settings -> job_id = FALSE;
  settings -> partition_ok = FALSE;
  settings -> account_ok = FALSE;
  settings -> features_ok = FALSE;

  /* config_file */ 
  const char* env_config_file = getenv("TICRYPT_SPANK_CONFIG");
  char default_path[] = "/etc/ticrypt-spank.conf";
  if ( env_config_file != NULL ) {
    settings -> config_file = (char *) malloc(strlen(env_config_file) * sizeof(char));
    sprintf(settings -> config_file,
            "%s",
            env_config_file);
  } else {
    settings -> config_file = (char *) malloc(strlen(default_path) * sizeof(char));
    sprintf(settings -> config_file,
            "%s",
            "/etc/ticrypt-spank.conf");
  }

  /* set job id */
  settings -> job_id = job_id;

  /* initialize and read the configuration */
  config_t config;
  config_init(&config);
  if(! config_read_file(&config,settings -> config_file)) {
    sprintf(message,"config error %s:%d: %s",
            config_error_file(&config),
            config_error_line(&config),
            config_error_text(&config)
    );
    tlog(message,ERROR);
    config_destroy(&config);
    return CONFAIL;
  }

  /* check for optional 'drain' option */
  settings -> allow_drain = TRUE;
  if ( config_lookup_bool(&config, "drain", 
                          &(settings -> allow_drain)) == CONFIG_TRUE ) {
    if ( settings -> allow_drain == CONFIG_TRUE ) {
      settings -> allow_drain = TRUE;
    } else {
      settings -> allow_drain = FALSE;
    }
  }

  /* check for required 'convert' option */
  const char *convert_opt;
  if ( ! config_lookup_string(&config, "convert", &convert_opt) ) {
    tlog("missing required config option 'convert'",ERROR);
    config_destroy(&config);
    return CONFAIL;
  }
  settings -> convert = (char *) malloc(strlen(convert_opt) * sizeof(char));
  sprintf(settings -> convert,"%s",convert_opt);

  /* check for required 'revert' option */
  const char *revert_opt;
  if ( ! config_lookup_string(&config, "revert", &revert_opt) ) {
    tlog("missing required config option 'revert'",ERROR);
    config_destroy(&config);
    return CONFAIL;
  }
  settings -> revert = (char *) malloc(strlen(convert_opt) * sizeof(char));
  sprintf(settings -> revert,"%s",revert_opt);

  /* check for optional 'requeue' option */
  settings -> allow_requeue = TRUE;
  if ( config_lookup_bool(&config, "requeue", 
                          &(settings -> allow_requeue)) == CONFIG_TRUE ) {
    if ( settings -> allow_requeue == CONFIG_TRUE ) {
      settings -> allow_requeue = TRUE;
    } else {
      settings -> allow_requeue = FALSE;
    }
  } 

  /* Verify job is running without sharing */
  int node_shared = FALSE;
  job_info_msg_t *job_spec;
  if ( slurm_load_job(&job_spec,settings -> job_id,0) != 0 ) {
    tlog("could not lookup job to verify exclusivity",ERROR);
    slurm_free_job_info_msg(job_spec);
    config_destroy(&config);
    return CONFAIL;
  } 
  node_shared = (job_spec -> job_array)[0].shared;
  if ( node_shared != 0 ) {
    tlog("shared jobs are not allowed to run ticrypt",ERROR);
    config_destroy(&config);
    return USERFAIL;
  } 
 
  /* check and process allowed 'partitions' option */
  settings -> partition_ok = TRUE;
  config_setting_t *partitions_opt;
  int index = 0;
  char check_partition[BUFLEN];
  partitions_opt = config_lookup(&config,"partitions");
  if ( partitions_opt != NULL ) {
    settings -> job_partition = (char *) 
                malloc(strlen((job_spec -> job_array)[0].partition)
                * sizeof(char));
	sprintf(settings -> job_partition,
			"%s",
			(job_spec -> job_array)[0].partition);
	settings -> partition_ok = FALSE;
	if ( config_setting_type(partitions_opt) != CONFIG_TYPE_ARRAY ) {
	  tlog("setting 'partitions' is not an array",ERROR);
      config_destroy(&config);
      slurm_free_job_info_msg(job_spec);
	  return CONFAIL; 
	}
	while ( config_setting_get_string_elem(partitions_opt, index) != NULL ) {
	  sprintf(check_partition,
			  "%s",
			  config_setting_get_string_elem(partitions_opt, index));
	  if ( strcmp(settings -> job_partition,check_partition) == 0 ) {
		settings -> partition_ok = TRUE;
		break;
	  }
	  index++;
	}
  }
  if ( settings -> partition_ok == FALSE ) {
	sprintf(message,
            "partition %s is disabled for ticrypt jobs",
            settings -> job_partition);
	tlog(message,ERROR);
    config_destroy(&config);
    slurm_free_job_info_msg(job_spec);
	return USERFAIL; 
  }
  
  /* check config for required 'features' and verify against job request */
  int n_job_features = 0;
  char job_features[40][BUFLEN];
  char *features = (job_spec -> job_array)[0].features;
  if ( features != NULL ) {
    char *ftoken;
    ftoken = strtok(features,"&");
    char feature[256];
    while ( ftoken != NULL ) {
      sscanf(ftoken,"%s",feature);
      sprintf(job_features[n_job_features],"%s",feature);
      n_job_features++;
      ftoken = strtok(NULL,"&");
    }
  }
  config_setting_t *features_opt;
  char check_feature[BUFLEN];
  features_opt = config_lookup(&config,"features");
  settings -> features_ok = FALSE;
  if ( features_opt != NULL ) {
    if ( config_setting_type(features_opt) != CONFIG_TYPE_ARRAY ) {
      tlog("setting 'features' is not an array",ERROR);
      config_destroy(&config);
      slurm_free_job_info_msg(job_spec);
      return CONFAIL; 
    }
    index = 0;
    while ( config_setting_get_string_elem(features_opt, index) != NULL ) {
      sprintf(check_feature,
              "%s",
              config_setting_get_string_elem(features_opt, index));
      int feature_match = FALSE;
      int i = 0;
      for (i=0;i<n_job_features;i++) {
        if ( strcmp(job_features[i],check_feature) == 0 ) {
          feature_match = TRUE;
          break;
        }
      }
      if ( feature_match != TRUE ) {
        sprintf(message,
                "required feature %s not present in job request",
               check_feature);
        tlog(message,ERROR);
        config_destroy(&config);
        slurm_free_job_info_msg(job_spec);
        return USERFAIL; 
      }
      index++;
    }
  }
  settings -> features_ok = TRUE;

  /* check config for allowed 'accounts' and verify against job charge acct */
  settings -> account_ok = TRUE;
  settings -> job_account = (char *) 
                            malloc(
                                   strlen((job_spec -> job_array)[0].account) *
                                    sizeof(char));
  sprintf(settings -> job_account,
                      "%s",
                      (job_spec -> job_array)[0].account);
  config_setting_t *accounts_opt;
  char check_account[BUFLEN];
  accounts_opt = config_lookup(&config,"accounts");
  if ( accounts_opt != NULL ) {
    settings -> account_ok = FALSE;
    if ( config_setting_type(accounts_opt) != CONFIG_TYPE_ARRAY ) {
      tlog("setting 'accounts' is not an array",ERROR);
      config_destroy(&config);
      slurm_free_job_info_msg(job_spec);
      return CONFAIL; 
    }
    index = 0;
    while ( config_setting_get_string_elem(accounts_opt, index) != NULL ) {
      sprintf(check_account,
              "%s",
              config_setting_get_string_elem(accounts_opt, index));
      if ( strcmp(settings -> job_account,check_account) == 0 ) {
        settings -> account_ok = TRUE;
        break;
      }
      index++;
    }
  }
  if ( settings -> account_ok == FALSE ) {
    sprintf(message,
            "account %s is not allowed to run ticrypt jobs",
            settings -> job_account);
    tlog(message,ERROR);
    config_destroy(&config);
    slurm_free_job_info_msg(job_spec);
    return USERFAIL;
  }

  return PASS;
}

/* ************************************************************************** */
/*                                                                            */
/*                             PLUGIN OPTIONS                                 */
/*                                                                            */
/* ************************************************************************** */
static int ticrypt = FALSE;
static int _ticrypt_opt_flag_process( int val, const char *optarg, int remote );

struct spank_option spank_options[] = {
  { 
    "ticrypt",
    "",
	"Trigger ticrypt node reconfiguration",
    2,
    0,
	(spank_opt_cb_f) _ticrypt_opt_flag_process
  },
  SPANK_OPTIONS_TABLE_END
};

/* ************************************************************************** */
/*                                                                            */
/*                        OPTIONS REGISTRATION                                */
/*                                                                            */
/* ************************************************************************** */
int slurm_spank_init(spank_t sp, int ac, char **av) {
  spank_option_register(sp,spank_options);
  return 0;
}


/* ************************************************************************** */
/*                                                                            */
/*                             job_prolog                                     */
/*                                                                            */
/* ************************************************************************** */
int slurm_spank_job_prolog(spank_t sp, int ac, char **av) {
  tlog("starting job_prolog",DEBUG);

  /* Define counts to check against */
  uint32_t n_tasks = 2;
  uint32_t n_nodes = 1;

  /* Get task and node counts from environment */
  if ( spank_get_item(sp,S_JOB_NNODES,&n_nodes) != ESPANK_SUCCESS ) {
    tlog("could not determine number of nodes in job",ERROR);
    return FAIL;
  }

  if ( spank_get_item(sp,S_JOB_TOTAL_TASK_COUNT,&n_nodes) != ESPANK_SUCCESS ) {
    tlog("could not determine number of tasks in job",ERROR);
    return FAIL;
  }

  /* Verify total task count does not exceed node count */
  /* https://github.com/UFResearchComputing/ticrypt-spank/issues/1 */
  if ( n_tasks > n_nodes ) {
    tlog("ticrypt jobs require a single task per node",ERROR);
    return FAIL;
  } 


  return PASS;
}

/* ************************************************************************** */
/*                                                                            */
/*                         task_init_privileged                               */
/*                                                                            */
/* ************************************************************************** */
int slurm_spank_task_init_privileged(spank_t sp, int ac, char **av) {


  /* Define logging variables */
  char message[BUFLEN] = "";
  char hostname[BUFLEN] = "";
  char context[BUFLEN] = "unknown";

  tlog("starting task_init_privileged",DEBUG);

  /* Do not proceed if --ticrypt not specified */
  if ( ticrypt != TRUE ) {
    tlog("not a ticrypt task",DEBUG);
    return PASS;
  }

  /* Get hostname for node where task is running */
  if ( gethostname(hostname,BUFLEN) != 0 ) {
    tlog("could not obtain hostname",ERROR);
    return FAIL;
  }

  /* Get a short version of the hostname in case of need to drain */
  char hostname_tmp[BUFLEN];
  sprintf(hostname_tmp,"%s",hostname);
  char *shost = strtok(hostname_tmp,".");
  char slurm_hostname[BUFLEN];
  sprintf(slurm_hostname,"%s",shost);
  sprintf(message,"using slurm hostname %s",slurm_hostname);
  tlog(message,DEBUG); 
 
  /* Get spank context */
  if ( getSpankContext(context) != PASS ) {
    tlog("unable to get spank context",ERROR);
    return FAIL;
  }
  sprintf(message,"running in context %s",context);
  tlog(message,DEBUG);

  /* Only run this in remote context  - on the allocated node where task runs */
  if ( strcmp(context,"S_CTX_REMOTE") != 0 ) {
    return PASS;
  }

  /* Get jobid from spank environment for config initialization */
  char env_job_id[BUFLEN];
  if ( spank_getenv(sp, "SLURM_JOBID", env_job_id, BUFLEN-1) != ESPANK_SUCCESS ) {
    tlog("SLURM_JOBID not found. Aborting.",ERROR);
    return FAIL; 
  }
  int job_id = atoi(env_job_id);  

  /* Read and process settings against job request */
  tlog("reading config and processing job options",DEBUG);
  ticrypt_settings_t settings;
  int rc = ticrypt_settings_init(&settings,job_id);
  if ( rc != PASS ) {
    tlog("error initializing ticrypt settings",ERROR);
  }
  else {
    tlog("successful job and config processing",DEBUG);
  }

  /* Drain node if configuration errors and allowed by config */
  if ( rc == CONFAIL ) {
    tlog("experienced configuration error",ERROR);
    if ( settings.allow_drain == TRUE ) {
      sprintf(message,
              "setting node to drain due to ticrypt config errors");
      tlog(message,VERBOSE);
      char reason[] = "ticrypt config error";
      struct slurm_update_node_msg node_msg;
      slurm_init_update_node_msg(&node_msg);
      node_msg.node_names = slurm_hostname;
      node_msg.node_state = NODE_STATE_DRAIN;
      node_msg.reason = reason;
      node_msg.reason_uid = 0;
      if ( slurm_update_node(&node_msg) != SLURM_SUCCESS ) {
        sprintf(message,
                "draining node failed with %s",
                slurm_strerror(errno));
        tlog(message,ERROR);
      }
    } else {
      tlog("'drain' is disabled by ticrypt configuration",VERBOSE);
    }
    ticrypt_settings_free(&settings);
    return FAIL;
  } 
  if ( rc == USERFAIL ) {
    tlog("experienced user request error",ERROR);
    ticrypt_settings_free(&settings);
    return FAIL;
  }

  /*  Execute node conversion  */
  tlog("converting node to ticrypt vm host",INFO);
  int convert_status = system(settings.convert);
  if ( convert_status != PASS ) {
    sprintf(message,
            "unable to run conversion on %s (%d)",
            slurm_hostname,
            convert_status);
    tlog(message,ERROR);
    /*  node drain if allowed */
    if ( settings.allow_drain == TRUE ) {
      sprintf(message,
              "setting node %s to drain due to ticrypt conversion error",
              slurm_hostname);
      tlog(message,INFO);
      struct slurm_update_node_msg node_msg;
      slurm_init_update_node_msg(&node_msg);
      node_msg.node_names = slurm_hostname;
      node_msg.node_state = NODE_STATE_DRAIN;
      char drain_reason[BUFLEN] = "ticrypt convert failure";
      node_msg.reason = drain_reason;
      node_msg.reason_uid = 0;
      if ( slurm_update_node(&node_msg) != SLURM_SUCCESS ) {
        sprintf(message,
                "draining node failed with %s",
                slurm_strerror(errno));
        tlog(message,ERROR);
        ticrypt_settings_free(&settings);
        return FAIL;
      }
    } else {
      tlog("'drain' is disabled by ticrypt configuration",VERBOSE);
    }
    /* attempt job requeue if allowed */
    if ( settings.allow_requeue == TRUE ) {
      tlog("sending reqeue request",DEBUG);
      int requeue_rc = slurm_requeue(settings.job_id,JOB_RECONFIG_FAIL);
      if ( requeue_rc != 0 ) {
        sprintf(message,
                "job requeue failed with %s",
                slurm_strerror(requeue_rc));
        tlog(message,ERROR);
      } 
    } else {
      tlog("'requeue' is disabled by dicrypt configuration",VERBOSE);
    }
    ticrypt_settings_free(&settings);
    return FAIL;
  }
   
  /* cleanup */
  tlog("cleaning dynamically allocated memory",DEBUG);
  ticrypt_settings_free(&settings);

  /* logging verification */
  tlog("ticrypt conversion complete",INFO);

  return PASS;
}

/* ************************************************************************** */
/*                                                                            */
/*                                task_exit                                   */
/*                                                                            */
/* ************************************************************************** */
int slurm_spank_task_exit(spank_t sp, int ac, char **av) {

  /* Define logging variables */
  char message[BUFLEN] = "";
  char hostname[BUFLEN] = "";
  char context[BUFLEN] = "unknown";

  tlog("starting task_exit",DEBUG);

  /* Do not proceed if --ticrypt not specified */
  if ( ticrypt != TRUE ) {
    tlog("not a ticrypt task",DEBUG);
    return PASS;
  }

  /* Get hostname for node where task is running */
  if ( gethostname(hostname,BUFLEN) != 0 ) {
    tlog("could not obtain hostname",ERROR);
    return FAIL;
  }

  /* Get a short version of the hostname in case of need to drain */
  char hostname_tmp[BUFLEN];
  sprintf(hostname_tmp,"%s",hostname);
  char *shost = strtok(hostname_tmp,".");
  char slurm_hostname[BUFLEN];
  sprintf(slurm_hostname,"%s",shost);
  sprintf(message,"using slurm hostname %s",slurm_hostname);
  tlog(message,DEBUG); 
 
  /* Get spank context */
  if ( getSpankContext(context) != PASS ) {
    tlog("unable to get spank context",ERROR);
    return FAIL;
  }
  sprintf(message,"running in context %s",context);
  tlog(message,DEBUG);

  /* Only run this in remote context  - on the allocated node where task runs */
  if ( strcmp(context,"S_CTX_REMOTE") != 0 ) {
    return PASS;
  }

  /* Get jobid from spank environment for config initialization */
  char env_job_id[BUFLEN];
  if ( spank_getenv(sp, "SLURM_JOBID", env_job_id, BUFLEN-1) != ESPANK_SUCCESS ) {
    tlog("SLURM_JOBID not found. Aborting.",ERROR);
    return FAIL; 
  }
  int job_id = atoi(env_job_id);  

  /* Read and process settings against job request */
  tlog("reading config and processing job options",DEBUG);
  ticrypt_settings_t settings;
  int rc = ticrypt_settings_init(&settings,job_id);
  if ( rc != PASS ) {
    tlog("error initializing ticrypt settings",ERROR);
  }
  else {
    tlog("successful job and config processing",DEBUG);
  }

  /* Drain node if configuration errors and allowed by config */
  if ( rc == CONFAIL ) {
    tlog("experienced configuration error",ERROR);
    if ( settings.allow_drain == TRUE ) {
      sprintf(message,
              "setting node to drain due to ticrypt config errors");
      tlog(message,VERBOSE);
      char reason[] = "ticrypt config error";
      struct slurm_update_node_msg node_msg;
      slurm_init_update_node_msg(&node_msg);
      node_msg.node_names = slurm_hostname;
      node_msg.node_state = NODE_STATE_DRAIN;
      node_msg.reason = reason;
      node_msg.reason_uid = 0;
      if ( slurm_update_node(&node_msg) != SLURM_SUCCESS ) {
        sprintf(message,
                "draining node failed with %s",
                slurm_strerror(errno));
        tlog(message,ERROR);
      }
    } else {
      tlog("'drain' is disabled by ticrypt configuration",VERBOSE);
    }
    ticrypt_settings_free(&settings);
    return FAIL;
  } 
  if ( rc == USERFAIL ) {
    tlog("experienced user request error",ERROR);
    ticrypt_settings_free(&settings);
    return FAIL;
  }

  /*  Execute node revert  */
  tlog("converting node back to compute node",INFO);
  int revert_status = system(settings.revert);
  if ( revert_status != PASS ) {
    sprintf(message,
            "unable to run revert on %s (%d)",
            slurm_hostname,
            revert_status);
    tlog(message,ERROR);
    /*  node drain if allowed */
    if ( settings.allow_drain == TRUE ) {
      sprintf(message,
              "setting node %s to drain due to ticrypt revert error",
              slurm_hostname);
      tlog(message,INFO);
      struct slurm_update_node_msg node_msg;
      slurm_init_update_node_msg(&node_msg);
      node_msg.node_names = slurm_hostname;
      node_msg.node_state = NODE_STATE_DRAIN;
      char drain_reason[BUFLEN] = "ticrypt revert failure";
      node_msg.reason = drain_reason;
      node_msg.reason_uid = 0;
      if ( slurm_update_node(&node_msg) != SLURM_SUCCESS ) {
        sprintf(message,
                "draining node failed with %s",
                slurm_strerror(errno));
        tlog(message,ERROR);
        ticrypt_settings_free(&settings);
        return FAIL;
      }
    } else {
      tlog("'drain' is disabled by ticrypt configuration",VERBOSE);
    }
    ticrypt_settings_free(&settings);
    return FAIL;
  }
  
  /* RESUME node in case cleanup took too long or NHC noticed */
  tlog("setting node back to resume after clean revert",VERBOSE);
  struct slurm_update_node_msg node_msg;
  slurm_init_update_node_msg(&node_msg);
  node_msg.node_names = slurm_hostname;
  node_msg.node_state = NODE_RESUME;
  node_msg.reason_uid = 0;
  if ( slurm_update_node(&node_msg) != SLURM_SUCCESS ) {
    sprintf(message,
            "node resume failed with %s",
            slurm_strerror(errno));
    tlog(message,ERROR);
  }
 
  /* cleanup */
  tlog("cleaning dynamically allocated memory",DEBUG);
  ticrypt_settings_free(&settings);

  /* logging verification */
  tlog("ticrypt revert complete",INFO);

  return PASS;
}

/* ************************************************************************** */
/*                                                                            */
/*                     OPTIONS PROCESSING FOR PLUGIN                          */
/*                                                                            */
/* ************************************************************************** */
static int _ticrypt_opt_flag_process ( int val, const char *optarg, int remote ) {
  ticrypt = TRUE;
  return PASS;
}
