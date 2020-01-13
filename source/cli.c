/*
 * Command line interface
 */

#include "types.h"
#include "kernel.h"
#include "hw86.h"
#include "ulib/ulib.h"
#include "syscall.h"
#include "fs.h"
#include "video.h"
#include "net.h"

/*
 * Extern program call
 */
#define UPROG_MEMSEG 0x1800L
#define UPROG_MEMLOC 0x0000L
#define UPROG_STRLOC 0xFF88L
#define UPROG_ARGLOC 0xFF80L

/* Cls command: clear the screen */
static void cli_cls(uint argc)
{
  if(argc == 1) {
    syscall(SYSCALL_IO_CLEAR_SCREEN, 0L);
  } else {
    putstr("usage: cls\n\r");
  }
}

/* List command: list directory entries */
static void cli_list(uint argc, uchar* argv[])
{
  /* If not path arg provided, add and implicit ROOT_DIR_NAME */
  /* This way it shows system disk contents */
  if(argc == 1) {
    argv[1] = ROOT_DIR_NAME;
    argc = 2;
  }

  if(argc == 2) {
    uint n=0, i=0, result=0;
    uchar line[64];
    sfs_entry_t entry;

    /* Get number of entries in target dir */
    n = fs_list(&entry, argv[1], 0);
    if(n >= ERROR_ANY) {
      putstr("path not found\n\r");
      return;
    }

    if(n > 0) {
      putstr("\n\r");

      /* Print one by one */
      for(i=0; i<n; i++) {
        time_t etime;
        uint c=0, size=0;

        /* Get entry */
        result = fs_list(&entry, argv[1], i);
        if(result >= ERROR_ANY) {
          putstr("Error\n\r");
          break;
        }

        /* Listed entry is a dir? If so,
         * start this line with a '+' */
        memset(line, 0, sizeof(line));
        strcpy_s(line, entry.flags & T_DIR ? "+ " : "  ", sizeof(line));
        strcat_s(line, entry.name, sizeof(line)); /* Append name */

        /* We want size to be right-aligned so add spaces
         * depending on figures of entry size */
        for(c=strlen(line); c<22; c++) {
          line[c] = ' ';
        }
        size = entry.size;
        while(size = size / 10 ) {
          line[--c] = 0;
        }

        /* Print name and size */
        putstr("%s%u %s   ", line, (uint)entry.size,
          (entry.flags & T_DIR) ? "items" : "bytes");

        /* Print date */
        fs_fstime_to_systime(entry.time, &etime);
        putstr("%d/%s%d/%s%d %s%d:%s%d:%s%d\n\r",
          etime.year,
          etime.month <10?"0":"", etime.month,
          etime.day   <10?"0":"", etime.day,
          etime.hour  <10?"0":"", etime.hour,
          etime.minute<10?"0":"", etime.minute,
          etime.second<10?"0":"", etime.second);
      }
      putstr("\n\r");
    }
  } else {
    putstr("usage: list <path>\n\r");
  }
}

/* Makedir command */
static void cli_makedir(uint argc, uchar* argv[])
{
  if(argc == 2) {
    uint result = fs_create_directory(argv[1]);
    if(result == ERROR_NOT_FOUND) {
      putstr("error: path not found\n\r");
    } else if(result == ERROR_EXISTS) {
      putstr("error: destination already exists\n\r");
    } else if(result == ERROR_NO_SPACE) {
      putstr("error: can't allocate destination in filesystem\n\r");
    } else if(result >= ERROR_ANY) {
      putstr("error: coludn't create directory\n\r");
    }
  } else {
    putstr("usage: makedir <path>\n\r");
  }
}

/* Delete command */
static void cli_delete(uint argc, uchar* argv[])
{
  if(argc == 2) {
    uint result = fs_delete(argv[1]);
    if(result >= ERROR_ANY) {
      putstr("error: failed to delete\n\r");
    }
  } else {
    putstr("usage: delete <path>\n\r");
  }
}

static void cli_move(uint argc, uchar* argv[])
{
  /* Move command */
  if(argc == 3) {
    uint result = fs_move(argv[1], argv[2]);
    if(result == ERROR_NOT_FOUND) {
      putstr("error: path not found\n\r");
    } else if(result == ERROR_EXISTS) {
      putstr("error: destination already exists\n\r");
    } else if(result == ERROR_NO_SPACE) {
      putstr("error: can't allocate destination in filesystem\n\r");
    } else if(result >= ERROR_ANY) {
      putstr("error: coludn't move files\n\r");
    }
  } else {
    putstr("usage: move <path> <newpath>\n\r");
  }
}

/* Copy command */
static void cli_copy(uint argc, uchar* argv[])
{
  if(argc == 3) {
    uint result = fs_copy(argv[1], argv[2]);
    if(result == ERROR_NOT_FOUND) {
      putstr("error: path not found\n\r");
    } else if(result == ERROR_EXISTS) {
      putstr("error: destination already exists\n\r");
    } else if(result == ERROR_NO_SPACE) {
      putstr("error: can't allocate destination in filesystem\n\r");
    } else if(result >= ERROR_ANY) {
      putstr("error: coludn't copy files\n\r");
    }
  } else {
    putstr("usage: copy <srcpath> <dstpath>\n\r");
  }
}

/* Info command: show system info */
static void cli_info(uint argc)
{
  if(argc == 1) {
    uint n=0, i=0;
    putstr("\n\r");
    putstr("NANO S16 [Version %u.%u build %u]\n\r",
      OS_VERSION_HI, OS_VERSION_LO, OS_BUILD_NUM);
    putstr("\n\r");

    putstr("Disks:\n\r");
    fs_init_info(); /* Rescan disks */
    for(i=0; i<MAX_DISK; i++) {
      n = index_to_disk(i);
      if(disk_info[i].size) {
        putstr("%s %s(%UMB)   Disk size: %UMB\n\r",
          disk_to_string(n), disk_info[i].fstype == FS_TYPE_NSFS ? "NSFS" : "UNKN",
          (ul_t)blocks_to_MB(disk_info[i].fssize), disk_info[i].size);
      }
    }
    putstr("\n\r");
    putstr("System disk: %s\n\r", disk_to_string(system_disk));
    putstr("Serial port status: %s\n\r", serial_status & 0x80 ? "Error" : "Enabled");
    putstr("A20 Line status: %s\n\r", a20_enabled ? "Enabled" : "Disabled");
    putstr("Network status: %s\n\r", network_enabled ? "Enabled" : "Disabled");
    putstr("Timer frequency: %UHz\n\r", system_timer_freq);
    putstr("System time alive: %Ums\n\r", system_timer_ms);
    putstr("\n\r");
  } else {
    putstr("usage: info\n\r");
  }
}

/* Clone command: clone system disk in another disk */
static void cli_clone(uint argc, uchar* argv[])
{
  if(argc == 2) {
    uint i=0, n=0, result=0;
    sfs_entry_t entry;
    uint disk=0, disk_index=0;
    uint sysdisk_index = disk_to_index(system_disk);

    /* Show source disk info */
    putstr("System disk: %s    fs=%s  size=%UMB\n\r",
      disk_to_string(system_disk),
      disk_info[sysdisk_index].fstype == FS_TYPE_NSFS ? "NSFS   " : "unknown",
      blocks_to_MB(disk_info[sysdisk_index].fssize));

    /* Check target disk */
    disk = string_to_disk(argv[1]);
    if(disk == ERROR_NOT_FOUND) {
      putstr("Target disk not found (%s)\n\r", argv[1]);
      return;
    }
    if(disk == system_disk) {
      putstr("Target disk can't be the system disk\n\r");
      return;
    }

    /* Show target disk info */
    disk_index = disk_to_index(disk);
    putstr("Target disk: %s    fs=%s  size=%UMB\n\r",
      disk_to_string(disk),
      disk_info[disk_index].fstype == FS_TYPE_NSFS ? "NSFS   " : "unknown",
      blocks_to_MB(disk_info[disk_index].fssize));

    putstr("\n\r");

    /* User should know this */
    putstr("Target disk (%s) will lose all data\n\r", disk_to_string(disk));
    putstr("Target disk (%s) will contain a %UMB NSFS filesystem after operation\n\r",
      disk_to_string(disk), disk_info[disk_index].size);

    /* Ask for confirmation */
    putstr("\n\r");
    putstr("Press 'y' to confirm: ");
    if(getkey(KM_WAIT_KEY) != 'y') {
      putstr("\n\rUser aborted operation\n\r");
      return;
    }

    putstr("y\n\r");

    /* Format disk and copy kernel */
    putstr("Formatting and copying system files...\n\r");
    result = fs_format(disk);
    if(result != 0) {
      putstr("Error formatting disk. Aborted\n\r");
      return;
    }

    /* Copy user files */
    putstr("Copying user files...\n\r");

    n = fs_list(&entry, ROOT_DIR_NAME, 0);
    if(n >= ERROR_ANY) {
      putstr("Error creating file list\n\r");
      return;
    }

    /* List entries */
    for(i=0; i<n; i++) {
      uchar dst[MAX_PATH];
      result = fs_list(&entry, ROOT_DIR_NAME, i);
      if(result >= ERROR_ANY) {
        putstr("Error copying files. Aborted\n\r");
        break;
      }

      strcpy_s(dst, argv[1], sizeof(dst));
      strcat_s(dst, PATH_SEPARATOR_S, sizeof(dst));
      strcat_s(dst, entry.name, sizeof(dst));

      debugstr("copy %s %s\n\r", entry.name, dst);
      result = fs_copy(entry.name, dst);
      /* Skip ERROR_EXISTS errors, because system files were copied
        * by fs_format function, so they are expected to fail */
      if(result >= ERROR_ANY && result != ERROR_EXISTS) {
        putstr("Error copying %s. Aborted\n\r", entry.name);
        break;
      }
    }

    /* Notify result */
    if(result < ERROR_ANY) {
      putstr("Operation completed\n\r");
    }
  } else {
    putstr("usage: clone <target_disk>\n\r");
  }
}

/* Read command: read a file */
static void cli_read(uint argc, uchar* argv[])
{
  if(argc==2 || (argc==3 && strcmp(argv[1],"hex")==0)) {
    uint result=0, i=0;
    uint offset = 0;
    uchar buff[512];
    memset(buff, 0, sizeof(buff));
    /* While it can read the file, print it */
    while(result = fs_read_file(buff, argv[argc-1], offset, sizeof(buff))) {
      if(result >= ERROR_ANY) {
        putstr("\n\rThere was an error reading input file\n\r");
        break;
      }
      for(i=0; i<result; i++) {
        if(argc==2) {
          putchar(buff[i]);
          /* Add '\r' after '\n' chars */
          if(buff[i] == '\n') {
            putchar('\r');
          }
        } else {
          putstr("%2x ", buff[i]);
        }
      }
      memset(buff, 0, sizeof(buff));
      offset += result;
    }
    putstr("\n\r");
  } else {
    putstr("usage: read [hex] <path>\n\r");
  }
}

/* Time command: Show date and time */
static void cli_time(uint argc)
{
  if(argc == 1) {
    time_t ctime;
    time(&ctime);

    putstr("\n\r%d/%s%d/%s%d %s%d:%s%d:%s%d\n\r\n\r",
      ctime.year,
      ctime.month <10?"0":"", ctime.month,
      ctime.day   <10?"0":"", ctime.day,
      ctime.hour  <10?"0":"", ctime.hour,
      ctime.minute<10?"0":"", ctime.minute,
      ctime.second<10?"0":"", ctime.second);
  } else {
    putstr("usage: time\n\r");
  }
}

/* Shutdown command: Shutdown computer */
static void cli_shutdown(uint argc, uchar* argv[])
{
  if(argc == 1) {
    apm_shutdown();

    /* This computer does not support APM */
    io_clear_screen();
    io_hide_cursor();
    putstr("Turn off computer\n\r");
    halt(); /* Halt() */
  } else if(argc == 2 && strcmp(argv[1], "reboot") == 0) {
      reboot();  /* Reboot computer */
      putstr("Reboot not supported\n\r");
  } else {
    putstr("usage: shutdown [reboot]\n\r");
  }
}

/* Config command: Show or edit config parameters */
static void cli_config(uint argc, uchar* argv[])
{
  if(argc == 1) {
    putstr("\n\r");
    putstr("debug: %s       - output debug info through serial port\n\r", serial_debug ? " enabled" : "disabled");
    putstr("graphics: %s    - use graphics mode\n\r", graphics_mode ? " enabled" : "disabled");
    putstr("net_IP: %u.%u.%u.%u\n\r", local_ip[0], local_ip[1], local_ip[2], local_ip[3]);
    putstr("net_gate: %u.%u.%u.%u\n\r", local_gate[0], local_gate[1], local_gate[2], local_gate[3]);
    putstr("\n\r");
  } else if(argc == 2 && strcmp(argv[1], "save") == 0) {
    uchar config_file[512];
    uchar tmps[32];

    /* Save config file */
    memset(config_file, 0, sizeof(config_file));
    strcat_s(config_file, "config debug ", sizeof(config_file));
    strcat_s(config_file, serial_debug?"enabled":"disabled", sizeof(config_file));
    strcat_s(config_file, "\n", sizeof(config_file));

    strcat_s(config_file, "config graphics ", sizeof(config_file));
    strcat_s(config_file, graphics_mode?"enabled":"disabled", sizeof(config_file));
    strcat_s(config_file, "\n", sizeof(config_file));

    strcat_s(config_file, "config net_IP ", sizeof(config_file));
    strcat_s(config_file, ip_to_str(tmps, local_ip), sizeof(config_file));
    strcat_s(config_file, "\n", sizeof(config_file));

    strcat_s(config_file, "config net_gate ", sizeof(config_file));
    strcat_s(config_file, ip_to_str(tmps, local_gate), sizeof(config_file));
    strcat_s(config_file, "\n", sizeof(config_file));

    fs_write_file(config_file, "config.ini", 0, strlen(config_file)+1, WF_CREATE|WF_TRUNCATE);
    debugstr("Config file saved\n\r");

  } else if(argc == 3) {
    if(strcmp(argv[1], "debug") == 0) {
      if(strcmp(argv[2], "enabled") == 0) {
        serial_debug = 1;
      } else if(strcmp(argv[2], "disabled") == 0) {
        serial_debug = 0;
      } else {
        putstr("Invalid value. Valid values are: enabled, disabled\n\r");
      }
    } else if(strcmp(argv[1], "graphics") == 0) {
      if(strcmp(argv[2], "enabled") == 0) {
        io_set_graphics_mode();
        io_set_cursor_pos(0, 0);
      } else if(strcmp(argv[2], "disabled") == 0) {
        io_set_text_mode();
        io_set_cursor_pos(0, 0);
      } else {
        putstr("Invalid value. Valid values are: enabled, disabled\n\r");
      }
    } else if(strcmp(argv[1], "net_IP") == 0) {
      str_to_ip(local_ip, argv[2]);
    } else if(strcmp(argv[1], "net_gate") == 0) {
      str_to_ip(local_gate, argv[2]);
    }

  } else {
    putstr("usages:\n\rconfig\n\rconfig save\n\rconfig <var> <value>");
  }
}

/* Not a built-in command */
/* Try to find an executable file */
static void cli_extern(uint argc, uchar* argv[])
{
  uint result = 0;
  uchar* prog_ext = ".bin";
  sfs_entry_t entry;
  uchar prog_file_name[32];
  strcpy_s(prog_file_name, argv[0], sizeof(prog_file_name));

  /* Append .bin if there is not a '.' in the name*/
  if(!strchr(prog_file_name, '.')) {
    strcat_s(prog_file_name, prog_ext, sizeof(prog_file_name));
  }

  /* Find .bin file */
  result = fs_get_entry(&entry, prog_file_name, UNKNOWN_VALUE, UNKNOWN_VALUE);
  if(result < ERROR_ANY) {
    /* Found */
    if(entry.flags & T_FILE) {
      uint offset = 0;
      /* It's a file: load it */
      uint mem_size = min((uint)entry.size, UPROG_ARGLOC-UPROG_MEMLOC);
      if(mem_size < (uint)entry.size) {
        putstr("not enough memory\n\r");
        return;
      }
      while(offset < mem_size) {
        uint r = 0;
        uint count = 0;
        uchar buff[512];
        count = min(mem_size-offset, sizeof(buff));
        r = fs_read_file(buff, prog_file_name, offset, count);
        if(r<ERROR_ANY) {
          uint b = 0;
          for(b=0; b<r; b++) {
            lmem_setbyte((lp_t)(UPROG_MEMSEG<<4)+UPROG_MEMLOC+(lp_t)(offset+b), buff[b]);
          }
          offset += r;
        } else  {
          putstr("error loading file\n\r");
          debugstr("error loading file\n\r");
          result = ERROR_IO;
          break;
        }
      }
    } else {
      /* It's not a file: error */
      result = ERROR_NOT_FOUND;
    }
  }

  if(result >= ERROR_ANY || result == 0) {
    putstr("unknown command\n\r");
  } else {
    uint uarg = 0;
    uint c = 0, i = 0;
    lp_t arg_var = (UPROG_MEMSEG<<4)+UPROG_ARGLOC;
    lp_t arg_str = (UPROG_MEMSEG<<4)+UPROG_STRLOC;

    /* Check name ends with ".bin" */
    if(strcmp(&prog_file_name[strchr(prog_file_name, '.') - 1], prog_ext)) {
      putstr("error: only %s files can be executed\n\r", prog_ext);
      return;
    }

    /* Create argv copy in program segment */
    for(uarg=0; uarg<argc; uarg++) {
      lmem_setbyte(arg_var+(lp_t)(2*uarg), (UPROG_STRLOC+c)&0xFF);
      lmem_setbyte(arg_var+(lp_t)(2*uarg+1), ((UPROG_STRLOC+c)>>8)&0xFF);
      for(i=0; i<strlen(argv[uarg])+1; i++) {
        lmem_setbyte(arg_str+(lp_t)c, argv[uarg][i]);
        c++;
      }
    }

    debugstr("CLI: Running program %s (%d bytes)\n\r",
      prog_file_name, (uint)entry.size);

    /* Run program */
    uprog_call(argc, UPROG_ARGLOC);
  }
}

/*
 * Execute command
 */
#define CLI_MAX_ARG 4
static void execute(uchar* str)
{
  uint   argc = 0;
  uchar* argv[CLI_MAX_ARG];
  uchar* tok = str;
  uchar* nexttok = tok;

  memset(argv, 0, sizeof(argv));
  debugstr("%Ums> %s\n\r", system_timer_ms, str);

  /* Tokenize */
  argc = 0;
  while(*tok && *nexttok && argc < CLI_MAX_ARG) {
    tok = strtok(tok, &nexttok, ' ');
    if(*tok) {
      argv[argc++] = tok;
    }
    tok = nexttok;
  }

  /* Process command */
  if(argc == 0) {
      /* Empty command line, skip */
  }
  /* Built in commands */
  else if(strcmp(argv[0], "cls") == 0) {  
    cli_cls(argc);

  } else if(strcmp(argv[0], "list") == 0) {
    cli_list(argc, argv);  

  } else if(strcmp(argv[0], "makedir") == 0) {
    cli_makedir(argc, argv);

  } else if(strcmp(argv[0], "delete") == 0) {
    cli_delete(argc, argv);
  
  } else if(strcmp(argv[0], "move") == 0) {
    cli_move(argc, argv);

  } else if(strcmp(argv[0], "copy") == 0) {
    cli_copy(argc, argv);

  } else if(strcmp(argv[0], "info") == 0) {
    cli_info(argc);

  } else if(strcmp(argv[0], "clone") == 0) {
    cli_clone(argc, argv);

  } else if(strcmp(argv[0], "read") == 0) {
    cli_read(argc, argv);

  } else if(strcmp(argv[0], "time") == 0) {
    cli_time(argc);

  } else if(strcmp(argv[0], "shutdown") == 0) {
    cli_shutdown(argc, argv);

  } else if(strcmp(argv[0], "config") == 0) {
    cli_config(argc, argv);
    

  } else if(strcmp(argv[0], "help") == 0) {
    /* Help command: Show help, and some tricks */
    if(argc == 1) {
      putstr("\n\r");
      putstr("Built-in commands:\n\r");
      putstr("\n\r");
      putstr("clone    - clone system in another disk\n\r");
      putstr("cls      - clear the screen\n\r");
      putstr("config   - show or set config\n\r");
      putstr("copy     - create a copy of a file or directory\n\r");
      putstr("delete   - delete entry\n\r");
      putstr("help     - show this help\n\r");
      putstr("info     - show system info\n\r");
      putstr("list     - list directory contents\n\r");
      putstr("makedir  - create directory\n\r");
      putstr("move     - move file or directory\n\r");
      putstr("read     - show file contents in screen\n\r");
      putstr("shutdown - shutdown the computer\n\r");
      putstr("time     - show time and date\n\r");
      putstr("\n\r");
    } else if(argc == 2 && strcmp(argv[1], "huri") == 0) { /* Easter egg */
      putstr("\n\r");
      putstr("                                     _,-/\\^---,      \n\r");
      putstr("             ;\"~~~~~~~~\";          _/;; ~~  {0 `---v \n\r");
      putstr("           ;\" :::::   :: \"\\_     _/   ;;     ~ _../  \n\r");
      putstr("         ;\" ;;    ;;;       \\___/::    ;;,'~~~~      \n\r");
      putstr("       ;\"  ;;;;.    ;;     ;;;    ::   ,/            \n\r");
      putstr("      / ;;   ;;;______;;;;  ;;;    ::,/              \n\r");
      putstr("     /;;V_;; _-~~~~~~~~~~;_  ;;;   ,/                \n\r");
      putstr("    | :/ / ,/              \\_  ~~)/                  \n\r");
      putstr("    |:| / /~~~=              \\;; \\~~=                \n\r");
      putstr("    ;:;{::~~~~~~=              \\__~~~=               \n\r");
      putstr(" ;~~:;  ~~~~~~~~~               ~~~~~~               \n\r");
      putstr(" \\/~~                                               \n\r");
      putstr("\n\r");
    } else {
      putstr("usage: help\n\r");
    }

  } else {
    /* Not a built-in command */
    /* Try to find an executable file */
    cli_extern(argc, argv);
  } 
}

/*
 * Execute a script file
 */
void cli_exec_file(uchar* path)
{
  uchar line[72];
  uint offset = 0;
  uint readed = 0;
  uint i = 0;

  while(1) {
    /* Read file */
    readed = fs_read_file(line, path, offset, sizeof(line));
    if(readed==0 || readed>=ERROR_ANY) {
      return;
    }

    /* Isolate a line */
    for(i=0; i<readed; i++) {
      if(line[i] == '\n') {
        line[i] = 0;
        offset += 1+i;
        break;
      }
    }

    /* Escape if lines are too long */
    if(i==readed) {
      return;
    }

    /* Run line */
    execute(line);
  }
}

/* Integrated CLI */
void cli() 
{
  while(1) {
    uchar  str[72];
    memset(str, 0, sizeof(str));

    /* Prompt and wait command */
    putstr("> ");
    getstr(str, sizeof(str));

    execute(str);
  }
}