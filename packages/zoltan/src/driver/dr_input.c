/*====================================================================
 * ------------------------
 * | CVS File Information |
 * ------------------------
 *
 * $RCSfile$
 *
 * $Author$
 *
 * $Date$
 *
 * $Revision$
 *
 * $Name$
 *====================================================================*/
#ifndef lint
static char *cvs_input_id = "$Id$";
#endif

/*--------------------------------------------------------------------------*/
/* Purpose: Determine file types for command files and read in the parallel */
/*          ExodusII command file.                                          */
/*          Taken from nemesis utilites nem_spread and nem_join.            */
/*--------------------------------------------------------------------------*/
/* Author(s):  Matthew M. St.John (9226)                                    */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* Revision History:                                                        */
/*    14 April 1999:       Date of creation.                                */
/*--------------------------------------------------------------------------*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mpi.h>

#include "dr_const.h"
#include "dr_input_const.h"
#include "dr_util_const.h"
#include "dr_err_const.h"

#define TLIST_CNT 5
#define MAX_INPUT_STR_LN 4096   /* maximum string length for read_string()  */

/*****************************************************************************/
/*****************************************************************************/
int read_cmd_file(char *filename, PROB_INFO_PTR prob,
                  PARIO_INFO_PTR pio_info)

/*
 *          This function reads the ASCII parallel-exodus command file.
 *
 *   Input
 *   -----
 *   filename - The name of the command file.
 *   pio_info - parallel I/O information.
 */
{
/* local declarations */
  FILE  *file_cmd;
  char   cmesg[256]; /* for error messages */
  char   inp_line[MAX_INPUT_STR_LN + 1];
  char   inp_copy[MAX_INPUT_STR_LN + 1];
  char  *cptr, *cptr2;
  double value;
  int    i, icnt;
  int    iret, param;

/***************************** BEGIN EXECUTION ******************************/

  /* Open the file */
  if((file_cmd=fopen(filename, "r")) == NULL)
    return 0;

  /* Begin parsing the input file */
  while(fgets(inp_line, MAX_INPUT_STR_LN, file_cmd)) {
    /* skip any line that is a comment */
    if((inp_line[0] != '#') && (inp_line[0] != '\n')) {

      strcpy(inp_copy, inp_line);
      clean_string(inp_line, " \t");
      cptr = strtok(inp_line, "\t=");
      /****** The FEM file type ******/
      if (token_compare(cptr, "fem file type")) {
        if(pio_info->file_type < 0) {
          cptr = strtok(NULL, "\t=");
          strip_string(cptr, " \t\n");
          if (cptr == NULL || strlen(cptr) == 0) {
            Gen_Error(0, "fatal: must specify FEM file type");
            Gen_Error(0, cmesg);
            return 0;
          }

          if (strcasecmp(cptr, "nemesisi") == 0) {
            pio_info->file_type = NEMESIS_FILE;
          }
          else if (strcasecmp(cptr, "chaco") == 0) {
            pio_info->file_type = CHACO_FILE;
          }
          else {
            sprintf(cmesg, "fatal: unknown file type, %s", cptr);
            Gen_Error(0, cmesg);
            return 0;
          }
        }
      }
      /****** The FEM file name ******/
      if (token_compare(cptr, "fem file name")) {
        if(strlen(pio_info->pexo_fname) == 0)
        {
          cptr = strtok(NULL, "\t=");
          strip_string(cptr, " \t\n");
          strcpy(pio_info->pexo_fname, cptr);
        }
      }
      /****** The Debug reporting level ******/
/* not sure about this yet ------------->
      else if (token_compare(cptr, "debug")) {
        if (Debug_Flag < 0) {
          cptr = strtok(NULL, "\t=");
          strip_string(cptr, " \t\n");
          if(sscanf(cptr, "%d", &Debug_Flag) != 1) {
            Gen_Error(0, "fatal: can\'t interp int for Debug_Flag");
            return 0;
          }
        }
      }
<-----------------------------------------*/
      /****** Parallel Disk Information ******/
      else if (token_compare(cptr, "parallel disk info")) {

        cptr = strchr(cptr, '\0');
        cptr++;
        strip_string(cptr," \t\n=");
        cptr = strtok(cptr, ",");
        strip_string(cptr, " \t\n");
        string_to_lower(cptr, '=');

        /* the first sub-option must be "number" */
        if (!strstr(cptr, "number")) {
          Gen_Error(0, "fatal: First sup-option for disk info must be "
                       "\"number\"");
          return 0;
        }
        else {
          cptr2 = strchr(cptr, '=');
          if (cptr2 == NULL) {
            Gen_Error(0, "fatal: integer value must be specified for"
                         " reserve space.");
            return 0;
          }
          cptr2++;
          icnt = sscanf(cptr2, "%d", &(pio_info->num_dsk_ctrlrs));
          if ((icnt <= 0) || (pio_info->num_dsk_ctrlrs < 0)) {
            Gen_Error(0, "fatal: Invalid value for # of raid controllers.");
            return 0;
          }
        }

        cptr = strtok(NULL, ",");

        /*
         * if number = 0, then the input FEM file(s) is in the
         * root directory given by the parallel disk info, or it
         * is in the same directory as the executable if nothing
         * is given for the root infomation. So, no other options
         * can be given when number = 0
         */
        if (pio_info->num_dsk_ctrlrs == 0 && cptr != NULL) {
          Gen_Error(0, "fatal: Other options not allowed if number = 0.");
          return 0;
        }

        while (cptr != NULL) {
          strip_string(cptr, " \t\n");
          string_to_lower(cptr, '=');
          if (strstr(cptr, "list")) {
            /*
             * So, "number" references the length of the list, and
             * I need to do some shuffling to make the new form
             * work with the old code.
             */
            pio_info->dsk_list_cnt = pio_info->num_dsk_ctrlrs;
            pio_info->num_dsk_ctrlrs = -1;

            /* "{" defines the beginning of the list */
            cptr = strchr(cptr, '{');
            if (cptr == NULL) {
              Gen_Error(0, "fatal: disk list must be specified.");
              return 0;
            }
            cptr++;

            /* allocate memory for to hold the values */
            pio_info->dsk_list = (int *) malloc(pio_info->dsk_list_cnt *
                                               sizeof(int));
            for (i = 0; i < (pio_info->dsk_list_cnt - 1); i++) {
              sscanf(cptr, "%d", &(pio_info->dsk_list[i]));
              cptr = strtok(NULL, ", \t;");
            }
            /* last one is a special case */
            sscanf(cptr, "%d}", &(pio_info->dsk_list[i]));
          }
          else if (strstr(cptr, "offset")) {
            cptr2 = strchr(cptr, '=');
            if (cptr2 == NULL) {
              Gen_Error(0, "fatal: value must be specified with the "
                           "\"offset\" option.");
              return 0;
            }
            cptr2++;
            icnt = sscanf(cptr2, "%d", &(pio_info->pdsk_add_fact));
            if ((icnt <= 0) || (pio_info->pdsk_add_fact < 0)) {
              Gen_Error(0, "fatal: Invalid value for offset.");
              return 0;
            }
          }
          else if (strcmp(cptr, "zeros") == 0) {
            pio_info->zeros = 1;
          }

          cptr = strtok(NULL, ",");
        }
      } /* End "else if (token_compare(cptr, "parallel disk info"))" */
      else if (token_compare(cptr, "parallel file location")) {
        cptr = strchr(cptr, '\0');
        cptr++;
        strip_string(cptr," \t\n=");
        cptr = strtok(cptr, ",");

        while (cptr != NULL) {
          strip_string(cptr, " \t\n");
          string_to_lower(cptr, '=');
          if (strstr(cptr, "root")) {
            cptr2 = strchr(cptr, '=');
            if(cptr2 == NULL)
            {
              Gen_Error(0, "fatal: must specify a path with \"root\"");
              return 0;
            }
            cptr2++;
            if(strlen(cptr2) == 0)
            {
              Gen_Error(0, "fatal: invalid path name specified with \"root\"");
              return 0;
            }
            strcpy(pio_info->pdsk_root, cptr2);
          }
          if (strstr(cptr, "subdir")) {
            cptr2 = strchr(cptr, '=');
            if(cptr2 == NULL)
            {
              Gen_Error(0, "fatal: must specify a path with \"subdir\"");
              return 0;
            }
            cptr2++;
            if(strlen(cptr2) == 0)
            {
              Gen_Error(0, "fatal: invalid path name specified with "
                           "\"subdir\"");
              return 0;
            }
            strcpy(pio_info->pdsk_subdir, cptr2);
            if (pio_info->pdsk_subdir[strlen(pio_info->pdsk_subdir)-1] != '/')
              strcat(pio_info->pdsk_subdir, "/");
          }

          cptr = strtok(NULL, ",");
        }
      }
      else if (token_compare(cptr, "decomposition info")) {
        /* The method to use for decomposing the graph */

        /* Search to the first null character */
        cptr = strchr(cptr, '\0');
        cptr++;
        strip_string(cptr, " \t\n=");
        cptr = strtok(cptr, ",");
        while(cptr != NULL)
        {
          strip_string(cptr, " \t\n");
          string_to_lower(cptr, '\0');
          if(strstr(cptr, "method"))
          {
            if(strlen(prob->method) == 0)
            {
              cptr2 = strchr(cptr, '=');
              if(cptr2 == NULL)
              {
                Gen_Error(0, "fatal: need to specify a value with method");
                return 0;
              }

              cptr2++;
              strcpy(prob->method, cptr2);
            }
          }
          else if(strstr(cptr, "tol"))
          {
            if(prob->tol < 0)
            {
              cptr2 = strchr(cptr, '=');
              if(cptr2 == NULL)
              {
                Gen_Error(0, "fatal: need to specify a value with tol");
                return 0;
              }

              cptr2++;
              iret = sscanf(cptr2, "%lf", &(prob->tol));
              if(iret != 1)
              {
                Gen_Error(0, "fatal: invalid value for tolerance");
                return 0;
              }
            }
          }
          else
          {
            sprintf(cmesg,
                    "fatal: unknown LB method \"%s\" specified in command"
                    " file", cptr);
            Gen_Error(0, cmesg);
            return 0;
          }
          cptr = strtok(NULL, ",");
        }
      }
      else if (token_compare(cptr, "zoltan parameters")) {
        /* parameters to be passed to Zoltan */
        cptr = strchr(cptr, '\0');
        cptr++;
        strip_string(cptr," \t\n=");
        cptr = strtok(cptr, ",");

        /* parameters should be designated by "<param #>=<value>" */
        while (cptr != NULL) {
          iret = sscanf(cptr, "%d", &param);
          if (iret != 1) {
            Gen_Error(0, "fatal: parameter number must be specified");
            return 0;
          }
          if (param > LB_PARAMS_MAX_SIZE || param < 0) {
            sprintf(cmesg, "fatal: parameter number, %d, invalid", param);
            Gen_Error(0, cmesg);
            return 0;
          }

          /* now get the value */
          cptr2 = strchr(cptr, '=');
          if(cptr2 == NULL)
          {
            Gen_Error(0, "fatal: must specify a parameter value");
            return 0;
          }
          cptr2++;

          iret = sscanf(cptr2, "%lf", &value);
          if (iret != 1) {
            Gen_Error(0, "fatal: must specify a parameter value");
            return 0;
          }
          prob->params[param] = value;
          cptr = strtok(NULL, ",");
        }
      }
    } /* End "if(inp_line[0] != '#')" */
  } /* End "while(fgets(inp_line, MAX_INPUT_STR_LN, file_cmd))" */


  /* Close the command file */
  fclose(file_cmd);

  return 1;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int check_inp(PROB_INFO_PTR prob, PARIO_INFO_PTR pio_info)
{
  char   cmesg[256]; /* for error messages */
/***************************** BEGIN EXECUTION ******************************/

  /* check for the parallel Nemesis file for proc 0 */
  if (strlen(pio_info->pexo_fname) <= 0) {
    Gen_Error(0, "fatal: must specify parallel results file base name");
    return 0;
  }

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                 Check the parallel IO specifications                      */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
  /* check that there is a list of disks, or a number of raids */
  if ((pio_info->dsk_list_cnt <= 0) && (pio_info->num_dsk_ctrlrs < 0))
    pio_info->num_dsk_ctrlrs = 0; /* default to single directory */

  /* default is not to have preceeding 0's in the disk names */
  if (pio_info->zeros < 0) pio_info->zeros = 0;

  /* default file type is nemesis */
  if (pio_info->file_type < 0) pio_info->file_type = NEMESIS_FILE;

  /* most systems that we deal with start their files systems with 1 not 0 */
  if (pio_info->pdsk_add_fact < 0) pio_info->pdsk_add_fact = 1;

  /*
   * if there are parallel disks, then the root and subdir locations must
   * be specified
   */
  if (pio_info->num_dsk_ctrlrs > 0 || pio_info->dsk_list_cnt > 0) {
    if (strlen(pio_info->pdsk_root) == 0) {
      Gen_Error(0, "fatal: must specify parallel disk root name");
      return 0;
    }
    if (strlen(pio_info->pdsk_subdir) == 0) {
      Gen_Error(0, "fatal: must specify parallel disk subdirectory");
      return 0;
    }
  }
  else
    if (strlen(pio_info->pdsk_root) == 0)
      strcpy(pio_info->pdsk_root, "."); /* default is execution directory */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                 Check the Zoltan specifications                           */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
  /*
   * check the method being used, and determine what needs to
   * be read in and generated to support it
   *
   * A new section needs to be added for every new load balancing
   * algorithm added to Zoltan.
   */
  if (strlen(prob->method) == 0) {
    Gen_Error(0, "fatal: load balance method must be specified");
    return 0;
  }

  if (strcasecmp(prob->method, "RCB") == 0) {
    prob->read_coord = 1;
    prob->gen_graph = 0;
  }
  else if (strcasecmp(prob->method, "OCTPART") == 0) {
    prob->read_coord = 1;
    prob->gen_graph = 0;
  }
  else if (strcasecmp(prob->method, "PARMETIS_PART") == 0) {
    prob->read_coord = 0;
    prob->gen_graph = 1;
  }
  else if (strcasecmp(prob->method, "PARMETIS_REPART") == 0) {
    prob->read_coord = 0;
    prob->gen_graph = 1;
  }
  else if (strcasecmp(prob->method, "PARMETIS_REFINE") == 0) {
    prob->read_coord = 0;
    prob->gen_graph = 1;
  }
  /*
   * Add information about new methods here
   */
  else {
    sprintf(cmesg, "fatal: unknown loadbalance method, %s", prob->method);
    Gen_Error(0, cmesg);
    return 0;
  }

  /* check if the tolerance was set */
  if (prob->tol < 0.0) {
    prob->tol = 1.0;  /* default = perfect balance */
  }

  return 1;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void brdcst_cmd_info(int Proc, PROB_INFO_PTR prob, PARIO_INFO_PTR pio_info)
{
/* local declarations */
  int ctrl_id;
/***************************** BEGIN EXECUTION ******************************/

  MPI_Bcast(pio_info, sizeof(PARIO_INFO), MPI_BYTE, 0, MPI_COMM_WORLD);

  if(pio_info->dsk_list_cnt > 0) {
    if(Proc != 0)
      pio_info->dsk_list = (int *) malloc(pio_info->dsk_list_cnt*sizeof(int));

    MPI_Bcast(pio_info->dsk_list, pio_info->dsk_list_cnt, MPI_INT,
              0, MPI_COMM_WORLD);
  }

  /* and broadcast the problem specifications */
  MPI_Bcast(prob, sizeof(PROB_INFO), MPI_BYTE, 0, MPI_COMM_WORLD);

  /* now calculate where the file for this processor is */
  if(pio_info->dsk_list_cnt <= 0) {
    if (pio_info->num_dsk_ctrlrs > 0) {
      ctrl_id = (Proc % pio_info->num_dsk_ctrlrs);
      pio_info->rdisk = ctrl_id + pio_info->pdsk_add_fact;
    }
  }
  else {
    ctrl_id = Proc % pio_info->dsk_list_cnt;
    pio_info->rdisk = pio_info->dsk_list[ctrl_id];
  }

  return;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void gen_par_filename(char *scalar_fname, char *par_fname,
                      PARIO_INFO_PTR pio_info, int proc_for, int nprocs)
/*----------------------------------------------------------------------------
 *
 *      Author(s):     Gary Hennigan (1421)
 *----------------------------------------------------------------------------
 *      Function which generates the name of a parallel file for a
 *      particular processor. The function does this by appending
 *      "N.p" to the end of the input parameter "scalar_fname", where:
 *
 *              N - The number of processors utilized
 *              p - The processor ID.
 *
 *      In addition, the location of the parallel disk system is prepended
 *      to each file name.
 *---------------------------------------------------------------------------
 *      Example:
 *
 *        scalar_fname = "Parallel-exoII-"   (Input)
 *        par_fname    = "/raid/io_01/tmp/rf_crew/Parallel-exoII-8.0" (Output)
 *
 *      where, for this example:
 *
 *              N = 8 processors
 *              p = 0 particular processor ID
 *---------------------------------------------------------------------------
 *      Revision History:
 *
 *              05 November 1993:    Date of Creation
 *---------------------------------------------------------------------------
 */
{

  /*      Local variables      */

  int i1, iTemp1;
  int iMaxDigit=0, iMyDigit=0;
  char cTemp[FILENAME_MAX];

/************************* EXECUTION BEGINS *******************************/

  /*
   * Find out the number of digits needed to specify the processor ID.
   * This allows numbers like 01-99, i.e., prepending zeros to the
   * name to preserve proper alphabetic sorting of the files.
   */

  iTemp1 = nprocs;
  do
  {
    iTemp1 /= 10;
    iMaxDigit++;
  }
  while(iTemp1 >= 1);

  iTemp1 = proc_for;
  do
  {
    iTemp1 /= 10;
    iMyDigit++;
  }
  while(iTemp1 >= 1);

  /*
   * Append the number of processors in this run to the scalar file name
   * along with a '.' (period).
   */
  par_fname[0] = 0x00;
  strcpy(par_fname, scalar_fname);
  strcat(par_fname, ".");
  sprintf(cTemp, "%d", nprocs);
  strcat(par_fname, cTemp);
  strcat(par_fname, ".");

  /*
   * Append the proper number of zeros to the filename.
   */
  for(i1=0; i1 < iMaxDigit-iMyDigit; i1++)
    strcat(par_fname, "0");

  /*
   * Generate the name of the directory on which the parallel disk
   * array resides. This also directs which processor writes to what
   * disk.
   */
  sprintf(cTemp, "%d", proc_for);
  strcat(par_fname, cTemp);
  strcpy(cTemp, par_fname);


  /*
   * Finally, generate the complete file specification for the parallel
   * file used by this processor.
   */
  if (pio_info->num_dsk_ctrlrs > 0) {
    if(pio_info->zeros) {
      if(pio_info->rdisk <= 9) {
        sprintf(par_fname, "%s%d%d/%s%s", pio_info->pdsk_root,0,
                pio_info->rdisk, pio_info->pdsk_subdir, cTemp);
      }
      else {
        sprintf(par_fname, "%s%d/%s%s", pio_info->pdsk_root,
                pio_info->rdisk, pio_info->pdsk_subdir, cTemp);
      }
    }
    else {
      sprintf(par_fname, "%s%d/%s%s", pio_info->pdsk_root, pio_info->rdisk,
              pio_info->pdsk_subdir, cTemp);
    }
  }
  else
    sprintf(par_fname, "%s/%s", pio_info->pdsk_root, cTemp);

  return;
}
